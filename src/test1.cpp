#include <solanaceae/llama-cpp-web/llama_cpp_web_impl.hpp>

#include <nlohmann/json.hpp>

#include <iostream>
#include <random>
#include <vector>
#include <chrono>
#include <cstdint>

int main(void) {
	LlamaCppWeb lcw;

	if (!lcw.isGood()) {
		std::cerr << lcw._cli.host() << " " << lcw._cli.port() << " endpoint not healthy\n";
		return 1;
	}
	std::cerr << lcw._cli.host() << " " << lcw._cli.port() << " endpoint healthy\n";

	std::cout << "The meaning of life is to"
		<< lcw.complete(nlohmann::json{
			{"prompt", "The meaning of life is to"},
			{"min_p", 0.1}, // model dependent
			{"repeat_penalty", 1.0}, // deactivate
			{"temperature", 0.9}, // depends 1.0 for chat models
			{"top_k", 60},
			{"top_p", 1.0}, // disable
			{"n_predict", 16},
			{"stop", {".", "\n"}},
			{"gramar", ""}
		})
		<< "\n";

	std::cout << "-------------------------\n";

	std::cout << "complete from select:\n";
	std::vector<std::string_view> possible {
		" die",
		" die.",
		" live",
		" love",
		" excersize",
		" Hi",
	};
	for (size_t i = 0; i < 10; i++) {
		std::cout << "The meaning of life is to";
		auto res = lcw.completeSelect("The meaning of life is to", possible);
		if (res < 0) {
			std::cout << " error--\n";
		} else {
			std::cout << possible[res] << "\n";
		}
	}

	return 0;
}
