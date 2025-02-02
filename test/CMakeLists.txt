include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json
  GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

enable_testing()

include(GoogleTest)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h")

add_executable(teenynes_test ${SOURCES})
target_link_libraries(teenynes_test teenynes_test_lib GTest::gmock GTest::gtest_main nlohmann_json::nlohmann_json)

gtest_discover_tests(teenynes_test)