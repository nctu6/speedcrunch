include_guard(GLOBAL)

option(BUILD_VARIANT_OFFICIAL_RELEASE "Force the official Release build settings" OFF)
option(BUILD_VARIANT_OFFICIAL_PORTABLE_RELEASE "Force the official portable Release build settings" OFF)
option(BUILD_VARIANT_DEV_DEBUG "Force the developer Debug build settings" OFF)

set(speedcrunch_build_variants)
foreach(variant IN ITEMS
        BUILD_VARIANT_OFFICIAL_RELEASE
        BUILD_VARIANT_OFFICIAL_PORTABLE_RELEASE
        BUILD_VARIANT_DEV_DEBUG)
    if(${variant})
        list(APPEND speedcrunch_build_variants ${variant})
    endif()
endforeach()

list(LENGTH speedcrunch_build_variants speedcrunch_build_variant_count)
if(speedcrunch_build_variant_count GREATER 1)
    list(JOIN speedcrunch_build_variants ", " speedcrunch_build_variant_names)
    message(FATAL_ERROR
        "Enable only one build variant at a time. Enabled: ${speedcrunch_build_variant_names}")
elseif(speedcrunch_build_variant_count EQUAL 0)
    return()
endif()

if(BUILD_VARIANT_DEV_DEBUG)
    set(speedcrunch_variant_configuration Debug)
    set(BUILD_TESTING ON CACHE BOOL "Build the test executables and enable CTest" FORCE)
    set(ENABLE_STARTUP_DIAGNOSTICS ON CACHE BOOL
        "Record startup timings and detailed appearance diagnostics" FORCE)
else()
    set(speedcrunch_variant_configuration Release)
    set(BUILD_TESTING OFF CACHE BOOL "Build the test executables and enable CTest" FORCE)
    set(INSTALL_LICENSES ON CACHE BOOL "Install license texts and notices" FORCE)
    set(STRIP_BINARY ON CACHE BOOL "Remove debug symbols from the application binary" FORCE)
endif()

set(ENABLE_UPDATE_CHECKS ON CACHE BOOL "Enable launch-time and manual update checks" FORCE)
set(CMAKE_BUILD_TYPE "${speedcrunch_variant_configuration}" CACHE STRING
    "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)

# Apply the configuration before project() initializes the compiler and toolchain.
get_property(speedcrunch_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(speedcrunch_multi_config)
    set(CMAKE_CONFIGURATION_TYPES "${speedcrunch_variant_configuration}" CACHE STRING
        "Available build configurations" FORCE)
endif()

if(BUILD_VARIANT_OFFICIAL_PORTABLE_RELEASE)
    set(PORTABLE_SPEEDCRUNCH ON CACHE BOOL "Enable to build in portable mode" FORCE)
    set(CPACK_GENERATOR ZIP CACHE STRING "CPack package generators" FORCE)
endif()

message(STATUS "Build variant: ${speedcrunch_build_variants}")
