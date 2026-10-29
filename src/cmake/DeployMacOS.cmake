# Deploy and sign the installed bundle before CPack creates the package.
if(NOT IS_DIRECTORY "${APP_BUNDLE}/Contents/MacOS")
    message(FATAL_ERROR "Missing application bundle: ${APP_BUNDLE}")
endif()
if(NOT EXISTS "${MACDEPLOYQT}")
    message(FATAL_ERROR "Missing macdeployqt: ${MACDEPLOYQT}")
endif()

get_filename_component(MACDEPLOYQT "${MACDEPLOYQT}" REALPATH)
get_filename_component(qt_bin_dir "${MACDEPLOYQT}" DIRECTORY)

execute_process(
    COMMAND "${qt_bin_dir}/qtpaths" --query QT_INSTALL_PLUGINS
    OUTPUT_VARIABLE qt_plugins_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE query_result
)
if(NOT query_result STREQUAL "0" OR NOT IS_DIRECTORY "${qt_plugins_dir}")
    message(FATAL_ERROR "Cannot locate plugins for ${MACDEPLOYQT}")
endif()

set(required_plugins platforms/libqcocoa.dylib sqldrivers/libqsqlite.dylib)
foreach(plugin IN LISTS required_plugins)
    if(NOT EXISTS "${qt_plugins_dir}/${plugin}")
        message(FATAL_ERROR "Missing required Qt plugin: ${qt_plugins_dir}/${plugin}")
    endif()
endforeach()

set(bundle_plugins_dir "${APP_BUNDLE}/Contents/PlugIns")
# Qt Help needs SQLite. Remove old drivers before macdeployqt scans the bundle.
file(REMOVE_RECURSE "${bundle_plugins_dir}/sqldrivers")
set(plugins ${required_plugins})
foreach(category IN ITEMS
        printsupport styles tls networkinformation imageformats iconengines platforminputcontexts)
    file(GLOB category_plugins RELATIVE "${qt_plugins_dir}" "${qt_plugins_dir}/${category}/*.dylib")
    list(FILTER category_plugins EXCLUDE REGEX "_debug\\.dylib$")
    list(APPEND plugins ${category_plugins})
endforeach()
foreach(plugin IN LISTS plugins)
    get_filename_component(category "${plugin}" DIRECTORY)
    file(COPY "${qt_plugins_dir}/${plugin}" DESTINATION "${bundle_plugins_dir}/${category}")
endforeach()

# macdeployqt scans the copied plugins and deploys their framework dependencies.
# Disable its plugin selection so it cannot bring back the unused SQL drivers.
execute_process(
    COMMAND "${MACDEPLOYQT}" "${APP_BUNDLE}" -no-plugins -always-overwrite -codesign=-
    OUTPUT_VARIABLE deploy_output
    ERROR_VARIABLE deploy_error
    RESULT_VARIABLE deploy_result
)
message("${deploy_output}${deploy_error}")
if(NOT deploy_result STREQUAL "0" OR "${deploy_output}${deploy_error}" MATCHES "(^|\n)ERROR:")
    message(FATAL_ERROR "Qt deployment failed for ${APP_BUNDLE}")
endif()

if(NOT EXISTS "${APP_BUNDLE}/Contents/Frameworks/QtCore.framework")
    message(FATAL_ERROR "Qt deployment failed: missing bundled QtCore.framework")
endif()

execute_process(
    COMMAND /usr/bin/otool -L "${APP_BUNDLE}/Contents/MacOS/${APP_EXECUTABLE}"
    OUTPUT_VARIABLE linked_libraries
    ERROR_VARIABLE dependency_error
    RESULT_VARIABLE dependency_result
)
if(NOT dependency_result STREQUAL "0")
    message(FATAL_ERROR "Cannot inspect deployed application dependencies: ${dependency_error}")
endif()
if(linked_libraries MATCHES "/opt/homebrew|/usr/local|Cellar")
    message(FATAL_ERROR "Deployed application still links to Homebrew paths: ${linked_libraries}")
endif()

execute_process(
    COMMAND /usr/bin/codesign --verify --deep --strict "${APP_BUNDLE}"
    OUTPUT_VARIABLE signing_output
    ERROR_VARIABLE signing_error
    RESULT_VARIABLE signing_result
)
if(NOT signing_result STREQUAL "0")
    message(FATAL_ERROR "Bundle signature verification failed: ${signing_output}${signing_error}")
endif()
