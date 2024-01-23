#pragma once

#include <solanaceae/util/config_model.hpp>
#include <solanaceae/llama-cpp-web/text_completion_interface.hpp>

struct RPBot {
	TextCompletionI& _completion;
	ConfigModelI& _conf;

	public:
		RPBot(
			TextCompletionI& completion,
			ConfigModelI& conf
		);

		float tick(float time_delta);
};

