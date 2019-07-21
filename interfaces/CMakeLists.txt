function(geninterface source_h output_h)
    set(xml_file ${CMAKE_CURRENT_BINARY_DIR}/${output_h}.xml)
    qt5_generate_dbus_interface( ${source_h} ${xml_file})

    set_source_files_properties(${xml_file} PROPERTIES NO_NAMESPACE true)

    qt5_add_dbus_interface(libkdeconnect_SRC ${xml_file} ${output_h})
    set(libkdeconnect_SRC ${libkdeconnect_SRC} PARENT_SCOPE)
endfunction()

set(libkdeconnect_SRC
    dbusinterfaces.cpp
    devicesmodel.cpp
    notificationsmodel.cpp
    devicessortproxymodel.cpp
    conversationmessage.cpp
    remotecommandsmodel.cpp
    remotesinksmodel.cpp
    devicespluginfilterproxymodel.cpp
#    modeltest.cpp
)

ecm_qt_declare_logging_category(
    libkdeconnect_SRC
    HEADER interfaces_debug.h
    IDENTIFIER KDECONNECT_INTERFACES
    CATEGORY_NAME kdeconnect.interfaces
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
geninterface(${CMAKE_SOURCE_DIR}/plugins/sms/smsplugin.h smsinterface)
geninterface(${CMAKE_SOURCE_DIR}/plugins/sms/conversationsdbusinterface.h conversationsinterface)
geninterface(${CMAKE_SOURCE_DIR}/plugins/share/shareplugin.h shareinterface)
geninterface(${CMAKE_SOURCE_DIR}/plugins/remotesystemvolume/remotesystemvolumeplugin.h remotesystemvolumeinterface)

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
    kdeconnectcore
)

install(TARGETS kdeconnectinterfaces EXPORT kdeconnectLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
