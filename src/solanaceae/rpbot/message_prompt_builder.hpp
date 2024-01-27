#pragma once

#include <solanaceae/util/config_model.hpp>
#include <solanaceae/contact/contact_model3.hpp>
#include <solanaceae/message3/registry_message_model.hpp>

#include <entt/container/dense_map.hpp>

// TODO: improve caching
struct MessagePromptBuilder {
	Contact3Registry& _cr;
	const Contact3 _c;
	RegistryMessageModel& _rmm;

	// lookup table, string_view since no name-components are changed
	entt::dense_map<Contact3, std::string_view> names;

	bool buildNameLookup(void);

	std::string buildPromptMessageHistory(void);

	// gets split across lines
	std::string buildPromptMessage(const Message3Handle m);

	// generate prompt prefix (just "name:")
	std::string promptMessagePrefixSimple(const Message3Handle m);

	// with both contacts (eg: "Name1 to Name2:"; or "Name1 to Everyone:"
	// this maps to private messages, might need fewshot of private or explainer system prompt
	std::string promptMessagePrefixDirected(const Message3Handle m);
};

