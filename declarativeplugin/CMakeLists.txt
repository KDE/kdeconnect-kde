ecm_add_qml_module(kdeconnectdeclarativeplugin URI "org.kde.kdeconnect" VERSION 1.0)

target_sources(kdeconnectdeclarativeplugin PRIVATE
    kdeconnectdeclarativeplugin.cpp
    keylistener.cpp
    responsewaiter.cpp
    objectfactory.cpp
    pointerlocker.cpp
)

ecm_target_qml_sources(kdeconnectdeclarativeplugin
    SOURCES
    qml/DBusProperty.qml
    qml/PluginChecker.qml
    qml/RemoteKeyboard.qml
    VERSION 1.0)

if(UNIX AND NOT APPLE)
    if (Qt6_VERSION VERSION_GREATER_EQUAL "6.8.0")
        set(private_code_option  "PRIVATE_CODE")
    endif()
    qt6_generate_wayland_protocol_client_sources(kdeconnectdeclarativeplugin FILES
        ${WaylandProtocols_DATADIR}/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml
        ${private_code_option}
    )
    qt6_generate_wayland_protocol_client_sources(kdeconnectdeclarativeplugin FILES
        ${WaylandProtocols_DATADIR}/unstable/relative-pointer/relative-pointer-unstable-v1.xml
        ${private_code_option}
    )

    target_sources(kdeconnectdeclarativeplugin PRIVATE ${wayland_SRCS})
    target_link_libraries(kdeconnectdeclarativeplugin PRIVATE Wayland::Client Qt::WaylandClient Qt::GuiPrivate)
    target_sources(kdeconnectdeclarativeplugin PUBLIC pointerlockerwayland.cpp)
endif()

target_link_libraries(kdeconnectdeclarativeplugin PRIVATE Qt::Quick kdeconnectdbusinterfaces kdeconnectmodels kdeconnectcore)

ecm_finalize_qml_module(kdeconnectdeclarativeplugin DESTINATION ${KDE_INSTALL_QMLDIR})
