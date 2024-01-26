#include <solanaceae/plugin/solana_plugin_v1.h>

#include <solanaceae/llama-cpp-web/llama_cpp_web_impl.hpp>

#include <memory>
#include <iostream>
#include <limits>

static std::unique_ptr<LlamaCppWeb> g_lcw = nullptr;

constexpr const char* plugin_name = "llama-cpp-web";

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
		auto* conf = PLUG_RESOLVE_INSTANCE(ConfigModelI);

		// static store, could be anywhere tho
		// construct with fetched dependencies
		g_lcw = std::make_unique<LlamaCppWeb>(*conf);

		// register types
		PLUG_PROVIDE_INSTANCE(LlamaCppWeb, plugin_name, g_lcw.get());
		PLUG_PROVIDE_INSTANCE(TextCompletionI, plugin_name, g_lcw.get());
	} catch (const ResolveException& e) {
		std::cerr << "PLUGIN " << plugin_name << " " << e.what << "\n";
		return 2;
	}

	return 0;
}

SOLANA_PLUGIN_EXPORT void solana_plugin_stop(void) {
	std::cout << "PLUGIN " << plugin_name << " STOP()\n";

	g_lcw.reset();
}

SOLANA_PLUGIN_EXPORT float solana_plugin_tick(float delta) {
	(void)delta;
	//g_ircc->iterate(); // TODO: return interval, respect dcc etc

	return std::numeric_limits<float>::max();
}

} // extern C

