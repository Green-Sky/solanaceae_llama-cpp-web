#include <solanaceae/plugin/solana_plugin_v1.h>

#include <solanaceae/util/config_model.hpp>
#include <solanaceae/contact/contact_store_i.hpp>
#include <solanaceae/llama-cpp-web/text_completion_interface.hpp>
#include <solanaceae/rpbot/rpbot.hpp>
#include <solanaceae/message3/message_command_dispatcher.hpp>

#include <entt/entt.hpp>
#include <entt/fwd.hpp>

#include <memory>
#include <iostream>
#include <limits>

static std::unique_ptr<RPBot> g_rpbot = nullptr;

constexpr const char* plugin_name = "RPBot";

extern "C" {

SOLANA_PLUGIN_EXPORT const char* solana_plugin_get_name(void) {
	return plugin_name;
}

SOLANA_PLUGIN_EXPORT uint32_t solana_plugin_get_version(void) {
	return SOLANA_PLUGIN_VERSION;
}

SOLANA_PLUGIN_EXPORT uint32_t solana_plugin_start(struct SolanaAPI* solana_api) {
	std::cout << "PLUGIN " << plugin_name << " START()\n";

	if (solana_api == nullptr) {
		return 1;
	}

	try {
		auto* completion = PLUG_RESOLVE_INSTANCE(TextCompletionI);
		auto* conf = PLUG_RESOLVE_INSTANCE(ConfigModelI);
		auto* cs = PLUG_RESOLVE_INSTANCE(ContactStore4I);
		auto* rmm = PLUG_RESOLVE_INSTANCE(RegistryMessageModelI);
		auto* mcd = PLUG_RESOLVE_INSTANCE(MessageCommandDispatcher);

		// static store, could be anywhere tho
		// construct with fetched dependencies
		g_rpbot = std::make_unique<RPBot>(*completion, *conf, *cs, *rmm, mcd);

		// register types
		PLUG_PROVIDE_INSTANCE(RPBot, plugin_name, g_rpbot.get());
	} catch (const ResolveException& e) {
		std::cerr << "PLUGIN " << plugin_name << " " << e.what << "\n";
		return 2;
	}

	return 0;
}

SOLANA_PLUGIN_EXPORT void solana_plugin_stop(void) {
	std::cout << "PLUGIN " << plugin_name << " STOP()\n";

	g_rpbot.reset();
}

SOLANA_PLUGIN_EXPORT float solana_plugin_tick(float delta) {
	return g_rpbot->tick(delta);
}

} // extern C

