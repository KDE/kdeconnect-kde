project(KDEConnectInterfaces)

function(geninterface source_h output_h)
    set(xml_file ${CMAKE_CURRENT_BINARY_DIR}/${output_h}.xml)
    qt5_generate_dbus_interface( ${source_h} ${xml_file})

    list(APPEND libkdeconnect_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/${output_h})
    set_source_files_properties(${xml_file} PROPERTIES NO_NAMESPACE true)

    qt5_add_dbus_interface(libkdeconnect_SRC ${xml_file} ${output_h})
    set(libkdeconnect_SRC ${libkdeconnect_SRC} PARENT_SCOPE)
    set(libkdeconnect_HEADERS ${libkdeconnect_HEADERS} PARENT_SCOPE)
endfunction()

set(libkdeconnect_SRC
    dbusinterfaces.cpp
    devicesmodel.cpp
    notificationsmodel.cpp
    devicessortproxymodel.cpp
#    modeltest.cpp
)

set(libkdeconnect_public_HEADERS
    KDEConnect/DevicesModel
    KDEConnect/NotificationsModel
)

set(libkdeconnect_HEADERS
    devicesmodel.h
    notificationsmodel.h
    dbusinterfaces.h
    ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectinterfaces_export.h
)

geninterface(${CMAKE_SOURCE_DIR}/core/daemon.h daemoninterface)
geninterface(${CMAKE_SOURCE_DIR}/core/device.h deviceinterface)
geninterface(${CMAKE_SOURCE_DIR}/plugins/battery/batterydbusinterface.h devicebatteryinterface)
geninterface(${CMAKE_SOURCE_DIR}/plugins/sftp/sftpplugin.h devicesftpinterface)
geninterface(${CMAKE_SOURCE_DIR}/plugins/notifications/notificationsdbusinterface.h devicenotificationsinterface)
geninterface(${CMAKE_SOURCE_DIR}/plugins/findmyphone/findmyphoneplugin.h devicefindmyphoneinterface)
geninterface(${CMAKE_SOURCE_DIR}/plugins/notifications/notification.h notificationinterface)
geninterface(${CMAKE_SOURCE_DIR}/plugins/mprisremote/mprisremoteplugin.h mprisremoteinterface)
geninterface(${CMAKE_SOURCE_DIR}/plugins/remotecontrol/remotecontrolplugin.h remotecontrolinterface)
geninterface(${CMAKE_SOURCE_DIR}/plugins/lockdevice/lockdeviceplugin.h lockdeviceinterface)
geninterface(${CMAKE_SOURCE_DIR}/plugins/remotecommands/remotecommandsplugin.h remotecommandsinterface)
geninterface(${CMAKE_SOURCE_DIR}/plugins/remotekeyboard/remotekeyboardplugin.h remotekeyboardinterface)


add_library(kdeconnectinterfaces SHARED ${libkdeconnect_SRC})
set_target_properties(kdeconnectinterfaces PROPERTIES
    VERSION ${KDECONNECT_VERSION}
    SOVERSION ${KDECONNECT_VERSION_MAJOR}
)

generate_export_header(kdeconnectinterfaces EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectinterfaces_export.h BASE_NAME KDEConnectInterfaces)

target_link_libraries(kdeconnectinterfaces
LINK_PUBLIC
    Qt5::Gui
    Qt5::DBus
LINK_PRIVATE
    KF5::ConfigCore
    KF5::I18n
)

configure_file(KDEConnectConfig.cmake.in ${CMAKE_BINARY_DIR}/interfaces/KDEConnectConfig.cmake @ONLY)

ecm_setup_version( "${KDECONNECT_VERSION_MAJOR}.${KDECONNECT_VERSION_MINOR}.${KDECONNECT_VERSION_PATCH}" VARIABLE_PREFIX KDECONNECTINTERFACES
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kdeconnectinterfaces_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KDEConnectConfigVersion.cmake"
                        SOVERSION ${KDECONNECT_VERSION_MAJOR})

install(TARGETS kdeconnectinterfaces EXPORT kdeconnectLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

## Don't install header files until API/ABI policy is defined
#
# install(FILES ${libkdeconnect_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/kdeconnect COMPONENT Devel)
# install(FILES ${libkdeconnect_public_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/KDEConnect COMPONENT Devel)
# install(FILES ${CMAKE_BINARY_DIR}/interfaces/KDEConnectConfig.cmake
#               ${CMAKE_BINARY_DIR}/interfaces/KDEConnectConfigVersion.cmake
#         DESTINATION ${LIB_INSTALL_DIR}/cmake/KDEConnect)
