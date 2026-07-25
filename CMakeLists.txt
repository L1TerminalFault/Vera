cmake_minimum_required(VERSION 3.25)
project(Vera VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# 1. Gather all source files for vera_windowing
# ============================================================================

file(GLOB_RECURSE VERA_WINDOWING_SOURCES
    CONFIGURE_DEPENDS
    "src/vera_windowing/core/*.cpp"
    "src/vera_windowing/core/*.cxx"
    "src/vera_windowing/core/*.h"
    "src/vera_windowing/core/*.hpp"
)

if(WIN32)
  file(GLOB_RECURSE WIN32_WINDOWING_SOURCES
        CONFIGURE_DEPENDS
        "src/vera_windowing/platform/win32/*.cpp"
        "src/vera_windowing/platform/win32/*.cxx"
        "src/vera_windowing/platform/win32/*.h"
        "src/vera_windowing/platform/win32/*.hpp"
    )
  list(APPEND VERA_WINDOWING_SOURCES ${WIN32_WINDOWING_SOURCES})

elseif(UNIX AND NOT APPLE)
  file(GLOB_RECURSE LINUX_WINDOWING_SOURCES
        CONFIGURE_DEPENDS
        "src/vera_windowing/platform/x11/*.cpp"
        "src/vera_windowing/platform/x11/*.cxx"
        "src/vera_windowing/platform/x11/*.h"
        "src/vera_windowing/platform/x11/*.hpp"
        "src/vera_windowing/platform/wayland/*.cpp"
        "src/vera_windowing/platform/wayland/*.cxx"
        "src/vera_windowing/platform/wayland/*.h"
        "src/vera_windowing/platform/wayland/*.hpp"
    )
  list(APPEND VERA_WINDOWING_SOURCES ${LINUX_WINDOWING_SOURCES})

  # --- Dynamic Wayland Protocol Generation Setup ---
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
  pkg_check_modules(WAYLAND_CURSOR REQUIRED wayland-cursor)
  pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
  pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)

  find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner REQUIRED)

  macro(wayland_generate_protocol xml_path target_sources_var)
    get_filename_component(protocol_name "${xml_path}" NAME_WE)

    set(header_out "${CMAKE_CURRENT_BINARY_DIR}/protocols/${protocol_name}-client-protocol.h")
    set(code_out "${CMAKE_CURRENT_BINARY_DIR}/protocols/${protocol_name}-protocol.c")

    add_custom_command(
            OUTPUT "${header_out}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/protocols"
            COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header "${xml_path}" "${header_out}"
            DEPENDS "${xml_path}"
            VERBATIM
        )

    add_custom_command(
            OUTPUT "${code_out}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/protocols"
            COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code "${xml_path}" "${code_out}"
            DEPENDS "${xml_path}"
            VERBATIM
        )

    list(APPEND ${target_sources_var} "${header_out}" "${code_out}")
  endmacro()

  pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

  set(XDG_SHELL_XML "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
  set(XDG_DECORATION_XML "${WAYLAND_PROTOCOLS_DIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml")
  set(POINTER_CONSTRAINTS_XML "${WAYLAND_PROTOCOLS_DIR}/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml")
  set(RELATIVE_POINTER_XML "${WAYLAND_PROTOCOLS_DIR}/unstable/relative-pointer/relative-pointer-unstable-v1.xml")
  set(XDG_OUTPUT_XML "${WAYLAND_PROTOCOLS_DIR}/unstable/xdg-output/xdg-output-unstable-v1.xml")

  set(GENERATED_PROTOCOL_SOURCES)
  wayland_generate_protocol("${XDG_SHELL_XML}" GENERATED_PROTOCOL_SOURCES)
  wayland_generate_protocol("${XDG_DECORATION_XML}" GENERATED_PROTOCOL_SOURCES)
  wayland_generate_protocol("${POINTER_CONSTRAINTS_XML}" GENERATED_PROTOCOL_SOURCES)
  wayland_generate_protocol("${RELATIVE_POINTER_XML}" GENERATED_PROTOCOL_SOURCES)
  wayland_generate_protocol("${XDG_OUTPUT_XML}" GENERATED_PROTOCOL_SOURCES)
  list(APPEND VERA_WINDOWING_SOURCES ${GENERATED_PROTOCOL_SOURCES})
endif()

# ============================================================================
# 2. Gather all source files for vera_shell
# ============================================================================

file(GLOB_RECURSE VERA_SHELL_SOURCES
    CONFIGURE_DEPENDS
    "src/vera_shell/core_shell/*.cpp"
    "src/vera_shell/core_shell/*.cxx"
    "src/vera_shell/core_shell/*.h"
    "src/vera_shell/core_shell/*.hpp"
)

if(WIN32)
  file(GLOB_RECURSE WIN32_SHELL_SOURCES
        CONFIGURE_DEPENDS
        "src/vera_shell/platform/win32/*.cpp"
        "src/vera_shell/platform/win32/*.cxx"
        "src/vera_shell/platform/win32/*.h"
        "src/vera_shell/platform/win32/*.hpp"
    )
  list(APPEND VERA_SHELL_SOURCES ${WIN32_SHELL_SOURCES})

elseif(UNIX AND NOT APPLE)
  file(GLOB_RECURSE LINUX_SHELL_SOURCES
        CONFIGURE_DEPENDS
        "src/vera_shell/platform/linux/*.cpp"
        "src/vera_shell/platform/linux/*.cxx"
        "src/vera_shell/platform/linux/*.h"
        "src/vera_shell/platform/linux/*.hpp"
    )
  list(APPEND VERA_SHELL_SOURCES ${LINUX_SHELL_SOURCES})

  # Use gio-unix-2.0 instead of gio-2.0 for UNIX app info support
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GIO_UNIX REQUIRED gio-unix-2.0)
  pkg_check_modules(CANBERRA REQUIRED libcanberra)
