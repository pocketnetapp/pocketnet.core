
#pragma once

#include <assert.h>
#include <stdint.h>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <type_traits>
#include <vector>

namespace reindexer {
#if 0
template <typename T, int holdSize>
class h_vector : public std::vector<T> {};
#else

#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ == 4
template <typename T>
using is_trivially_default_constructible = std::has_trivial_default_constructor<T>;
#else
template <typename T>
using is_trivially_default_constructible = std::is_trivially_default_constructible<T>;
#endif

using std::iterator;
using std::iterator_traits;

template <class Iterator>
class trivial_reverse_iterator
	: public iterator<typename iterator_traits<Iterator>::iterator_category, typename iterator_traits<Iterator>::value_type,
					  typename iterator_traits<Iterator>::difference_type, typename iterator_traits<Iterator>::pointer,
					  typename iterator_traits<Iterator>::reference> {
public:
	typedef trivial_reverse_iterator this_type;
	typedef Iterator iterator_type;
	typedef typename iterator_traits<Iterator>::difference_type difference_type;
	typedef typename iterator_traits<Iterator>::reference reference;
	typedef typename iterator_traits<Iterator>::pointer pointer;

public:
	//	if CTOR is enabled std::is_trivial<trvial_reverse_iterator<...>> return false;
	//	trivial_reverse_iterator() : current_(nullptr) {}

	template <class Up>
	trivial_reverse_iterator& operator=(const trivial_reverse_iterator<Up>& u) {
		current_ = u.base();
		return *this;
	}

	Iterator base() const { return current_; }
	reference operator*() const {
		Iterator tmp = current_;
		return *--tmp;
	}
	pointer operator->() const { return std::addressof(operator*()); }
	trivial_reverse_iterator& operator++() {
		--current_;
		return *this;
	}
	trivial_reverse_iterator operator++(int) {
		trivial_reverse_iterator tmp(*this);
		--current_;
		return tmp;
	}
	trivial_reverse_iterator& operator--() {
		++current_;
		return *this;
	}
	trivial_reverse_iterator operator--(int) {
		trivial_reverse_iterator tmp(*this);
		++current_;
		return tmp;
	}
	trivial_reverse_iterator operator+(difference_type n) const {
		Iterator ptr = current_ - n;
		trivial_reverse_iterator tmp;
		tmp = ptr;
		return tmp;
	}
	trivial_reverse_iterator& operator+=(difference_type n) {
		current_ -= n;
		return *this;
	}
	trivial_reverse_iterator operator-(difference_type n) const {
		Iterator ptr = current_ + n;
		trivial_reverse_iterator tmp;
		tmp = ptr;
		return tmp;
	}
	trivial_reverse_iterator& operator-=(difference_type n) {
		current_ += n;
		return *this;
	}
	reference operator[](difference_type n) const { return *(*this + n); }

	// Assign operator overloading from const std::reverse_iterator<U>
	template <typename U>
	trivial_reverse_iterator& operator=(const std::reverse_iterator<U>& u) {
		if (current_ != u.base()) current_ = u.base();
		return *this;
	}

	// Assign operator overloading from non-const std::reverse_iterator<U>
	template <typename U>
	trivial_reverse_iterator& operator=(std::reverse_iterator<U>& u) {
		if (current_ != u.base()) current_ = u.base();
		return *this;
	}

	// Assign native pointer
	template <class Upn>
	trivial_reverse_iterator& operator=(Upn ptr) {
		static_assert(std::is_pointer<Upn>::value, "attempting assign a non-trivial pointer");
		/*if (current_ != ptr)*/ current_ = ptr;
		return *this;
	}

	inline bool operator!=(const this_type& rhs) const { return !EQ(current_, rhs.current_); }
	inline bool operator==(const this_type& rhs) const { return EQ(current_, rhs.current_); }

protected:
	Iterator current_;

private:
	inline bool EQ(Iterator lhs, Iterator rhs) const { return lhs == rhs; }
};

template <typename T, int holdSize = 4, int objSize = sizeof(T)>
class h_vector {
public:
	typedef T value_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef const_pointer const_iterator;
	typedef pointer iterator;
	typedef trivial_reverse_iterator<const_iterator> const_reverse_iterator;
	typedef trivial_reverse_iterator<iterator> reverse_iterator;
	typedef unsigned size_type;
	typedef std::ptrdiff_t difference_type;
	h_vector() noexcept : e_{0, 0}, size_(0), is_hdata_(1) {}
	h_vector(size_type size) : h_vector() { resize(size); }
	h_vector(std::initializer_list<T> l) : e_{0, 0}, size_(0), is_hdata_(1) { insert(begin(), l.begin(), l.end()); }
	template <typename InputIt>
	h_vector(InputIt first, InputIt last) : e_{0, 0}, size_(0), is_hdata_(1) {
		insert(begin(), first, last);
	}
	h_vector(const h_vector& other) : e_{0, 0}, size_(0), is_hdata_(1) {
		reserve(other.capacity());
		for (size_type i = 0; i < other.size(); i++) new (ptr() + i) T(other.ptr()[i]);
		size_ = other.size_;
	}
	h_vector(h_vector&& other) noexcept : size_(0), is_hdata_(1) {
		if (other.is_hdata()) {
			for (size_type i = 0; i < other.size(); i++) {
				new (ptr() + i) T(std::move(other.ptr()[i]));
				other.ptr()[i].~T();
			}
		} else {
			e_.data_ = other.e_.data_;
			e_.cap_ = other.capacity();
			other.is_hdata_ = 1;
			is_hdata_ = 0;
		}
		size_ = other.size_;
		other.size_ = 0;
	}
	~h_vector() { clear(); }
	h_vector& operator=(const h_vector& other) {
		if (&other != this) {
			reserve(other.capacity());
			size_type mv = other.size() > size() ? size() : other.size();
			std::copy(other.begin(), other.begin() + mv, begin());
			size_type i = mv;
			for (; i < other.size(); i++) new (ptr() + i) T(other.ptr()[i]);
			for (; i < size(); i++) ptr()[i].~T();
			size_ = other.size_;
		}
		return *this;
	}

