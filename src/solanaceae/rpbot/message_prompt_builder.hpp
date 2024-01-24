#pragma once

#include "./rpbot.hpp"

#include <solanaceae/contact/components.hpp>
#include <solanaceae/message3/components.hpp>

#include <entt/container/dense_map.hpp>
#include <entt/container/dense_set.hpp>

// TODO: improve caching
struct MessagePromptBuilder {
	Contact3Registry& _cr;
	const Contact3 _c;
	RegistryMessageModel& _rmm;

	// lookup table, string_view since no name-components are changed
	entt::dense_map<Contact3, std::string_view> names;


	bool buildNameLookup(void) {
		if (_cr.all_of<Contact::Components::ParentOf>(_c)) { // group rpbot
			const auto& subs = _cr.get<Contact::Components::ParentOf>(_c).subs;
			// should include self
			for (const auto sub_c : subs) {
				if (_cr.all_of<Contact::Components::Name>(sub_c)) {
					names[sub_c] = _cr.get<Contact::Components::Name>(sub_c).name;
				}
			}
		} else { // pm rpbot
			if (_cr.all_of<Contact::Components::Name>(_c)) {
				names[_c] = _cr.get<Contact::Components::Name>(_c).name;
			} else {
				std::cerr << "RPBot error: other missing name\n";
				return false;
			}

			if (_cr.all_of<Contact::Components::Self>(_c)) {
				const auto self = _cr.get<Contact::Components::Self>(_c).self;
				if (_cr.all_of<Contact::Components::Name>(self)) {
					names[self] = _cr.get<Contact::Components::Name>(self).name;
				} else {
					std::cerr << "RPBot error: self missing name\n";
					return false;
				}
			} else {
				std::cerr << "RPBot error: contact missing self\n";
				return false;
			}
		}

		return true;
	}

	std::string buildPromptMessageHistory(void) {
		auto* mr =  _rmm.get(_c);
		assert(mr);

		std::string prompt;

		auto view = mr->view<Message::Components::Timestamp>();
		for (auto view_it = view.rbegin(), view_last = view.rend(); view_it != view_last; view_it++) {
			const Message3 e = *view_it;

			// manually filter ("reverse" iteration <.<)
			// TODO: add mesagetext?
			if (!mr->all_of<Message::Components::ContactFrom, Message::Components::ContactTo>(e)) {
				continue;
			}

			//Message::Components::ContactFrom& c_from = mr->get<Message::Components::ContactFrom>(e);
			//Message::Components::ContactTo& c_to = msg_reg.get<Message::Components::ContactTo>(e);
			//Message::Components::Timestamp ts = view.get<Message::Components::Timestamp>(e);

			prompt += "\n";
			prompt += buildPromptMessage({*mr, e});
		}

		return prompt;
	}

	// gets split across lines
	std::string buildPromptMessage(const Message3Handle m) {
		if (!m.all_of<Message::Components::ContactFrom, Message::Components::MessageText>()) {
			// TODO: case for transfers
			return "";
		}

		// TODO: cache as comp
		const auto line_prefix = buildPromptMessagePrefix(m);

		// TODO: trim
		std::string message_lines = line_prefix + m.get<Message::Components::MessageText>().text;
		for (auto nlpos = message_lines.find('\n'); nlpos != std::string::npos; nlpos = message_lines.find('\n', nlpos+1)) {
			message_lines.insert(nlpos+1, line_prefix);
			nlpos += line_prefix.size();
		}

		return message_lines;
	}

	// generate prompt prefix (timestamp + name), configurable?
	std::string buildPromptMessagePrefix(const Message3Handle m) {
		const Contact3 from = m.get<Message::Components::ContactFrom>().c;
		if (names.count(from)) {
			return std::string{names[from]} + ": ";
		} else {
			return "<unk-user>: ";
		}
	}
};

