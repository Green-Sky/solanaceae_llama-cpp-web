cmake_minimum_required(VERSION 3.14...3.24 FATAL_ERROR)

add_library(plugin_llama-cpp-web SHARED
	./plugin_llama-cpp-web.cpp
)

target_link_libraries(plugin_llama-cpp-web PUBLIC
	solanaceae_plugin
	solanaceae_llama-cpp-web
)

