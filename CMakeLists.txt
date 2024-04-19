cmake_minimum_required(VERSION 3.28)

if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
    endif()
endif()

project(RIFE)

find_package(Boost 1.80 REQUIRED COMPONENTS filesystem)

option(NCNN_INSTALL_SDK "" OFF)
option(NCNN_PIXEL_ROTATE "" OFF)
option(NCNN_PIXEL_AFFINE "" OFF)
option(NCNN_PIXEL_DRAWING "" OFF)
option(NCNN_VULKAN "" ON)
option(NCNN_BUILD_BENCHMARK "" OFF)
option(NCNN_BUILD_TESTS "" OFF)
option(NCNN_BUILD_TOOLS "" OFF)
option(NCNN_BUILD_EXAMPLES "" OFF)
option(NCNN_SIMPLEVK "" OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ncnn)

set(sources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rife.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/warp.cpp
)

if (WIN32)
    set(sources
        ${sources}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/rife.rc
    )
endif()

add_library(RIFE SHARED ${sources})

target_include_directories(RIFE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(RIFE PRIVATE
    Boost::filesystem
    ncnn
    avisynth
)

target_compile_features(RIFE PRIVATE cxx_std_20)

if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    string(TOLOWER ${CMAKE_BUILD_TYPE} build_type)
    if (build_type STREQUAL debug)
        target_compile_definitions(RIFE PRIVATE DEBUG_BUILD)
    else (build_type STREQUAL release)
        target_compile_definitions(RIFE PRIVATE RELEASE_BUILD)
    endif()

    message(STATUS "Build type - ${CMAKE_BUILD_TYPE}")
else()
    target_compile_options(RIFE PRIVATE "/EHsc")
endif()

if (WIN32)
    set_target_properties(RIFE PROPERTIES
        PREFIX ""
        OUTPUT_NAME "RIFE"
    )
else()
    find_package(Git)
    if (GIT_FOUND)
        execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
            OUTPUT_VARIABLE ver
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set_target_properties(RIFE PROPERTIES OUTPUT_NAME "rife.${ver}")
    endif()

    include(GNUInstallDirs)
    INSTALL(TARGETS RIFE LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/avisynth")

    # uninstall target
    if(NOT TARGET uninstall)
    configure_file(
        "${PROJECT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
    endif()
endif()
