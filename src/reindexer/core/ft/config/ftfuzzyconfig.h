#pragma once

#include "baseftconfig.h"

namespace reindexer {

using std::vector;
using std::string;

struct FtFuzzyConfig : public BaseFTConfig {
	virtual void parse(char *json) final;

	double maxSrcProc = 78;
	double maxDstProc = 22;
	double posSourceBoost = 1.5;
	double posSourceDistMin = 0.3;
	double posSourceDistBoost = 1.2;
	double posDstBoost = 1;
	double startDecreeseBoost = 1.2;
	double startDefaultDecreese = 0.7;
	double minOkProc = 10;
	size_t bufferSize = 3;
	size_t spaceSize = 2;
};

const size_t maxFuzzyFTBufferSize = 10;

}  // namespace reindexer
