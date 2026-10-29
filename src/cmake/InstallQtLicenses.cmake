if(IS_ABSOLUTE "${QT_LICENSE_DESTINATION}")
    set(qt_license_dir "$ENV{DESTDIR}${QT_LICENSE_DESTINATION}")
else()
    set(qt_license_dir "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${QT_LICENSE_DESTINATION}")
endif()

# Keep the SDK's build-specific copyrights and notices with its license texts.
file(REMOVE_RECURSE "${qt_license_dir}")
foreach(module IN LISTS QT_MODULES)
    set(module_source_dir "${QT_SOURCE_DIR}/${module}")
    set(module_sbom "${QT_SBOM_DIR}/${module}-${QT_VERSION}.spdx")
    file(GLOB module_license_files "${module_source_dir}/LICENSES/*.txt")
    if(NOT EXISTS "${module_source_dir}/.cmake.conf" OR NOT module_license_files)
        message(FATAL_ERROR "Missing ${module} license texts. Install matching Qt Sources "
            "or set SPEEDCRUNCH_QT_SOURCE_DIR to the Qt ${QT_VERSION} source tree.")
    endif()
    file(STRINGS "${module_source_dir}/.cmake.conf" module_version_line
        REGEX "^set\\(QT_REPO_MODULE_VERSION ")
    string(REGEX MATCH "\"([^\"]+)\"" module_version "${module_version_line}")
    if(NOT "${CMAKE_MATCH_1}" STREQUAL "${QT_VERSION}")
        message(FATAL_ERROR "${module_source_dir} does not contain Qt ${QT_VERSION} sources")
    endif()
    if(NOT EXISTS "${module_sbom}")
        message(FATAL_ERROR "Missing Qt component notices: ${module_sbom}. "
            "Use a Qt SDK with SPDX SBOM files when bundling Qt libraries.")
    endif()
    file(COPY "${module_source_dir}/LICENSES" "${module_sbom}"
        DESTINATION "${qt_license_dir}/${module}")
endforeach()
