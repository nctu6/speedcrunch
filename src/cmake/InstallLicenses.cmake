option(INSTALL_LICENSES "Install license texts and notices" OFF)
if(NOT INSTALL_LICENSES)
    return()
endif()

set(speedcrunch_license_dir "${DOC_INSTALL_DIR}/Licenses")

if(WIN32)
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/../../pkg/COPYING.rtf"
        DESTINATION "${DOC_INSTALL_DIR}")
endif()

set(speedcrunch_qt_notice [[
Qt is supplied by the operating system or package provider. Its license texts,
copyright notices and source information accompany the Qt packages.]])
set(speedcrunch_qt_replacement_notice
    "You may replace the system Qt libraries under their applicable licenses.")

if(WIN32 OR APPLE)
    get_filename_component(speedcrunch_qt_prefix "${Qt6Core_DIR}/../../.." ABSOLUTE)
    get_filename_component(speedcrunch_qt_sources "${speedcrunch_qt_prefix}/../Src" ABSOLUTE)
    set(SPEEDCRUNCH_QT_SOURCE_DIR "${speedcrunch_qt_sources}" CACHE PATH
        "Matching Qt sources containing license texts for bundled Qt libraries")
    set(speedcrunch_qt_modules qtbase qttools)
    foreach(module IN ITEMS qtsvg qtimageformats)
        if(APPLE OR EXISTS "${speedcrunch_qt_prefix}/sbom/${module}-${Qt6_VERSION}.spdx")
            list(APPEND speedcrunch_qt_modules "${module}")
        endif()
    endforeach()
    string(REPLACE ";" ", " speedcrunch_qt_module_names "${speedcrunch_qt_modules}")
    string(CONFIGURE [[
The bundled Qt components come from @speedcrunch_qt_module_names@.
The Qt directory contains their license texts and the SDK's SPDX documents,
including third-party copyright notices, license terms and source locations.
These documents cover the complete Qt modules, including components that
SpeedCrunch does not use. Each component retains its own license.

Qt's open-source license alternatives are recorded in the SPDX documents.
The LGPLv3 and GPLv3 texts are included alongside the other Qt license texts.
The presence of a commercial license text does not grant a commercial license.
The SPDX documents identify the source revisions used by the Qt SDK. Match
those revisions when obtaining sources or rebuilding the bundled libraries.
]] speedcrunch_qt_notice @ONLY)
    set(speedcrunch_qt_replacement_notice [[
You may modify or replace the dynamically linked Qt libraries under their
applicable licenses, including reverse engineering to debug such changes.
]])
    if(APPLE)
        string(APPEND speedcrunch_qt_replacement_notice
            [[
After replacing frameworks in Contents/Frameworks or plugins in Contents/PlugIns,
apply a new ad hoc signature to your copy of the app:

    codesign --force --deep --sign - /path/to/SpeedCrunch.app]])
    else()
        string(APPEND speedcrunch_qt_replacement_notice
            [[
Replace Qt DLLs beside speedcrunch.exe and plugins in their category folders
with compatible builds, then start the application again.]])
    endif()
endif()

string(STRIP "${speedcrunch_qt_notice}" speedcrunch_qt_notice)
configure_file("${CMAKE_CURRENT_LIST_DIR}/LICENSES.txt.in"
    "${CMAKE_CURRENT_BINARY_DIR}/LICENSES.txt" @ONLY)
install(FILES "${CMAKE_CURRENT_LIST_DIR}/../../LICENSE"
    DESTINATION "${speedcrunch_license_dir}" RENAME SpeedCrunch-GPL-2.0.txt)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/LICENSES.txt"
    DESTINATION "${speedcrunch_license_dir}" RENAME README.txt)

if(WIN32 OR APPLE)
    # Install notices before macOS deployment signs the complete bundle.
    install(CODE "
        set(QT_LICENSE_DESTINATION \"${speedcrunch_license_dir}/Qt\")
        set(QT_SOURCE_DIR \"${SPEEDCRUNCH_QT_SOURCE_DIR}\")
        set(QT_SBOM_DIR \"${speedcrunch_qt_prefix}/sbom\")
        set(QT_VERSION \"${Qt6_VERSION}\")
        set(QT_MODULES \"${speedcrunch_qt_modules}\")
        include(\"${CMAKE_CURRENT_LIST_DIR}/InstallQtLicenses.cmake\")
    ")
endif()
