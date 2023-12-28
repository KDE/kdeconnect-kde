add_definitions(-DTRANSLATION_DOMAIN=\"kdeconnect-interfaces\")

# geninterface generates a Qt class that is an interface to DBus
# source_file can a DBus XML definition or QObject class header that is exposed through DBus
# output_basename is the path and filename that will be generated (.h and .cpp will be appended to this)
# The new targets are appended to the ${interfaces_SRC} variable in the parent scope.
function(geninterface source_file output_basename)
    if(${source_file} MATCHES ".*\.xml$")
        set(xml_file ${source_file})
    else()
        set(xml_file ${CMAKE_CURRENT_BINARY_DIR}/${output_basename}.xml)
        qt_generate_dbus_interface(${source_file} ${xml_file})
    endif()

    # Fixes "redefinition of 'MediaPlayer2' as different kind of symbol"
    set_source_files_properties(${xml_file} PROPERTIES NO_NAMESPACE true)

    qt_add_dbus_interface(generated_sources ${xml_file} ${output_basename})

    # Hack to append clazy:skip to generated files
    set(fake_file ${output_basename}_replaced)
    add_custom_command(OUTPUT ${fake_file}
        COMMAND ${CMAKE_COMMAND} -E echo "// clazy:skip" >> ${output_basename}.h
        COMMAND ${CMAKE_COMMAND} -E touch ${fake_file}
        DEPENDS ${output_basename}.h
    )

    set(interfaces_SRC ${interfaces_SRC} ${generated_sources} ${fake_file} PARENT_SCOPE)
endfunction()

set(interfaces_SRC
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
)

# KDE Connect interfaces
geninterface(${PROJECT_SOURCE_DIR}/core/daemon.h generated/daemoninterface)
geninterface(${PROJECT_SOURCE_DIR}/core/device.h generated/deviceinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/battery/batteryplugin.h generated/batteryinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/connectivity-report/connectivity_reportplugin.h generated/connectivityinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/sftp/sftpplugin.h generated/devicesftpinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/notifications/notificationsplugin.h generated/devicenotificationsinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/findmyphone/findmyphoneplugin.h generated/devicefindmyphoneinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/notifications/notification.h generated/notificationinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/mprisremote/mprisremoteplugin.h generated/mprisremoteinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/remotecontrol/remotecontrolplugin.h generated/remotecontrolinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/lockdevice/lockdeviceplugin.h generated/lockdeviceinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/remotecommands/remotecommandsplugin.h generated/remotecommandsinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/remotekeyboard/remotekeyboardplugin.h generated/remotekeyboardinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/sms/smsplugin.h generated/smsinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/sms/conversationsdbusinterface.h generated/conversationsinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/share/shareplugin.h generated/shareinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/remotesystemvolume/remotesystemvolumeplugin.h generated/remotesystemvolumeinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/bigscreen/bigscreenplugin.h generated/bigscreeninterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/virtualmonitor/virtualmonitorplugin.h generated/virtualmonitorinterface)
geninterface(${PROJECT_SOURCE_DIR}/plugins/clipboard/clipboardplugin.h generated/deviceclipboardinterface)

# System dbus interfaces
if (UNIX AND NOT APPLE)
    geninterface(systeminterfaces/org.freedesktop.login1.xml generated/systeminterfaces/login1)
    geninterface(systeminterfaces/org.freedesktop.ScreenSaver.xml generated/systeminterfaces/screensaver)
    geninterface(systeminterfaces/org.freedesktop.DBus.Properties.xml generated/systeminterfaces/dbusproperties)
    geninterface(systeminterfaces/org.mpris.MediaPlayer2.Player.xml generated/systeminterfaces/mprisplayer)
    geninterface(systeminterfaces/org.mpris.MediaPlayer2.xml generated/systeminterfaces/mprisroot)
    geninterface(systeminterfaces/org.freedesktop.portal.RemoteDesktop.xml generated/systeminterfaces/remotedesktop)
endif()

add_library(kdeconnectinterfaces STATIC)
set_property(TARGET kdeconnectinterfaces PROPERTY POSITION_INDEPENDENT_CODE ON)

target_sources(kdeconnectinterfaces PRIVATE ${interfaces_SRC})

ecm_qt_declare_logging_category(kdeconnectinterfaces
    HEADER interfaces_conversation_message_debug.h
    IDENTIFIER CONVERSATION_MESSAGE_LOGGING_CATEGORY CATEGORY_NAME kdeconnect.interfaces.conversationmessage
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (conversation message)")

ecm_qt_declare_logging_category(kdeconnectinterfaces
    HEADER interfaces_debug.h
    IDENTIFIER KDECONNECT_INTERFACES CATEGORY_NAME kdeconnect.interfaces
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (interfaces)")


set_target_properties(kdeconnectinterfaces PROPERTIES
    VERSION ${KDECONNECT_VERSION}
    SOVERSION ${KDECONNECT_VERSION_MAJOR}
)

generate_export_header(kdeconnectinterfaces EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectinterfaces_export.h BASE_NAME KDEConnectInterfaces)

target_link_libraries(kdeconnectinterfaces
LINK_PUBLIC
    Qt::Gui
    Qt::DBus
    KF6::ConfigCore
LINK_PRIVATE
    kdeconnectcore
    KF6::I18n
    kdeconnectcore
    KF6::CoreAddons
)

install(TARGETS kdeconnectinterfaces EXPORT kdeconnectLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
