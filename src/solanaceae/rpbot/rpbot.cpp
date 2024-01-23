#include "./rpbot.hpp"

RPBot::RPBot(
	TextCompletionI& completion,
	ConfigModelI& conf
) : _completion(completion), _conf(conf) {
}

float RPBot::tick(float time_delta) {
	return 10.f;
}

