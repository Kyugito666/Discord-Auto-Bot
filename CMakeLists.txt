cmake_minimum_required(VERSION 3.15)
project(DiscordAutoBot CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FETCHCONTENT_QUIET NO)
include(FetchContent)

# Dependensi 1: nlohmann/json untuk parsing JSON
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

# Dependensi 2: CPR (C++ Requests) untuk permintaan HTTP
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.0
)
FetchContent_MakeAvailable(cpr)

# Dependensi 3: cpp-dotenv untuk memuat file .env
FetchContent_Declare(
    dotenv
    GIT_REPOSITORY https://github.com/caor-el/cpp-dotenv.git
    GIT_TAG v3.1.0
)
FetchContent_MakeAvailable(dotenv)

add_executable(bot src/main.cpp src/Bot.cpp)

target_link_libraries(bot PRIVATE
    nlohmann_json::nlohmann_json
    cpr::cpr
    dotenv::dotenv
)

target_include_directories(bot PRIVATE
    ${nlohmann_json_SOURCE_DIR}/include
    ${cpr_SOURCE_DIR}/include
    ${dotenv_SOURCE_DIR}/include
)
