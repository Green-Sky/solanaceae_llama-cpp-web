#pragma once

#include "./text_completion_interface.hpp"

#include <solanaceae/util/config_model.hpp>

#include <httplib.h>
#include <nlohmann/json_fwd.hpp>

#include <random>
#include <atomic>

struct LlamaCppWeb : public TextCompletionI {
	ConfigModelI& _conf;

	// this mutex-locks internally
	httplib::Client _cli;

	// this is a bad idea
	static std::minstd_rand thread_local _rng;

	std::atomic<bool> _use_server_cache {false};

	LlamaCppWeb(
		ConfigModelI& conf
	);
	~LlamaCppWeb(void);

	bool isGood(void) override;
	int64_t completeSelect(const std::string_view prompt, const std::vector<std::string_view>& possible) override;
	std::string completeLine(const std::string_view prompt) override;

	// TODO: expose?
	nlohmann::json complete(const nlohmann::json& request_j);
};

