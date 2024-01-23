#pragma once

#include <string>
#include <string_view>
#include <vector>

struct TextCompletionI {
	virtual ~TextCompletionI(void) {}

	virtual bool isGood(void) = 0;

	// TODO: add more complex api

	virtual int64_t completeSelect(const std::string_view prompt, const std::vector<std::string_view>& possible) = 0;

	// stops at newlines
	// (and limit of 1000 and eos)
	virtual std::string completeLine(const std::string_view prompt) = 0;
};

