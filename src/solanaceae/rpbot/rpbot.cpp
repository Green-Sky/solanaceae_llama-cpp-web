#include "./rpbot.hpp"

#include "./message_prompt_builder.hpp"

#include "./rpbot_states.hpp"

#include <solanaceae/contact/components.hpp>
#include <solanaceae/message3/components.hpp>
#include <solanaceae/util/utils.hpp>

#include <fmt/core.h>

#include <limits>
#include <string_view>
#include <vector>
#include <future>
#include <chrono>
#include <cstdint>

template<>
void RPBot::stateTransition(const Contact3 c, const StateIdle& from, StateNext& to) {
	// collect promp

	MessagePromptBuilder mpb{_cr, c, _rmm, {}};
	mpb.buildNameLookup();

	int64_t self {-1};
	{ // get set of possible usernames (even if forced, just to make sure)
		// copy mpb.names (contains string views, needs copies)
		for (const auto& [name_c, name] : mpb.names) {
			if (_cr.all_of<Contact::Components::TagSelfStrong>(name_c)) {
				self = to.possible_contacts.size();
				to.possible_names.push_back(std::string{name});
				to.possible_contacts.push_back(name_c);
			} else if (_cr.all_of<Contact::Components::ConnectionState>(name_c)) {
				if (_cr.get<Contact::Components::ConnectionState>(name_c).state != Contact::Components::ConnectionState::disconnected) {
					// online
					to.possible_names.push_back(std::string{name});
					to.possible_contacts.push_back(name_c);
				}
			}
		}
	}

	if (self < 0) {
		// early exit for invalid self
		to.future = std::async(std::launch::async, [self]() -> int64_t { return self; });
		return;
	}

	{ // - system promp (needs self name etc)
		if (const auto* id_comp = _cr.try_get<Contact::Components::ID>(c); id_comp != nullptr) {
			const auto id_hex = bin2hex(id_comp->data);
			to.prompt = _conf.get_string("RPBot", "system_prompt", id_hex).value();
		} else {
			to.prompt = _conf.get_string("RPBot", "system_prompt").value();
		}

		to.prompt = fmt::format(fmt::runtime(to.prompt),
			fmt::arg("self_name", to.possible_names.at(self))
			//fmt::arg("chat_name", "test_group"),
			//fmt::arg("chat_type", "Group")
			//fmt::arg("chat_topic", "Group")
			// current online?
			// current date?
		);
	}

	// - message history
	to.prompt += mpb.buildPromptMessageHistory();

	{ // - next needs the beginning of the new message
		// empty rn
		to.prompt += "\n";
	}

	std::cout << "prompt for next: '" << to.prompt << "'\n";

	if (!from.force) { // launch async
		// copy names for string view param (lol)
		std::vector<std::string_view> pnames;
		for (const auto& n : to.possible_names) {
			pnames.push_back(n);
		}

		to.future = std::async(std::launch::async, [pnames, &to, this]() -> int64_t {
			return _completion.completeSelect(to.prompt, pnames);
		});
	} else {
		std::cout << "next forced with self " << self << "\n";
		// forced, we predict ourselfs
		// TODO: set without future?
		to.future = std::async(std::launch::async, [self]() -> int64_t { return self; });
	}
}

template<>
void RPBot::stateTransition(const Contact3, const StateNext&, StateIdle& to) {
	to.timeout = std::uniform_real_distribution<>{10.f, 45.f}(_rng);
}

template<>
void RPBot::stateTransition(const Contact3 c, const StateNext& from, StateGenerateMsg& to) {
	to.prompt = from.prompt; // TODO: move from?
	assert(_cr.all_of<Contact::Components::Self>(c));
	const Contact3 self = _cr.get<Contact::Components::Self>(c).self;

	to.prompt += _cr.get<Contact::Components::Name>(self).name + ":"; // TODO: remove space

	{ // launch async
		to.future = std::async(std::launch::async, [&to, this]() -> std::string {
			return _completion.completeLine(to.prompt);
		});
	}
}

template<>
void RPBot::stateTransition(const Contact3, const StateGenerateMsg&, StateIdle& to) {
	// relativly slow delay for multi line messages
	to.timeout = std::uniform_real_distribution<>{2.f, 15.f}(_rng);
}

RPBot::RPBot(
	TextCompletionI& completion,
	ConfigModelI& conf,
	Contact3Registry& cr,
	RegistryMessageModel& rmm,
	MessageCommandDispatcher* mcd
) : _completion(completion), _conf(conf), _cr(cr), _rmm(rmm), _mcd(mcd) {
	//system_prompt = R"sys(Transcript of a group chat, where Bob talks to online strangers.
//)sys";

	// set default system prompt
	if (!_conf.has_string("RPBot", "system_prompt")) {
		_conf.set("RPBot", "system_prompt", std::string_view{
R"sys(Transcript of a group chat, where {self_name} talks to online strangers.
{self_name} is creative and curious. {self_name} is writing with precision, but also with occasional typos.
)sys"
		});
	}

	assert(_conf.has_string("RPBot", "system_prompt"));

	registerCommands();
}

