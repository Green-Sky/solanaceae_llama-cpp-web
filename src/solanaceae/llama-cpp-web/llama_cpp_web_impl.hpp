#pragma once

#include "./llama_cpp_web_interface.hpp"

#include <httplib.h>
#include <nlohmann/json_fwd.hpp>

#include <random>

struct LlamaCppWeb : public LlamaCppWebI {
	httplib::Client _cli{"http://localhost:8080"};
	std::minstd_rand _rng{std::random_device{}()};

	~LlamaCppWeb(void);

	bool isHealthy(void) override;
	int64_t completeSelect(const std::string_view prompt, const std::vector<std::string_view>& possible) override;
	std::string completeLine(const std::string_view prompt) override;

	// TODO: expose?
	nlohmann::json complete(const nlohmann::json& request_j);
};

