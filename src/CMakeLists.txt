include(FetchContent)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.5
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
  nfd
  GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
  GIT_TAG v1.2.1
)
FetchContent_MakeAvailable(nfd)

file(GLOB IMGUI_CORE_SOURCES CONFIGURE_DEPENDS
  ${imgui_SOURCE_DIR}/*.cpp
  ${imgui_SOURCE_DIR}/*.h)
set(IMGUI_BACKENDS_SOURCES
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp)
include_directories(${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)

file(GLOB_RECURSE EMU_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/emu/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/emu/*.h)
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/app/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/app/*.h)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CXX_FLAGS /W4 /WX)
else()
  set(CXX_FLAGS -Wall -Wextra -Wpedantic -Werror -Wno-gcc-compat -Wno-sign-conversion -march=native)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CXX_FLAGS ${CXX_FLAGS} -Wconversion)
  endif()
endif()

# -Werror=conversion
add_executable(teenynes
  ${EMU_SOURCES}
  ${APP_SOURCES}
  ${IMGUI_CORE_SOURCES}
  ${IMGUI_BACKENDS_SOURCES})
add_library(teenynes_test_lib STATIC ${EMU_SOURCES})

target_compile_options(teenynes PRIVATE ${CXX_FLAGS})
target_compile_options(teenynes_test_lib PRIVATE ${CXX_FLAGS})
target_link_libraries(teenynes PRIVATE ${SDL2_LIBRARIES} nfd)
target_include_directories(teenynes PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(teenynes_test_lib PUBLIC ${PROJECT_SOURCE_DIR})
