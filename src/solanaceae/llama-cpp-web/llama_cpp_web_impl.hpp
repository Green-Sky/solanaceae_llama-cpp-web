#pragma once

#include "./text_completion_interface.hpp"

#include <httplib.h>
#include <nlohmann/json_fwd.hpp>

#include <random>

struct LlamaCppWeb : public TextCompletionI {
	// this mutex locks internally
	httplib::Client _cli{"http://localhost:8080"};

	// this is a bad idea
	static std::minstd_rand thread_local _rng;

	~LlamaCppWeb(void);

	bool isGood(void) override;
	int64_t completeSelect(const std::string_view prompt, const std::vector<std::string_view>& possible) override;
	std::string completeLine(const std::string_view prompt) override;

	// TODO: expose?
	nlohmann::json complete(const nlohmann::json& request_j);
};