	h_vector& operator=(h_vector&& other) noexcept {
		if (&other != this) {
			if (other.is_hdata()) {
				size_type mv = other.size() > size() ? size() : other.size();
				std::move(other.begin(), other.begin() + mv, begin());
				size_type i = mv;
				for (; i < other.size(); i++) {
					new (ptr() + i) T(std::move(other.ptr()[i]));
				}
				for (; i < size(); i++) ptr()[i].~T();

				for (i = 0; i < other.size(); i++) {
					other.ptr()[i].~T();
				}
			} else {
				clear();
				e_.data_ = other.e_.data_;
				e_.cap_ = other.capacity();
				other.is_hdata_ = 1;
				is_hdata_ = 0;
			}
			size_ = other.size_;
			other.size_ = 0;
		}
		return *this;
	}

	bool operator==(const h_vector& other) const noexcept {
		if (&other != this) {
			if (size() != other.size()) return false;
			for (size_t i = 0; i < size(); ++i) {
				if (at(i) != other.at(i)) return false;
			}
			return true;
		}
		return true;
	}
	bool operator!=(const h_vector& other) const noexcept { return !operator==(other); }

	void clear() {
		resize(0);
		if (!is_hdata()) operator delete(static_cast<void*>(e_.data_));
		is_hdata_ = 1;
	}

