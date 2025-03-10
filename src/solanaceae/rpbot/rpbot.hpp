#pragma once

#include <solanaceae/util/config_model.hpp>
#include <solanaceae/llama-cpp-web/text_completion_interface.hpp>
#include <solanaceae/contact/contact_store_i.hpp>
#include <solanaceae/message3/registry_message_model.hpp>
#include <solanaceae/message3/message_command_dispatcher.hpp>

#include <random>
#include <string>
#include <iostream>

// fwd
struct StateIdle;
struct StateNextActor;
struct StateGenerateMsg;
struct StateTimingCheck;

struct RPBot : public RegistryMessageModelEventI {
	TextCompletionI& _completion;
	ConfigModelI& _conf;
	ContactStore4I& _cs;
	RegistryMessageModelI& _rmm;
	RegistryMessageModelI::SubscriptionReference _rmm_sr;
	MessageCommandDispatcher* _mcd;

	std::minstd_rand _rng{std::random_device{}()};

	public:
		RPBot(
			TextCompletionI& completion,
			ConfigModelI& conf,
			ContactStore4I& cs,
			RegistryMessageModelI& rmm,
			MessageCommandDispatcher* mcd
		);

		float tick(float time_delta);

		void registerCommands(void);

	protected: // state transitions
		// all transitions need to be explicitly declared
		template<typename To, typename From>
		void stateTransition(const Contact4 c, const From& from, To& to) = delete;

		// reg helper
		template<typename To, typename From>
		To& emplaceStateTransition(ContactStore4I& cs, Contact4 c, const From& state) {
			std::cout << "RPBot: transition from " << From::name << " to " << To::name << "\n";
			To& to = cs.registry().emplace_or_replace<To>(c);
			stateTransition<To>(c, state, to);
			return to;
		}

	protected: // systems
		float doAllIdle(float time_delta);
		float doAllNext(float time_delta);
		float doAllGenerateMsg(float time_delta);
		float doAllTimingCheck(float time_delta);

	protected: // onMsg
		bool onEvent(const Message::Events::MessageConstruct& e) override;
};

