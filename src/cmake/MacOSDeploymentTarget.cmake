include_guard(GLOBAL)

# Set defaults before project() initializes the compiler and Qt toolchain.
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin"
        OR (CMAKE_HOST_APPLE AND NOT CMAKE_SYSTEM_NAME))
    if(NOT CMAKE_OSX_ARCHITECTURES)
        # Replace an empty cached value while preserving explicit architectures.
        set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING
            "macOS target architectures" FORCE)
    endif()
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "$ENV{MACOSX_DEPLOYMENT_TARGET}")
        if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
            set(CMAKE_OSX_DEPLOYMENT_TARGET "14.4")
        endif()
        # Replace an empty cached value while preserving explicit targets.
        set(CMAKE_OSX_DEPLOYMENT_TARGET "${CMAKE_OSX_DEPLOYMENT_TARGET}" CACHE STRING
            "Minimum macOS version supported by the build" FORCE)
    endif()
endif()
