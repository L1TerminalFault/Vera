cmake_minimum_required(VERSION 3.25)
project(Vera VERSION 0.1.0 LANGUAGES C CXX) # 1. Enabled C for generated code

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# 1. Gather all source files
# ============================================================================

# Grab all platform-agnostic core sources
file(GLOB_RECURSE VERA_SOURCES
    CONFIGURE_DEPENDS
    "src/core/*.cpp"
    "src/core/*.h"
    "src/core/*.hpp"
)

# Append OS-specific backend directories
if(WIN32)
  file(GLOB_RECURSE WIN32_SOURCES
        CONFIGURE_DEPENDS
        "src/platform/win32/*.cpp"
        "src/platform/win32/*.h"
        "src/platform/win32/*.hpp"
    )
  list(APPEND VERA_SOURCES ${WIN32_SOURCES})

elseif(UNIX AND NOT APPLE)
  # 2. Corrected standard recursive glob patterns
  file(GLOB_RECURSE LINUX_BACKEND_SOURCES
        CONFIGURE_DEPENDS
        "src/platform/x11/*.cpp"
        "src/platform/x11/*.cxx"
        "src/platform/x11/*.h"
        "src/platform/x11/*.hpp"
        "src/platform/x11/*.hxx"
        "src/platform/wayland/*.cpp"
        "src/platform/wayland/*.cxx"
        "src/platform/wayland/*.h"
        "src/platform/wayland/*.hpp"
        "src/platform/wayland/*.hxx"
    )
  list(APPEND VERA_SOURCES ${LINUX_BACKEND_SOURCES})

  # --- Dynamic Wayland Protocol Generation Setup ---
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
  pkg_check_modules(WAYLAND_CURSOR REQUIRED wayland-cursor)
  pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
  pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)

  # Find the wayland-scanner code-generator
  find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner REQUIRED)

  # Macro to compile system XML files into dynamic local sources and headers
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

  # Extract the standard system protocol directory path
  pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

  # Locate required raw XML protocols
  set(XDG_SHELL_XML "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
  set(XDG_DECORATION_XML "${WAYLAND_PROTOCOLS_DIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml")
  set(POINTER_CONSTRAINTS_XML "${WAYLAND_PROTOCOLS_DIR}/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml")
  set(RELATIVE_POINTER_XML "${WAYLAND_PROTOCOLS_DIR}/unstable/relative-pointer/relative-pointer-unstable-v1.xml")
  set(XDG_OUTPUT_XML "${WAYLAND_PROTOCOLS_DIR}/unstable/xdg-output/xdg-output-unstable-v1.xml")

  # Generate protocol files and append directly to VERA_SOURCES
  set(GENERATED_PROTOCOL_SOURCES)
  wayland_generate_protocol("${XDG_SHELL_XML}" GENERATED_PROTOCOL_SOURCES)
  wayland_generate_protocol("${XDG_DECORATION_XML}" GENERATED_PROTOCOL_SOURCES)
  wayland_generate_protocol("${POINTER_CONSTRAINTS_XML}" GENERATED_PROTOCOL_SOURCES)
  wayland_generate_protocol("${RELATIVE_POINTER_XML}" GENERATED_PROTOCOL_SOURCES)
  wayland_generate_protocol("${XDG_OUTPUT_XML}" GENERATED_PROTOCOL_SOURCES)
  list(APPEND VERA_SOURCES ${GENERATED_PROTOCOL_SOURCES})
endif()

# ============================================================================
# 2. Create the target
# ============================================================================
add_library(vera STATIC ${VERA_SOURCES})

# ============================================================================
# 3. Configure the "vera" target
# ============================================================================
target_include_directories(vera PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(MSVC)
  target_compile_options(vera PRIVATE /W4 /WX)
else()
  target_compile_options(vera PRIVATE -Wall -Wextra -Werror)
endif()

# Apply OS-specific definitions and library linkages
if(WIN32)
  target_compile_definitions(vera PRIVATE VERA_PLATFORM_WIN32 UNICODE _UNICODE)
  target_link_libraries(vera PRIVATE user32 gdi32 shell32 shcore dwmapi)
elseif(UNIX AND NOT APPLE)
  target_compile_definitions(vera PRIVATE VERA_PLATFORM_LINUX)

  # 3. Added generated protocols directory to build include paths
  target_include_directories(vera PRIVATE
      "${CMAKE_CURRENT_BINARY_DIR}"
      "${CMAKE_CURRENT_BINARY_DIR}/protocols"
      ${WAYLAND_CLIENT_INCLUDE_DIRS}
      ${WAYLAND_CURSOR_INCLUDE_DIRS}
      ${XKBCOMMON_INCLUDE_DIRS}
  )

  # Find legacy X11 dependencies
  find_package(X11 REQUIRED COMPONENTS Xrandr Xi)

  # Link targets to "vera" target
  target_link_libraries(vera PRIVATE
    X11::X11
    X11::Xrandr
    X11::Xi
    ${WAYLAND_CLIENT_LIBRARIES}
    ${WAYLAND_CURSOR_LIBRARIES}
    ${XKBCOMMON_LIBRARIES}
  )
endif()

# Code analysis properties
find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
  set_target_properties(vera PROPERTIES
    CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-extra-arg=-Wno-unknown-warning-option"
  )
endif()

# ============================================================================
# 4. Create example executable
# ============================================================================
add_executable(vera_example example/basic.cpp)
target_link_libraries(vera_example PRIVATE vera)