	iterator begin() noexcept { return ptr(); }
	iterator end() noexcept { return ptr() + size_; }
	const_iterator begin() const noexcept { return ptr(); }
	const_iterator end() const noexcept { return ptr() + size_; }
	reverse_iterator rbegin() const noexcept {
		reverse_iterator it;
		it = end();
		return it;
	}
	reverse_iterator rend() const noexcept {
		reverse_iterator it;
		it = begin();
		return it;
	}
	reverse_iterator rbegin() noexcept {
		reverse_iterator it;
		it = end();
		return it;
	}
	reverse_iterator rend() noexcept {
		reverse_iterator it;
		it = begin();
		return it;
	}
	size_type size() const noexcept { return size_; }
	size_type capacity() const noexcept { return is_hdata_ ? holdSize : e_.cap_; }
	bool empty() const noexcept { return size_ == 0; }
	const_reference operator[](size_type pos) const { return ptr()[pos]; }
	reference operator[](size_type pos) { return ptr()[pos]; }
	const_reference at(size_type pos) const { return ptr()[pos]; }
	reference at(size_type pos) { return ptr()[pos]; }
	reference back() { return ptr()[size() - 1]; }
	reference front() { return ptr()[0]; }
	const_reference back() const { return ptr()[size() - 1]; }
	const_reference front() const { return ptr()[0]; }
	const_pointer data() const noexcept { return ptr(); }
	pointer data() noexcept { return ptr(); }

	void resize(size_type sz) {
		grow(sz);
		if (!reindexer::is_trivially_default_constructible<T>::value)
			for (size_type i = size_; i < sz; i++) new (ptr() + i) T();
		if (!std::is_trivially_destructible<T>::value)
			for (size_type i = sz; i < size_; i++) ptr()[i].~T();
		size_ = sz;
	}
	void reserve(size_type sz) {
		if (sz > capacity()) {
			assert(sz > holdSize);
			pointer new_data = static_cast<pointer>(operator new(sz * sizeof(T)));  // ?? dynamic
			pointer oold_data = ptr();
			pointer old_data = ptr();
			for (size_type i = 0; i < size_; i++) {
				new (new_data + i) T(std::move(*old_data));
				if (!std::is_trivially_destructible<T>::value) old_data->~T();
				old_data++;
			}
			if (!is_hdata()) operator delete(static_cast<void*>(oold_data));
			e_.data_ = new_data;
			e_.cap_ = sz;
			is_hdata_ = 0;
		}
	}
	void grow(size_type sz) {
		if (sz > capacity()) reserve(sz + capacity() * 2);
	}
	void push_back(const T& v) {
		grow(size_ + 1);
		new (ptr() + size_) T(v);
		size_++;
	}
	void push_back(T&& v) {
		grow(size_ + 1);
		new (ptr() + size_) T(std::move(v));
		size_++;
	}
	void pop_back() {
		assert(size_);
		resize(size_ - 1);
	}
	iterator insert(const_iterator pos, const T& v) {
		size_type i = pos - begin();
		assert(i <= size());
		grow(size_ + 1);
		resize(size_ + 1);
		std::move_backward(begin() + i, end() - 1, end());
		ptr()[i] = v;
		return begin() + i;
	}
	iterator insert(const_iterator pos, T&& v) {
		size_type i = pos - begin();
		assert(i <= size());
		grow(size_ + 1);
		resize(size_ + 1);
		std::move_backward(begin() + i, end() - 1, end());
		ptr()[i] = std::move(v);
		return begin() + i;
	}
	iterator insert(const_iterator pos, size_type count, const T& v) {
		size_type i = pos - begin();
		assert(i <= size());
		grow(size_ + count);
		resize(size_ + count);
		std::move_backward(begin() + i, end() - count, end());
		for (size_type j = i; j < i + count; ++j) ptr()[j] = v;
		return begin() + i;
	}
	iterator erase(const_iterator it) { return erase(it, it + 1); }
	template <class InputIt>
	iterator insert(const_iterator pos, InputIt first, InputIt last) {
		size_type i = pos - begin();
		assert(i <= size());
		auto cnt = last - first;
		grow(size_ + cnt);
		resize(size_ + cnt);
		std::move_backward(begin() + i, end() - cnt, end());
		std::copy(first, last, begin() + i);
		return begin() + i;
	}
	template <class InputIt>
	void assign(InputIt first, InputIt last) {
		clear();
		insert(begin(), first, last);
	}
	iterator erase(const_iterator first, const_iterator last) {
		size_type i = first - ptr();
		auto cnt = last - first;
		assert(i <= size());

		std::move(begin() + i + cnt, end(), begin() + i);
		resize(size_ - (last - first));
		return begin() + i;
	}
	void shrink_to_fit() {
		if (is_hdata() || size_ == capacity()) return;

		h_vector tmp;
		tmp.reserve(size());
		tmp.insert(tmp.begin(), begin(), end());
		clear();
		*this = std::move(tmp);
	}
	size_t heap_size() const {
		if (is_hdata())
			return 0;
		else
			return capacity() * sizeof(T);
	}

protected:
	bool is_hdata() const noexcept { return is_hdata_; }
	pointer ptr() noexcept { return is_hdata() ? reinterpret_cast<pointer>(hdata_) : e_.data_; }
	const_pointer ptr() const noexcept { return is_hdata() ? reinterpret_cast<const_pointer>(hdata_) : e_.data_; }

#pragma pack(push, 1)
	struct edata {
		pointer data_;
		size_type cap_;
	};
#pragma pack(pop)

