cmake_minimum_required(VERSION 3.16)
project(reply_evaluator)

# C++ 标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++17")

# 第三方库路径
set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# 查找必要依赖
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found in system, checking third_party...")
    if(EXISTS "${THIRD_PARTY_DIR}/nlohmann/json.hpp")
        message(STATUS "Found nlohmann/json.hpp in third_party")
    else()
        message(WARNING "nlohmann/json not found. Please download from https://github.com/nlohmann/json/releases")
    endif()
endif()

find_package(CURL QUIET)
if(NOT CURL_FOUND)
    message(WARNING "CURL not found, LLM mode may not be available")
endif()

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${THIRD_PARTY_DIR}
)

# 源文件列表
set(SOURCES
    src/main.cpp
    src/models.cpp
    src/json_loader.cpp
    src/evaluator.cpp
    src/mock_judge.cpp
    src/llm_judge.cpp
    src/report.cpp
    src/validation.cpp
)

# 创建可执行文件
add_executable(reply_eval ${SOURCES})

# 链接库
if(nlohmann_json_FOUND)
    target_link_libraries(reply_eval nlohmann_json::nlohmann_json)
endif()

if(CURL_FOUND)
    target_link_libraries(reply_eval CURL::libcurl)
endif()

# 输出目录
set_target_properties(reply_eval PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 编译信息
message(STATUS "C++ Standard: C++17")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "nlohmann_json found: ${nlohmann_json_FOUND}")
message(STATUS "CURL found: ${CURL_FOUND}")
