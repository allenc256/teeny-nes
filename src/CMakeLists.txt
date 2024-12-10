file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h")
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/app/main.cpp)

if (MSVC)
  set(CXX_FLAGS /W4 /WX)
else()
  set(CXX_FLAGS -Wall -Wextra -Wpedantic -Werror -Wconversion -Wno-gcc-compat -Wno-sign-conversion -march=native)
endif()

add_executable(teenynes app/main.cpp ${SOURCES})
add_library(teenynes_test_lib STATIC ${SOURCES})

target_compile_options(teenynes PRIVATE ${CXX_FLAGS})
target_compile_options(teenynes_test_lib PRIVATE ${CXX_FLAGS})

target_include_directories(teenynes PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(teenynes_test_lib PUBLIC ${PROJECT_SOURCE_DIR})
