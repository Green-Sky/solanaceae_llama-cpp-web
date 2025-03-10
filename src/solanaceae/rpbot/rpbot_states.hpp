#pragma once

#include "./rpbot.hpp"

#include <string>
#include <vector>
#include <future>
#include <cstdint>

struct TagStopRPBot {};

// sleeps until onMsg or onTimer
struct StateIdle {
	static constexpr const char* name {"StateIdle"};
	float timeout {0.f};
	bool force {false};
};

// determines if self should generate a message
struct StateNextActor {
	static constexpr const char* name {"StateNextActor"};

	std::string prompt;
	std::vector<std::string> possible_names;
	std::vector<Contact4> possible_contacts;

	std::future<int64_t> future;
};

// generate message
struct StateGenerateMsg {
	static constexpr const char* name {"StateGenerateMsg"};

	std::string prompt;

	// returns new line (single message)
	std::future<std::string> future;
};

// look if it took too long/too many new messages came in
// while also optionally sleeping to make message appear not too fast
// HACK: skip, just send for now
struct StateTimingCheck {
	static constexpr const char* name {"StateTimingCheck"};
	int tmp;
};

