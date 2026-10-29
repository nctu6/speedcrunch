# Macdeployqt.cmake
#
# Copyright (c) 2017 Samuel Gaist <samuel.gaist@edeltech.ch>
#

set(SPEEDCRUNCH_MACOS_DEPLOY_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/DeployMacOS.cmake")

function(macdeployqt target app_name)
    install(CODE "
        set(APP_BUNDLE \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${BIN_INSTALL_DIR}/${target}\")
        set(APP_EXECUTABLE \"${app_name}\")
        set(MACDEPLOYQT \"$<TARGET_FILE:Qt6::macdeployqt>\")
        include(\"${SPEEDCRUNCH_MACOS_DEPLOY_SCRIPT}\")
    ")
endfunction()
