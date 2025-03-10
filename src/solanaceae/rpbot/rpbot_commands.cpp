#include "./rpbot.hpp"

#include "./rpbot_states.hpp"

#include <solanaceae/contact/components.hpp>
#include <solanaceae/message3/components.hpp>
#include <solanaceae/util/utils.hpp>

void RPBot::registerCommands(void) {
	if (_mcd == nullptr) {
		return;
	}

	_mcd->registerCommand(
		"RPBot", "rpbot",
		"start",
		[this](std::string_view params, Message3Handle m) -> bool {
			const auto contact_from = m.get<Message::Components::ContactFrom>().c;
			const auto contact_to = m.get<Message::Components::ContactTo>().c;

			auto& cr = _cs.registry();

			if (params.empty()) {
				// contact_to should be the contact this is for
				if (cr.any_of<StateIdle, StateGenerateMsg, StateNextActor, StateTimingCheck>(contact_to)) {
					_rmm.sendText(
						contact_from,
						"error: already running"
					);
					return true;
				}
				if (cr.any_of<StateIdle, StateGenerateMsg, StateNextActor, StateTimingCheck>(contact_from)) {
					_rmm.sendText(
						contact_from,
						"error: already running"
					);
					return true;
				}

				if (cr.all_of<Contact::Components::ParentOf>(contact_to)) {
					// group
					auto& new_state = cr.emplace<StateIdle>(contact_to);
					new_state.timeout = 10.f;
				} else {
					// pm
					auto& new_state = cr.emplace<StateIdle>(contact_from);
					new_state.timeout = 10.f;
				}

				_rmm.sendText(
					contact_from,
					"RPBot started"
				);
				return true;
			} else {
				// id in params
				if (params.size() % 2 != 0) {
					_rmm.sendText(
						contact_from,
						"malformed hex id"
					);
					return true;
				}

				auto id_bin = hex2bin(params);

				auto view = cr.view<Contact::Components::ID>();
				for (auto it = view.begin(), it_end = view.end(); it != it_end; it++) {
					if (view.get<Contact::Components::ID>(*it).data == id_bin) {
						if (cr.any_of<StateIdle, StateNextActor, StateGenerateMsg, StateTimingCheck>(*it)) {
							_rmm.sendText(
								contact_from,
								"RPBot already running"
							);
							return true;
						}

						auto& new_state = cr.emplace<StateIdle>(*it);
						new_state.timeout = 10.f;

						_rmm.sendText(
							contact_from,
							"RPBot started"
						);
						return true;
					}
				}

				_rmm.sendText(
					contact_from,
					"no contact found for id"
				);
				return true;
			}
		},
		"Start RPBot in current contact.",
		MessageCommandDispatcher::Perms::ADMIN // TODO: should proably be MODERATOR
	);

	_mcd->registerCommand(
		"RPBot", "rpbot",
		"stop",
		[this](std::string_view params, Message3Handle m) -> bool {
			const auto contact_from = m.get<Message::Components::ContactFrom>().c;
			const auto contact_to = m.get<Message::Components::ContactTo>().c;

			auto& cr = _cs.registry();

			if (params.empty()) {
				// contact_to should be the contact this is for
				if (cr.any_of<StateIdle, StateGenerateMsg, StateNextActor, StateTimingCheck>(contact_to)) {
					cr.emplace_or_replace<TagStopRPBot>(contact_to);
					_rmm.sendText(
						contact_from,
						"stopped"
					);
					return true;
				}
				if (cr.any_of<StateIdle, StateGenerateMsg, StateNextActor, StateTimingCheck>(contact_from)) {
					cr.emplace_or_replace<TagStopRPBot>(contact_from);
					_rmm.sendText(
						contact_from,
						"stopped"
					);
					return true;
				}

				_rmm.sendText(
					contact_from,
					"error: not running"
				);
				return true;
			} else {
				// id in params
				if (params.size() % 2 != 0) {
					_rmm.sendText(
						contact_from,
						"malformed hex id"
					);
					return true;
				}

				auto id_bin = hex2bin(params);

				auto view = cr.view<Contact::Components::ID>();
				for (auto it = view.begin(), it_end = view.end(); it != it_end; it++) {
					if (view.get<Contact::Components::ID>(*it).data == id_bin) {
						if (cr.any_of<StateIdle, StateGenerateMsg, StateNextActor, StateTimingCheck>(*it)) {
							cr.emplace_or_replace<TagStopRPBot>(*it);
							_rmm.sendText(
								contact_from,
								"stopped"
							);
							return true;
						} else {
							_rmm.sendText(
								contact_from,
								"error: not running"
							);
							return true;
						}
					}
				}

				_rmm.sendText(
					contact_from,
					"no contact found for id"
				);
				return true;
			}
		},
		"Stop RPBot in current or id contact.",
		MessageCommandDispatcher::Perms::ADMIN // TODO: should proably be MODERATOR
	);

	_mcd->registerCommand(
		"RPBot", "rpbot",
		"force",
		[this](std::string_view params, Message3Handle m) -> bool {
			const auto contact_from = m.get<Message::Components::ContactFrom>().c;
			const auto contact_to = m.get<Message::Components::ContactTo>().c;

			auto& cr = _cs.registry();

			if (params.empty()) {
				// contact_to should be the contact this is for
				if (cr.any_of<StateIdle, StateGenerateMsg, StateNextActor, StateTimingCheck>(contact_to)) {
					if (cr.all_of<StateIdle>(contact_to)) {
						cr.get<StateIdle>(contact_to).force = true;
						cr.get<StateIdle>(contact_to).timeout = 2.f;
						_rmm.sendText(
							contact_from,
							"forced its hand"
						);
					}
					return true;
				}
				if (cr.any_of<StateIdle, StateGenerateMsg, StateNextActor, StateTimingCheck>(contact_from)) {
					if (cr.all_of<StateIdle>(contact_from)) {
						cr.get<StateIdle>(contact_from).force = true;
						cr.get<StateIdle>(contact_from).timeout = 2.f;
						_rmm.sendText(
							contact_from,
							"forced its hand"
						);
					}
					return true;
				}

				_rmm.sendText(
					contact_from,
					"error: not running"
				);
				return true;
			} else {
				// id in params
				if (params.size() % 2 != 0) {
					_rmm.sendText(
						contact_from,
						"malformed hex id"
					);
					return true;
				}

				auto id_bin = hex2bin(params);

				auto view = cr.view<Contact::Components::ID>();
				for (auto it = view.begin(), it_end = view.end(); it != it_end; it++) {
					if (view.get<Contact::Components::ID>(*it).data == id_bin) {
						if (cr.any_of<StateIdle, StateGenerateMsg, StateNextActor, StateTimingCheck>(*it)) {
							if (cr.all_of<StateIdle>(*it)) {
								cr.get<StateIdle>(*it).force = true;
								cr.get<StateIdle>(*it).timeout = 2.f;
								_rmm.sendText(
									contact_from,
									"forced its hand"
								);
							}
							return true;
						} else {
							_rmm.sendText(
								contact_from,
								"error: not running"
							);
							return true;
						}
					}
				}

				_rmm.sendText(
					contact_from,
					"no contact found for id"
				);
				return true;
			}
		},
		"force it to generate a message",
		MessageCommandDispatcher::Perms::ADMIN // TODO: should proably be MODERATOR
	);

	std::cout << "RPBot: registered commands\n";
}

