cmake_minimum_required(VERSION 3.9...3.24 FATAL_ERROR)

project(solanaceae)

add_library(solanaceae_llama-cpp-web
	./solanaceae/llama-cpp-web/text_completion_interface.hpp
	./solanaceae/llama-cpp-web/llama_cpp_web_impl.hpp
	./solanaceae/llama-cpp-web/llama_cpp_web_impl.cpp
)

target_include_directories(solanaceae_llama-cpp-web PUBLIC .)
target_compile_features(solanaceae_llama-cpp-web PRIVATE cxx_std_20)
target_compile_features(solanaceae_llama-cpp-web INTERFACE cxx_std_17)
target_link_libraries(solanaceae_llama-cpp-web PUBLIC
	httplib::httplib
	nlohmann_json::nlohmann_json

	solanaceae_util
)

########################################

add_executable(solanaceae_llama-cpp-web_test1 EXCLUDE_FROM_ALL
	test1.cpp
)

target_link_libraries(solanaceae_llama-cpp-web_test1 PUBLIC
	solanaceae_llama-cpp-web
)

########################################

add_library(solanaceae_rpbot
	./solanaceae/rpbot/message_prompt_builder.hpp
	./solanaceae/rpbot/message_prompt_builder.cpp

	./solanaceae/rpbot/rpbot.hpp
	./solanaceae/rpbot/rpbot_states.hpp
	./solanaceae/rpbot/rpbot.cpp
	./solanaceae/rpbot/rpbot_commands.cpp
)

target_include_directories(solanaceae_rpbot PUBLIC .)
target_compile_features(solanaceae_rpbot PRIVATE cxx_std_20)
target_compile_features(solanaceae_rpbot INTERFACE cxx_std_17)
target_link_libraries(solanaceae_rpbot PUBLIC
	solanaceae_util
	solanaceae_message3

	solanaceae_llama-cpp-web

	fmt::fmt # TODO: switch to header only?
)