float RPBot::tick(float time_delta) {
	float min_tick_interval = std::numeric_limits<float>::max();

	min_tick_interval = std::min(min_tick_interval, doAllIdle(time_delta));
	min_tick_interval = std::min(min_tick_interval, doAllNext(time_delta));
	min_tick_interval = std::min(min_tick_interval, doAllGenerateMsg(time_delta));
	min_tick_interval = std::min(min_tick_interval, doAllTimingCheck(time_delta));

	return min_tick_interval;
}

float RPBot::doAllIdle(float time_delta) {
	float min_tick_interval = std::numeric_limits<float>::max();
	std::vector<Contact3> to_remove_stateidle;
	auto view = _cr.view<StateIdle>();

	view.each([this, time_delta, &to_remove_stateidle, &min_tick_interval](const Contact3 c, StateIdle& state) {
		if (_cr.all_of<TagStopRPBot>(c)) {
			// marked for deletion, in idle (here) we remove them without adding next state
			to_remove_stateidle.push_back(c);
			_cr.remove<TagStopRPBot>(c);
			return;
		}

		state.timeout -= time_delta;
		if (state.timeout <= 0.f) {
			std::cout << "RPBot: idle timed out\n";
			// TODO: use multiprompt and better system promp to start immediatly
			if (auto* mreg = _rmm.get(c); mreg != nullptr && mreg->view<Message::Components::MessageText>().size() >= 4) {
				to_remove_stateidle.push_back(c);
				min_tick_interval = 0.1f;

				// transition to Next
				emplaceStateTransition<StateNext>(_cr, c, state);
			} else {
				// check-in in another 15-45s
				state.timeout = std::uniform_real_distribution<>{15.f, 45.f}(_rng);
				std::cout << "RPBot: not ready yet, back to ideling\n";
				if (mreg == nullptr) {
					std::cout << "mreg is null\n";
				} else {
					std::cout << "size(): " << mreg->view<Message::Components::MessageText>().size() << "\n";
				}
			}
		}
	});

	_cr.remove<StateIdle>(to_remove_stateidle.cbegin(), to_remove_stateidle.cend());
	return min_tick_interval;
}

float RPBot::doAllNext(float) {
	float min_tick_interval = std::numeric_limits<float>::max();
	std::vector<Contact3> to_remove;
	auto view = _cr.view<StateNext>();

	view.each([this, &to_remove, &min_tick_interval](const Contact3 c, StateNext& state) {
		// TODO: how to timeout?
		if (state.future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
			to_remove.push_back(c);
			min_tick_interval = 0.1f;
			std::cout << "RPBot: next compute done!\n";

			const auto selected = state.future.get();
			if (selected >= 0 && size_t(selected) < state.possible_names.size()) {
				std::cout << "next is " << state.possible_names.at(selected) << "(" << selected << ")\n";
				if (_cr.all_of<Contact::Components::TagSelfStrong>(state.possible_contacts.at(selected))) {
					// we predicted ourselfs
					emplaceStateTransition<StateGenerateMsg>(_cr, c, state);
					return;
				}
			} else {
				std::cerr << "RPBot error: next was negative or too large (how?) " << selected << "\n";
			}

			// transition to Idle
			emplaceStateTransition<StateIdle>(_cr, c, state);
		}
	});

	_cr.remove<StateNext>(to_remove.cbegin(), to_remove.cend());
	return min_tick_interval;
}

float RPBot::doAllGenerateMsg(float) {
	float min_tick_interval = std::numeric_limits<float>::max();
	std::vector<Contact3> to_remove;
	auto view = _cr.view<StateGenerateMsg>();

	_cr.remove<StateGenerateMsg>(to_remove.cbegin(), to_remove.cend());
	view.each([this, &to_remove, &min_tick_interval](const Contact3 c, StateGenerateMsg& state) {
		// TODO: how to timeout?
		if (state.future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
			to_remove.push_back(c);
			min_tick_interval = 0.1f;
			std::cout << "RPBot: generatemessage compute done!\n";

			std::string msg = state.future.get();
			if (!msg.empty()) {
				std::string::size_type start_pos = 0;
				if (msg.front() == ' ') {
					start_pos += 1;
				}
				_rmm.sendText(c, std::string_view{msg}.substr(start_pos));
			}

			// TODO: timing check?
			// transition to Idle
			emplaceStateTransition<StateIdle>(_cr, c, state);
		}
	});

	_cr.remove<StateGenerateMsg>(to_remove.cbegin(), to_remove.cend());
	return min_tick_interval;
}

float RPBot::doAllTimingCheck(float time_delta) {
	float min_tick_interval = std::numeric_limits<float>::max();
	return min_tick_interval;
}

