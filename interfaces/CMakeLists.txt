add_definitions(-DTRANSLATION_DOMAIN=\"kdeconnect-interfaces\")

function(geninterface source_h output_h)
    set(xml_file ${CMAKE_CURRENT_BINARY_DIR}/${output_h}.xml)
    qt_generate_dbus_interface( ${source_h} ${xml_file})

    set_source_files_properties(${xml_file} PROPERTIES NO_NAMESPACE true)

    qt_add_dbus_interface(libkdeconnect_SRC ${xml_file} ${output_h})
    set(libkdeconnect_SRC ${libkdeconnect_SRC} PARENT_SCOPE)
endfunction()

set(debug_files_SRCS)
ecm_qt_declare_logging_category(
    debug_files_SRCS HEADER interfaces_conversation_message_debug.h
    IDENTIFIER CONVERSATION_MESSAGE_LOGGING_CATEGORY CATEGORY_NAME kdeconnect.interfaces.conversationmessage
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (conversation message)")

ecm_qt_declare_logging_category(
    debug_files_SRCS HEADER interfaces_debug.h
    IDENTIFIER KDECONNECT_INTERFACES CATEGORY_NAME kdeconnect.interfaces
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (interfaces)")

set(libkdeconnect_SRC
    dbusinterfaces.cpp
    devicesmodel.cpp
    notificationsmodel.cpp
    devicessortproxymodel.cpp
    conversationmessage.cpp
    remotecommandsmodel.cpp
    remotesinksmodel.cpp
    devicespluginfilterproxymodel.cpp
    pluginmodel.cpp
    commandsmodel.cpp
#    modeltest.cpp
    ${debug_files_SRCS}
)

geninterface(${PROJECT_SOURCE_DIR}/core/daemon.h daemoninterface)
geninterface(${PROJECT_SOURCE_DIR}/core/device.h deviceinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/battery/batteryplugin.h batteryinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/connectivity-report/connectivity_reportplugin.h connectivityinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/sftp/sftpplugin.h devicesftpinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/notifications/notificationsplugin.h devicenotificationsinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/findmyphone/findmyphoneplugin.h devicefindmyphoneinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/notifications/notification.h notificationinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/mprisremote/mprisremoteplugin.h mprisremoteinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/remotecontrol/remotecontrolplugin.h remotecontrolinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/lockdevice/lockdeviceplugin.h lockdeviceinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/remotecommands/remotecommandsplugin.h remotecommandsinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/remotekeyboard/remotekeyboardplugin.h remotekeyboardinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/sms/smsplugin.h smsinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/sms/conversationsdbusinterface.h conversationsinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/share/shareplugin.h shareinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/remotesystemvolume/remotesystemvolumeplugin.h remotesystemvolumeinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/bigscreen/bigscreenplugin.h bigscreeninterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/virtualmonitor/virtualmonitorplugin.h virtualmonitorinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/photo/photoplugin.h photointerface)

add_library(kdeconnectinterfaces ${libkdeconnect_SRC})
set_target_properties(kdeconnectinterfaces PROPERTIES
    VERSION ${KDECONNECT_VERSION}
    SOVERSION ${KDECONNECT_VERSION_MAJOR}
)

generate_export_header(kdeconnectinterfaces EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectinterfaces_export.h BASE_NAME KDEConnectInterfaces)

target_link_libraries(kdeconnectinterfaces
LINK_PUBLIC
    Qt${QT_MAJOR_VERSION}::Gui
    Qt${QT_MAJOR_VERSION}::DBus
    KF${QT_MAJOR_VERSION}::ConfigCore
LINK_PRIVATE
    kdeconnectcore
    KF${QT_MAJOR_VERSION}::I18n
    kdeconnectcore
    KF${QT_MAJOR_VERSION}::CoreAddons
)

install(TARGETS kdeconnectinterfaces EXPORT kdeconnectLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
