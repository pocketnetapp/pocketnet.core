#pragma once

#include <string>
#include <vector>
#include "estl/fast_hash_set.h"
#include "gason/gason.h"

namespace reindexer {

using std::vector;
using std::string;

class BaseFTConfig {
public:
	BaseFTConfig();
	virtual ~BaseFTConfig() = default;

	virtual void parse(char *json) = 0;

	int mergeLimit = 20000;
	vector<string> stemmers = {"en", "ru"};
	bool enableTranslit = true;
	bool enableKbLayout = true;
	bool enableNumbersSearch = false;
	fast_hash_set<string> stopWords;
	int logLevel = 0;
	string extraWordSymbols = "-/+%.";

protected:
	void parseBase(const JsonNode *val);
};

}  // namespace reindexer
