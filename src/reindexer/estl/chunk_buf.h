#pragma once

#include <stdlib.h>
#include <mutex>
#include <vector>
#include "h_vector.h"
#include "string_view.h"

namespace reindexer {

class chunk {
public:
	chunk() : data_(nullptr), len_(0), offset_(0), cap_(0) {}
	~chunk() { delete[] data_; }
	chunk(const chunk &) = delete;
	chunk &operator=(const chunk &) = delete;
	chunk(chunk &&other) noexcept {
		data_ = other.data_;
		len_ = other.len_;
		offset_ = other.offset_;
		cap_ = other.cap_;
		other.data_ = nullptr;
		other.len_ = 0;
		other.cap_ = 0;
		other.offset_ = 0;
	}
	chunk &operator=(chunk &&other) noexcept {
		if (this != &other) {
			delete[] data_;
			data_ = other.data_;
			len_ = other.len_;
			offset_ = other.offset_;
			cap_ = other.cap_;
			other.data_ = nullptr;
			other.len_ = 0;
			other.cap_ = 0;
			other.offset_ = 0;
		}
		return *this;
	}
	void append(string_view data) {
		if (!data_ || len_ + data.size() > cap_) {
			cap_ = std::max(0x1000, int(len_ + data.size()));
			uint8_t *newdata = new uint8_t[cap_];
			if (data_) {
				memcpy(newdata, data_, len_);
			}
			data_ = newdata;
		}
		memcpy(data_ + len_, data.data(), data.size());
		len_ += data.size();
	}
	unsigned size() { return len_ - offset_; }
	uint8_t *data() { return data_ + offset_; }

	uint8_t *data_;
	size_t len_;
	size_t offset_;
	size_t cap_;
};

template <typename Mutex>
class chain_buf {
public:
	chain_buf(unsigned cap) : ring_(cap) {}
	void write(chunk &&ch) {
		if (ch.size()) {
			std::unique_lock<Mutex> lck(mtx_);
			ring_[head_] = std::move(ch);
			head_ = (head_ + 1) % ring_.size();
			assert(head_ != tail_);
		} else
			std::move(ch);
	}
	void write(string_view sv) {
		chunk chunk = get_chunk();
		chunk.append(sv);
		write(std::move(chunk));
	}
	span<chunk> tail() {
		std::unique_lock<Mutex> lck(mtx_);
		size_t cnt = ((tail_ > head_) ? ring_.size() : head_) - tail_;
		return span<chunk>(ring_.data() + tail_, cnt);
	}
	void erase(size_t nread) {
		std::unique_lock<Mutex> lck(mtx_);
		while (nread) {
			assert(head_ != tail_);
			chunk &cur = ring_[tail_];
			if (cur.size() > nread) {
				cur.offset_ += nread;
				break;
			}
			nread -= cur.size();
			cur.len_ = 0;
			cur.offset_ = 0;
			if (free_.size() < ring_.size() && cur.cap_ < 0x10000)
				free_.push_back(std::move(cur));
			else
				std::move(cur);
			tail_ = (tail_ + 1) % ring_.size();
		}
	}
	chunk get_chunk() {
		std::unique_lock<Mutex> lck(mtx_);
		chunk ret;
		if (free_.size()) {
			ret = std::move(free_.back());
			free_.pop_back();
		}
		return ret;
	}

	unsigned size() {
		std::unique_lock<Mutex> lck(mtx_);
		return (head_ - tail_ + ring_.size()) % ring_.size();
	}
	void clear() {
		std::unique_lock<Mutex> lck(mtx_);
		head_ = tail_ = 0;
	}

protected:
	unsigned head_ = 0, tail_ = 0;
	std::vector<chunk> ring_, free_;
	Mutex mtx_;
};

};  // namespace reindexer
