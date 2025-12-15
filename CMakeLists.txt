cmake_minimum_required(VERSION 3.16)
project(x11bench VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(PkgConfig REQUIRED)

pkg_check_modules(X11 REQUIRED x11)
pkg_check_modules(XEXT REQUIRED xext)
pkg_check_modules(XRENDER REQUIRED xrender)
pkg_check_modules(XFT REQUIRED xft)
pkg_check_modules(PNG REQUIRED libpng)

# Option for static linking (future use)
option(BUILD_STATIC "Build with static linking" OFF)

# Source files
set(SOURCES
    src/main.cpp
    src/image.cpp
    src/display.cpp
    src/capture.cpp
    src/compare.cpp
    src/tests/test_shapes.cpp
    src/tests/test_colors.cpp
    src/tests/test_text.cpp
    src/tests/test_composite.cpp
    src/tests/test_advanced.cpp
)

# Create executable
add_executable(x11bench ${SOURCES})

# Include directories
target_include_directories(x11bench PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${X11_INCLUDE_DIRS}
    ${XEXT_INCLUDE_DIRS}
    ${XRENDER_INCLUDE_DIRS}
    ${XFT_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
)

# Link libraries
if(BUILD_STATIC)
    # Static linking (requires static libraries to be available)
    target_link_libraries(x11bench PRIVATE
        ${X11_STATIC_LIBRARIES}
        ${XEXT_STATIC_LIBRARIES}
        ${XRENDER_STATIC_LIBRARIES}
        ${XFT_STATIC_LIBRARIES}
        ${PNG_STATIC_LIBRARIES}
    )
    target_link_options(x11bench PRIVATE -static)
else()
    target_link_libraries(x11bench PRIVATE
        ${X11_LIBRARIES}
        ${XEXT_LIBRARIES}
        ${XRENDER_LIBRARIES}
        ${XFT_LIBRARIES}
        ${PNG_LIBRARIES}
    )
endif()

# Compiler flags
target_compile_options(x11bench PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Copy reference directory structure
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/reference)

# Install target (optional)
install(TARGETS x11bench RUNTIME DESTINATION bin)
