include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR})

set(kdeconnectdeclarativeplugin_SRC
    kdeconnectdeclarativeplugin.cpp
    responsewaiter.cpp
    objectfactory.cpp
)

set(qml_SRC
    qml/RemoteKeyboard.qml
    qml/PluginChecker.qml
    qml/DBusProperty.qml
)

add_library(kdeconnectdeclarativeplugin SHARED ${kdeconnectdeclarativeplugin_SRC})
target_link_libraries(kdeconnectdeclarativeplugin
    Qt5::Qml
    kdeconnectinterfaces
)
install(TARGETS kdeconnectdeclarativeplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/kdeconnect)
install(FILES qmldir ${qml_SRC} DESTINATION ${QML_INSTALL_DIR}/org/kde/kdeconnect)
