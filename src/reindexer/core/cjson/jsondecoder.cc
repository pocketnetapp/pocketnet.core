#include "jsondecoder.h"
#include "cjsonbuilder.h"
#include "tagsmatcher.h"
#include "tools/json2kv.h"
#include "tools/serializer.h"

namespace reindexer {

JsonDecoder::JsonDecoder(TagsMatcher &tagsMatcher) : tagsMatcher_(tagsMatcher), filter_(nullptr) {}
JsonDecoder::JsonDecoder(TagsMatcher &tagsMatcher, const FieldsSet *filter) : tagsMatcher_(tagsMatcher), filter_(filter) {}

Error JsonDecoder::Decode(Payload *pl, WrSerializer &wrser, JsonValue &v) {
	try {
		wrser.Reset();
		CJsonBuilder builder(wrser, CJsonBuilder::TypePlain, &tagsMatcher_);
		decodeJson(pl, builder, v, 0, true);
	}

	catch (const Error &err) {
		return err;
	}
	return 0;
}

void JsonDecoder::decodeJsonObject(Payload *pl, CJsonBuilder &builder, JsonValue &v, bool match) {
	for (auto elem : v) {
		int tagName = tagsMatcher_.name2tag(elem->key, true);
		assert(tagName);
		tagsPath_.push_back(tagName);
		int field = tagsMatcher_.tags2field(tagsPath_.data(), tagsPath_.size());
		if (filter_) {
			if (field >= 0)
				match = filter_->contains(field);
			else
				match = match && filter_->match(tagsPath_);
		}

		if (field < 0) {
			decodeJson(pl, builder, elem->value, tagName, match);
		} else if (match) {
			// Indexed field. extract it
			auto &f = pl->Type().Field(field);
			if (elem->value.getTag() == JSON_ARRAY) {
				if (!f.IsArray()) {
					throw Error(errLogic, "Error parsing json field '%s' - got array, expected scalar %s", f.Name().c_str(),
								Variant::TypeName(f.Type()));
				}
				int count = 0;
				for (auto subelem : elem->value) {
					(void)subelem;
					count++;
				}
				int pos = pl->ResizeArray(field, count, true);
				for (auto subelem : elem->value) {
					pl->Set(field, pos++, jsonValue2Variant(subelem->value, f.Type(), f.Name().c_str()));
				}
				builder.ArrayRef(tagName, field, count);
			} else if (elem->value.getTag() != JSON_NULL) {
				Variant v = jsonValue2Variant(elem->value, f.Type(), f.Name().c_str());
				pl->Set(field, {v}, true);
				builder.Ref(tagName, v, field);
			} else {
				builder.Null(tagName);
			}
		}
		tagsPath_.pop_back();
	}
}

// Split original JSON into 2 parts:
// 1. PayloadFields - fields from json found by 'jsonPath' tags
// 2. stripped binary packed JSON without fields values found by 'jsonPath' tags
void JsonDecoder::decodeJson(Payload *pl, CJsonBuilder &builder, JsonValue &v, int tagName, bool match) {
	auto jsonTag = v.getTag();
	if (!match && jsonTag != JSON_OBJECT) return;
	switch (jsonTag) {
		case JSON_NUMBER: {
			double value = v.toNumber(), intpart;
			if (std::modf(value, &intpart) == 0.0) {
				builder.Put(tagName, int64_t(value));
			} else {
				builder.Put(tagName, value);
			}
		} break;
		case JSON_STRING:
			builder.Put(tagName, v.toString());
			break;
		case JSON_TRUE:
			builder.Put(tagName, true);
			break;
		case JSON_FALSE:
			builder.Put(tagName, false);
			break;
		case JSON_NULL:
			builder.Null(tagName);
			break;
		case JSON_ARRAY: {
			auto arrNode = builder.Array(tagName);
			for (auto elem : v) {
				decodeJson(pl, arrNode, elem->value, 0, match);
			}
			break;
		}
		case JSON_OBJECT: {
			auto node = builder.Object(tagName);
			decodeJsonObject(pl, node, v, match);
			break;
		}
	}
}

}  // namespace reindexer
