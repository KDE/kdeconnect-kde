ecm_add_qml_module(kdeconnectdeclarativeplugin URI "org.kde.kdeconnect" VERSION 1.0)

target_sources(kdeconnectdeclarativeplugin PRIVATE
    kdeconnectdeclarativeplugin.cpp
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

if (TARGET KF5::WaylandClient)
    target_sources(kdeconnectdeclarativeplugin PRIVATE pointerlockerwayland.cpp)
    target_link_libraries(kdeconnectdeclarativeplugin PRIVATE KF5::WaylandClient)
    target_compile_definitions(kdeconnectdeclarativeplugin PRIVATE -DWITH_WAYLAND=1)
else()
    target_compile_definitions(kdeconnectdeclarativeplugin PRIVATE -DWITH_WAYLAND=0)
endif()

target_link_libraries(kdeconnectdeclarativeplugin PRIVATE Qt::Quick kdeconnectinterfaces kdeconnectcore)

ecm_finalize_qml_module(kdeconnectdeclarativeplugin DESTINATION ${KDE_INSTALL_QMLDIR})