endif()

# ============================================================================
# 3. Create targets
# ============================================================================

add_library(vera_windowing STATIC ${VERA_WINDOWING_SOURCES})
add_library(vera_shell STATIC ${VERA_SHELL_SOURCES})

target_include_directories(vera_windowing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/vera_windowing)
target_include_directories(vera_shell PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vera_shell
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vera_shell/core_shell
)

# Set common compiler warnings
if(MSVC)
  target_compile_options(vera_windowing PRIVATE /W4 /WX)
  target_compile_options(vera_shell PRIVATE /W4 /WX)
else()
  target_compile_options(vera_windowing PRIVATE -Wall -Wextra -Werror)
  target_compile_options(vera_shell PRIVATE -Wall -Wextra -Werror)
endif()

# ============================================================================
# 4. OS-Specific Links and Definitions
# ============================================================================

if(WIN32)
  target_compile_definitions(vera_windowing PRIVATE VERA_PLATFORM_WIN32 UNICODE _UNICODE)
  target_link_libraries(vera_windowing PRIVATE user32 gdi32 shell32 shcore dwmapi)

  target_compile_definitions(vera_shell PRIVATE VERA_PLATFORM_WIN32 UNICODE _UNICODE)
  target_link_libraries(vera_shell PRIVATE user32 shell32 ole32 shlwapi)

elseif(UNIX AND NOT APPLE)
  target_compile_definitions(vera_windowing PRIVATE VERA_PLATFORM_LINUX)
  target_include_directories(vera_windowing PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/protocols"
        ${WAYLAND_CLIENT_INCLUDE_DIRS}
        ${WAYLAND_CURSOR_INCLUDE_DIRS}
        ${XKBCOMMON_INCLUDE_DIRS}
    )

  find_package(X11 REQUIRED COMPONENTS Xrandr Xi)

  target_link_libraries(vera_windowing PRIVATE
        X11::X11
        X11::Xrandr
        X11::Xi
        ${WAYLAND_CLIENT_LIBRARIES}
        ${WAYLAND_CURSOR_LIBRARIES}
        ${XKBCOMMON_LIBRARIES}
    )

  target_compile_definitions(vera_shell PRIVATE VERA_PLATFORM_LINUX)

  # PUBLIC includes propagate GIO UNIX paths to dependent targets
  target_include_directories(vera_shell PUBLIC
        ${GIO_UNIX_INCLUDE_DIRS}
        ${CANBERRA_INCLUDE_DIRS}
    )
  target_link_libraries(vera_shell PUBLIC
        ${GIO_UNIX_LIBRARIES}
        ${CANBERRA_LIBRARIES}
    )
endif()

# Code analysis properties
find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
  set_target_properties(vera_windowing PROPERTIES
        CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-extra-arg=-Wno-unknown-warning-option"
    )
  set_target_properties(vera_shell PROPERTIES
        CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-extra-arg=-Wno-unknown-warning-option"
    )
endif()

# ============================================================================
# 5. Create example executable
# ============================================================================

add_executable(vera_example example/basic.cpp)

# Links example executable to both windowing and shell modules
target_link_libraries(vera_example PUBLIC
    vera_windowing
    vera_shell
)
