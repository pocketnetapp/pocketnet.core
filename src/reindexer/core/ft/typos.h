#pragma once

#include <functional>
#include <string>

namespace reindexer {

using std::string;
using std::wstring;

struct typos_context {
	wstring utf16Word, utf16Typo;
	string typo;
};
const int kMaxTyposInWord = 4;

void mktypos(typos_context *ctx, const wstring &word, int level, int maxTyposLen, std::function<void(const string &, int)> callback);
void mktypos(typos_context *ctx, const char *word, int level, int maxTyposLen, std::function<void(const string &, int)> callback);

}  // namespace reindexer
