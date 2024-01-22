cmake_minimum_required(VERSION 3.9...3.24 FATAL_ERROR)

project(solanaceae)

add_library(solanaceae_llama-cpp-web
	./solanaceae/llama-cpp-web/llama_cpp_web_interface.hpp
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
	solanaceae_message3
)

########################################

add_executable(test1
	test1.cpp
)

target_link_libraries(test1 PUBLIC
	solanaceae_llama-cpp-web
)

