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
void RPBot::stateTransition(const Contact4 c, const StateIdle& from, StateNextActor& to) {
	// collect promp

	MessagePromptBuilder mpb{_cs, c, _rmm, {}};
	mpb.buildNameLookup();

	const auto& cr = _cs.registry();

	int64_t self {-1};
	{ // get set of possible usernames (even if forced, just to make sure)
		// copy mpb.names (contains string views, needs copies)
		for (const auto& [name_c, name] : mpb.names) {
			if (cr.all_of<Contact::Components::TagSelfStrong>(name_c)) {
				self = to.possible_contacts.size();
				to.possible_names.push_back(std::string{name});
				to.possible_contacts.push_back(name_c);
			} else if (cr.all_of<Contact::Components::ConnectionState>(name_c)) {
				if (cr.get<Contact::Components::ConnectionState>(name_c).state != Contact::Components::ConnectionState::disconnected) {
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

	if (mpb.names.size() < 2) {
		// early exit for too small groups
		to.future = std::async(std::launch::async, [self]() -> int64_t { return -10; });
		return;
	}

	{ // - system promp (needs self name etc)
		if (const auto* id_comp = cr.try_get<Contact::Components::ID>(c); id_comp != nullptr) {
			const auto id_hex = bin2hex(id_comp->data);
			to.prompt = _conf.get_string("RPBot", "system_prompt", id_hex).value();
		} else {
			to.prompt = _conf.get_string("RPBot", "system_prompt").value();
		}

		std::string online_users;
		for (const auto& [_, name] : mpb.names) {
			if (!online_users.empty()) {
				online_users += ", ";
			}
			online_users += name;
		}

		to.prompt = fmt::format(fmt::runtime(to.prompt),
			fmt::arg("self_name", to.possible_names.at(self)),
			//fmt::arg("chat_name", "test_group"),
			//fmt::arg("chat_type", "Group")
			//fmt::arg("chat_topic", "Group")
			fmt::arg("online_users", online_users)
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
void RPBot::stateTransition(const Contact4, const StateNextActor&, StateIdle& to) {
	to.timeout = std::uniform_real_distribution<>{30.f, 5.f*60.f}(_rng);
}

template<>
void RPBot::stateTransition(const Contact4 c, const StateNextActor& from, StateGenerateMsg& to) {
	const auto& cr = _cs.registry();
	to.prompt = from.prompt; // TODO: move from?
	assert(cr.all_of<Contact::Components::Self>(c));
	const Contact4 self = cr.get<Contact::Components::Self>(c).self;

	to.prompt += cr.get<Contact::Components::Name>(self).name + ":"; // TODO: remove space

	{ // launch async
		to.future = std::async(std::launch::async, [&to, this]() -> std::string {
			return _completion.completeLine(to.prompt);
		});
	}
}

template<>
void RPBot::stateTransition(const Contact4, const StateGenerateMsg&, StateIdle& to) {
	// relativly slow delay for multi line messages
	to.timeout = std::uniform_real_distribution<>{2.f, 15.f}(_rng);
}

RPBot::RPBot(
	TextCompletionI& completion,
	ConfigModelI& conf,
	ContactStore4I& cs,
	RegistryMessageModelI& rmm,
	MessageCommandDispatcher* mcd
) : _completion(completion), _conf(conf), _cs(cs), _rmm(rmm), _rmm_sr(_rmm.newSubRef(this)), _mcd(mcd) {
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

	_rmm_sr.subscribe(RegistryMessageModel_Event::message_construct);
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
	auto& cr = _cs.registry();
	float min_tick_interval = std::numeric_limits<float>::max();
	std::vector<Contact4> to_remove_stateidle;
	auto view = cr.view<StateIdle>();

	view.each([this, &cr, time_delta, &to_remove_stateidle, &min_tick_interval](const Contact4 c, StateIdle& state) {
		if (cr.all_of<TagStopRPBot>(c)) {
			// marked for deletion, in idle (here) we remove them without adding next state
			to_remove_stateidle.push_back(c);
			cr.remove<TagStopRPBot>(c);
			return;
		}

		state.timeout -= time_delta;
		if (state.timeout > 0.f) {
			return;
		}
		std::cout << "RPBot: idle timed out\n";

		// TODO: use multi-shot-prompt and better system promp to start immediatly
		auto* mreg = _rmm.get(c);
		if (mreg != nullptr) {
			// TODO: per id min_messages
			if (mreg->view<Message::Components::MessageText>().size() >= _conf.get_int("RPBot", "min_messages").value_or(4)) {
				// maximum amount of messages the bot can send, before someone else needs to send a message
				// TODO: per id max_cont_messages
				const size_t max_cont_messages = _conf.get_int("RPBot", "max_cont_messages").value_or(4);
				auto tmp_view = mreg->view<Message::Components::Timestamp, Message::Components::MessageText, Message::Components::ContactFrom>();
				tmp_view.use<Message::Components::Timestamp>();
				bool other_sender {false};
				auto view_it = tmp_view.begin(), view_last = tmp_view.end();
				for (size_t i = 0; i < max_cont_messages && view_it != view_last; view_it++, i++) {
					// TODO: also test for weak self?
					if (!cr.any_of<Contact::Components::TagSelfStrong>(tmp_view.get<Message::Components::ContactFrom>(*view_it).c)) {
						other_sender = true;
						break;
					}
				}

				if (other_sender) {
					to_remove_stateidle.push_back(c);
					min_tick_interval = 0.1f;

					// transition to Next
					emplaceStateTransition<StateNextActor>(_cs, c, state);
					return;
				}
			}
		}

		// if not handled yet
		// check-in in another 15-45s
		state.timeout = std::uniform_real_distribution<>{15.f, 45.f}(_rng);
		std::cout << "RPBot: not ready yet, back to ideling\n";
		if (mreg == nullptr) {
			std::cout << "mreg is null\n";
		} else {
			std::cout << "size(): " << mreg->view<Message::Components::MessageText>().size() << "\n";
		}
	});

	cr.remove<StateIdle>(to_remove_stateidle.cbegin(), to_remove_stateidle.cend());
	return min_tick_interval;
}

float RPBot::doAllNext(float) {
	auto& cr = _cs.registry();
	float min_tick_interval = std::numeric_limits<float>::max();
	std::vector<Contact4> to_remove;
	auto view = cr.view<StateNextActor>();

	view.each([this, &cr, &to_remove, &min_tick_interval](const Contact4 c, StateNextActor& state) {
		// TODO: how to timeout?
		if (state.future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
			to_remove.push_back(c);
			min_tick_interval = 0.1f;
			std::cout << "RPBot: next compute done!\n";

			const auto selected = state.future.get();
			if (selected >= 0 && size_t(selected) < state.possible_names.size()) {
				std::cout << "next is " << state.possible_names.at(selected) << "(" << selected << ")\n";
				if (cr.all_of<Contact::Components::TagSelfStrong>(state.possible_contacts.at(selected))) {
					// we predicted ourselfs
					emplaceStateTransition<StateGenerateMsg>(_cs, c, state);
					return;
				}
			} else {
				std::cerr << "RPBot error: next was negative or too large (how?) " << selected << "\n";
			}

			// transition to Idle
			emplaceStateTransition<StateIdle>(_cs, c, state);
		}
	});

	cr.remove<StateNextActor>(to_remove.cbegin(), to_remove.cend());
	return min_tick_interval;
}

float RPBot::doAllGenerateMsg(float) {
	auto& cr = _cs.registry();
	float min_tick_interval = std::numeric_limits<float>::max();
	std::vector<Contact4> to_remove;
	auto view = cr.view<StateGenerateMsg>();

	cr.remove<StateGenerateMsg>(to_remove.cbegin(), to_remove.cend());
	view.each([this, &to_remove, &min_tick_interval](const Contact4 c, StateGenerateMsg& state) {
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
			emplaceStateTransition<StateIdle>(_cs, c, state);
		}
	});

	cr.remove<StateGenerateMsg>(to_remove.cbegin(), to_remove.cend());
	return min_tick_interval;
}

float RPBot::doAllTimingCheck(float time_delta) {
	float min_tick_interval = std::numeric_limits<float>::max();
	return min_tick_interval;
}

bool RPBot::onEvent(const Message::Events::MessageConstruct& e) {
	if (!e.e.all_of<Message::Components::ContactFrom, Message::Components::ContactTo>()) {
		return false;
	}

	auto& cr = _cs.registry();

	const auto contact_to = e.e.get<Message::Components::ContactTo>().c;
	const auto contact_from = e.e.get<Message::Components::ContactFrom>().c;

	if (!cr.valid(contact_to) || !cr.valid(contact_from)) {
		std::cerr << "RPBot error: invalid contact in message\n";
		return false;
	}

	if (cr.any_of<Contact::Components::TagSelfStrong>(contact_from)) {
		return false; // ignore own messages
	}

	Contact4 rpbot_contact = entt::null;

	// check ContactTo (public)
	// check ContactTo parent (group private)
	// check ContactFrom (private)

	if (cr.any_of<StateIdle, StateNextActor, StateGenerateMsg, StateTimingCheck>(contact_to)) {
		rpbot_contact = contact_to;
	} else if (cr.all_of<Contact::Components::Parent>(contact_to)) {
		rpbot_contact = cr.get<Contact::Components::Parent>(contact_to).parent;
	} else if (cr.any_of<StateIdle, StateNextActor, StateGenerateMsg, StateTimingCheck>(contact_from)) {
		rpbot_contact = contact_from;
	} else {
		return false; // not a rpbot related message
	}

	if (!cr.all_of<StateIdle>(rpbot_contact)) {
		return false; // not idle
	}

	auto& timeout = cr.get<StateIdle>(rpbot_contact).timeout;
	// TODO: config with id
	timeout = std::clamp<float>(
		timeout,
		1.5f, // minimum, helps when activly history syncing
		_conf.get_double("RPBot", "max_interactive_delay").value_or(4.f)
	);
	std::cout << "RPBot: onMsg new timeout: " << timeout << "\n";

	return false;
}