	union {
		edata e_;
		uint8_t hdata_[holdSize > 0 ? holdSize* objSize : 1];
	};
	size_type size_ : 31;
	size_type is_hdata_ : 1;
};
#endif

template <typename T>
class span {
public:
	typedef T value_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef const_pointer const_iterator;
	typedef pointer iterator;
	typedef trivial_reverse_iterator<const_iterator> const_reverse_iterator;
	typedef trivial_reverse_iterator<iterator> reverse_iterator;
	typedef size_t size_type;

	constexpr span() noexcept : data_(nullptr), size_(0) {}
	constexpr span(const span& other) noexcept : data_(other.data_), size_(other.size_) {}

	span& operator=(const span& other) noexcept {
		data_ = other.data_;
		size_ = other.size_;
		return *this;
	}

	span& operator=(span&& other) noexcept {
		data_ = other.data_;
		size_ = other.size_;
		return *this;
	}

	// FIXME: const override
	template <typename Container>
	constexpr span(const Container& other) noexcept : data_(const_cast<T*>(other.data())), size_(other.size()) {}

	constexpr span(const T* str, size_type len) : data_(const_cast<T*>(str)), size_(len) {}  // static??
	constexpr iterator begin() const noexcept { return data_; }
	constexpr iterator end() const noexcept { return data_ + size_; }
	/*constexpr*/ reverse_iterator rbegin() const noexcept {
		reverse_iterator it;
		it = end();
		return it;
	}
	/*constexpr*/ reverse_iterator rend() const noexcept {
		reverse_iterator it;
		it = begin();
		return it;
	}
	constexpr size_type size() const noexcept { return size_; }
	constexpr bool empty() const noexcept { return size_ == 0; }
	constexpr const T& operator[](size_type pos) const { return data_[pos]; }
	T& operator[](size_type pos) { return data_[pos]; }
	constexpr const T& at(size_type pos) const { return data_[pos]; }
	constexpr const T& front() const { return data_[0]; }
	constexpr const T& back() const { return data_[size() - 1]; }
	constexpr pointer data() const noexcept { return data_; }
	span subspan(size_type offset, size_type count) const noexcept {
		assert(offset + count <= size_);
		return span(data_ + offset, count);
	}

protected:
	pointer data_;
	size_type size_;
};

}  // namespace reindexer

namespace std {
template <typename C, int H>
inline static std::ostream& operator<<(std::ostream& o, const reindexer::h_vector<C, H>& vec) {
	o << "[";
	for (unsigned i = 0; i < vec.size(); i++) {
		if (i != 0) o << ",";
		o << vec[i] << " ";
	}
	o << "]";
	return o;
}

}  // namespace std
