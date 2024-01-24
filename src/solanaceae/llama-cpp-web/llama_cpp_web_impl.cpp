#include "./llama_cpp_web_impl.hpp"

#include <solanaceae/util/utils.hpp>

#include <nlohmann/json.hpp>

#include <chrono>

std::minstd_rand thread_local LlamaCppWeb::_rng{std::random_device{}()};

// TODO: variant that strips unicode?
static std::string convertToSafeGrammarString(std::string_view input) {
	std::string res;
	for (const char c : input) {
		res += "\\x";
		res += bin2hex({static_cast<uint8_t>(c)});
	}
	return res;
}

LlamaCppWeb::~LlamaCppWeb(void) {
}

bool LlamaCppWeb::isGood(void) {
	auto res = _cli.Get("/health");
	if (
		res.error() != httplib::Error::Success ||
		res->status != 200 ||
		res->body.empty() ||
		res->get_header_value("Content-Type") != "application/json"
	) {
		return false;
	}

	//std::cout << "/health code: " << res->status << " body: " << res->body << "\n";
	//std::cout << "Content-Type: " << res->get_header_value("Content-Type") << "\n";

	const auto response_body_j = nlohmann::json::parse(res->body, nullptr, false);

	const std::string status = response_body_j.value("status", std::string{"value-not-found"});
	if (status != "ok") {
		std::cerr << "status not ok: " << status << "\n";
		return false;
	}

	return true; // healthy endpoint
}

int64_t LlamaCppWeb::completeSelect(const std::string_view prompt, const std::vector<std::string_view>& possible) {
	if (possible.empty()) {
		return -1;
	}
	if (possible.size() == 1) {
		return 0;
	}

	// see
	// https://github.com/ggerganov/llama.cpp/tree/master/grammars#example
	std::string grammar {"root ::= "};
	bool first = true;
	for (const auto& it : possible) {
		if (first) {
			first = false;
		} else {
			grammar += "| ";
		}
		grammar += "\"";
		//grammar += it;
		grammar += convertToSafeGrammarString(it);
		grammar += "\" ";
	}
	//grammar += ")";

	//std::cout << "generated grammar:\n" << grammar << "\n";

	auto ret = complete(nlohmann::json{
		{"prompt", prompt},
		{"grammar", grammar},
		{"min_p", 0.1}, // model dependent
		{"repeat_penalty", 1.0}, // deactivate
		{"temperature", 0.9}, // depends 1.0 for chat models
		{"top_k", 60},
		{"top_p", 1.0}, // disable
		{"n_predict", 256}, // unlikely to ever be so high
		{"seed", _rng()},
	});

	if (ret.empty()) {
		return -2;
	}

	if (!ret.count("content")) {
		return -3;
	}

	std::string selected = ret.at("content");
	if (selected.empty()) {
		return -4;
	}

	for (int64_t i = 0; i < (int64_t)possible.size(); i++) {
		if (selected == possible[i]) {
			return i;
		}
	}

	std::cerr << "complete failed j:'" << ret.dump() << "'\n";
	return -5;
}

std::string LlamaCppWeb::completeLine(const std::string_view prompt) {
	auto ret = complete(nlohmann::json{
		{"prompt", prompt},
		{"min_p", 0.1}, // model dependent
		{"repeat_penalty", 1.0}, // deactivate
		{"temperature", 0.9}, // depends 1.0 for chat models
		{"top_k", 60},
		{"top_p", 1.0}, // disable
		{"n_predict", 1000},
		{"seed", _rng()},
		{"stop", {"\n"}},
	});

	return ret.dump();
}

nlohmann::json LlamaCppWeb::complete(const nlohmann::json& request_j) {
	// TODO: dont check ourself
	if (!isGood()) {
		return {};
	}

	// completions can take very long
	// steaming instead would be better
	_cli.set_read_timeout(std::chrono::minutes(10));

	//std::cout << "j dump: '" << request_j.dump(-1, ' ', true) << "'\n";

	auto res = _cli.Post("/completion", request_j.dump(-1, ' ', true), "application/json");

	//std::cerr << "res.error():" << res.error() << "\n";

	if (
		res.error() != httplib::Error::Success ||
		res->status != 200
		//res->body.empty() ||
		//res->get_header_value("Content-Type") != "application/json"
	) {
		std::cerr << "error posting\n";
		return {};
	}

	return nlohmann::json::parse(res->body, nullptr, false);
}

