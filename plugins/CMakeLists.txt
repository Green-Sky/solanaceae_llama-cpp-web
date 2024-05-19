cmake_minimum_required(VERSION 3.14...3.24 FATAL_ERROR)

add_library(plugin_llama-cpp-web SHARED
	./plugin_llama-cpp-web.cpp
)
set_target_properties(plugin_llama-cpp-web PROPERTIES
	C_VISIBILITY_PRESET hidden
	POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(plugin_llama-cpp-web PUBLIC
	solanaceae_plugin
	solanaceae_llama-cpp-web
)

########################################

add_library(plugin_rpbot SHARED
	./plugin_rpbot.cpp
)

set_target_properties(plugin_rpbot PROPERTIES
	C_VISIBILITY_PRESET hidden
	POSITION_INDEPENDENT_CODE ON
)
target_compile_definitions(plugin_rpbot PUBLIC ENTT_API_IMPORT)

target_link_libraries(plugin_rpbot PUBLIC
	solanaceae_plugin
	solanaceae_rpbot
)

