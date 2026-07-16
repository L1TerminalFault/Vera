cmake_minimum_required(VERSION 3.25)
project(Vera VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE VERA_SOURCES
    CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.h"
    "src/*.hpp"
)

add_library(vera STATIC ${VERA_SOURCES})

target_include_directories(vera PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(MSVC)
  target_compile_options(vera PRIVATE  /W4 /WX)
else()
  target_compile_options(vera PRIVATE -Wall -Wextra -Werror)
endif()

find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
  set_target_properties(vera PROPERTIES
    CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-extra-arg=-Wno-unknown-warning-option"
  )
endif()

if(WIN32)
  target_link_libraries(vera PRIVATE user32 gdi32 shell32 shcore dwmapi)
  target_compile_definitions(vera PRIVATE VERA_PLATFORM_WIN32 UNICODE _UNICODE)
elseif(UNIX AND NOT APPLE)
  target_compile_definitions(vera PRIVATE VERA_PLATFORM_LINUX)
endif()

add_executable(vera_example example/basic.cpp)
target_link_libraries(vera_example PRIVATE vera)

