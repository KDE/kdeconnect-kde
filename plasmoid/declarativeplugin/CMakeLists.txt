include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR})

set(kdeconnectdeclarativeplugin_SRC
    kdeconnectdeclarativeplugin.cpp
    responsewaiter.cpp
)

set(kdeconnectdeclarativeplugin_MOC
  objectfactory.h
)

qt5_wrap_cpp(kdeconnectdeclarativeplugin_SRC ${kdeconnectdeclarativeplugin_MOC})

add_library(kdeconnectdeclarativeplugin SHARED ${kdeconnectdeclarativeplugin_SRC})
target_link_libraries(kdeconnectdeclarativeplugin
    Qt5::Qml
    kdeconnectinterfaces
)
install(TARGETS kdeconnectdeclarativeplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/kdeconnect)
install(FILES qmldir qml/PluginChecker.qml qml/DBusProperty.qml DESTINATION ${QML_INSTALL_DIR}/org/kde/kdeconnect)
