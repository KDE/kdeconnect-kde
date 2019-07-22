include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR})

set(kdeconnectdeclarativeplugin_SRC
    kdeconnectdeclarativeplugin.cpp
    responsewaiter.cpp
    objectfactory.cpp
)

qt5_add_resources(kdeconnectdeclarativeplugin_SRC resources.qrc)

add_library(kdeconnectdeclarativeplugin SHARED ${kdeconnectdeclarativeplugin_SRC})
target_link_libraries(kdeconnectdeclarativeplugin
    Qt5::Qml
    kdeconnectinterfaces
)
install(TARGETS kdeconnectdeclarativeplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/kdeconnect)
install(FILES qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/kdeconnect)
