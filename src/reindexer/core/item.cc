
#include "core/item.h"
#include "core/itemimpl.h"
#include "core/keyvalue/p_string.h"
#include "core/namespace.h"

namespace reindexer {

Item::FieldRef::FieldRef(int field, ItemImpl *itemImpl) : itemImpl_(itemImpl), field_(field) {}
Item::FieldRef::FieldRef(const string &jsonPath, ItemImpl *itemImpl) : itemImpl_(itemImpl), jsonPath_(jsonPath), field_(-1) {}

Item::Item(Item &&other) noexcept : impl_(other.impl_), status_(std::move(other.status_)), id_(other.id_) { other.impl_ = nullptr; }

Item &Item::operator=(Item &&other) noexcept {
	if (&other != this) {
		delete impl_;
		impl_ = other.impl_;
		status_ = std::move(other.status_);
		id_ = other.id_;
		other.impl_ = nullptr;
	}
	return *this;
}

const string &Item::FieldRef::Name() { return field_ >= 0 ? itemImpl_->Type().Field(field_).Name() : jsonPath_; }
Item::FieldRef::operator Variant() {
	VariantArray kr;
	if (field_ >= 0)
		itemImpl_->GetPayload().Get(field_, kr);
	else
		kr = itemImpl_->GetValueByJSONPath(jsonPath_);

	if (kr.size() != 1) {
		throw Error(errParams, "Invalid array access");
	}
	return kr[0];
}

Item::FieldRef::operator VariantArray() {
	VariantArray kr;
	if (field_ >= 0)
		itemImpl_->GetPayload().Get(field_, kr);
	else
		kr = itemImpl_->GetValueByJSONPath(jsonPath_);
	return kr;
}

Item::FieldRef &Item::FieldRef::operator=(Variant kr) {
	if (field_ >= 0) {
		itemImpl_->SetField(field_, VariantArray{kr});
	} else {
		throw Error(errConflict, "Item::FieldRef::SetValue by json path not implemented yet");
	}

	return *this;
}

Item::FieldRef &Item::FieldRef::operator=(const char *str) { return operator=(p_string(str)); }
Item::FieldRef &Item::FieldRef::operator=(const string &str) { return operator=(p_string(&str)); }

Item::FieldRef &Item::FieldRef::operator=(const VariantArray &krs) {
	if (field_ >= 0) {
		itemImpl_->SetField(field_, krs);
	} else {
		throw Error(errConflict, "Item::FieldRef::SetValue by json path not implemented yet");
	}
	return *this;
}

Item::~Item() {
	if (impl_) {
		auto ns = impl_->GetNamespace();
		if (ns) {
			ns->ToPool(impl_);
			impl_ = nullptr;
		}
	}
	delete impl_;
}

Error Item::FromJSON(const string_view &slice, char **endp, bool pkOnly) { return impl_->FromJSON(slice, endp, pkOnly); }
Error Item::FromCJSON(const string_view &slice, bool pkOnly) { return impl_->FromCJSON(slice, pkOnly); }
string_view Item::GetCJSON() { return impl_->GetCJSON(); }
string_view Item::GetJSON() { return impl_->GetJSON(); }

int Item::NumFields() { return impl_->Type().NumFields(); }
Item::FieldRef Item::operator[](int field) {
	assert(field >= 0 && field < impl_->Type().NumFields());
	return FieldRef(field, impl_);
}

Item::FieldRef Item::operator[](const string &name) {
	int field = 0;
	if (impl_->Type().FieldByName(name, field)) {
		return FieldRef(field, impl_);
	} else {
		return FieldRef(name, impl_);
	}
}

Item::FieldRef Item::operator[](const char *name) {
	int field = 0;
	if (impl_->Type().FieldByName(name, field)) {
		return FieldRef(field, impl_);
	} else {
		return FieldRef(name, impl_);
	}
}

void Item::SetPrecepts(const vector<string> &precepts) { impl_->SetPrecepts(precepts); }
bool Item::IsTagsUpdated() { return impl_->tagsMatcher().isUpdated(); }
int Item::GetStateToken() { return impl_->tagsMatcher().stateToken(); }

Item &Item::Unsafe(bool enable) {
	impl_->Unsafe(enable);
	return *this;
}

int64_t Item::GetLSN() { return impl_->Value().GetLSN(); }
void Item::setLSN(int64_t lsn) { impl_->Value().SetLSN(lsn); }

}  // namespace reindexer
