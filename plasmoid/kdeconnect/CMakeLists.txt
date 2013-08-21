include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR})




#DataEngine
set(plasma_engine_kdeconnect_SRC
    kdeconnectengine.cpp
    plasmoidadaptor.cpp
)
kde4_add_plugin(plasma_engine_kdeconnect ${plasma_engine_kdeconnect_SRC} )
qt4_add_dbus_adaptor(kdeconnect_engine_SRCS org.kde.kdeconnect.plasmoid.xml kdeconnectengine.h KdeConnectEngine )
target_link_libraries(plasma_engine_kdeconnect
    ${KDE4_PLASMA_LIBS}
    ${KDE4_KDECORE_LIBS}
)
install(TARGETS plasma_engine_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})


#DeclarativePlugin
set(kdeconnectdeclarativeplugin_SRC
    kdeconnectdeclarativeplugin.cpp
)
kde4_add_library(kdeconnectdeclarativeplugin SHARED ${kdeconnectdeclarativeplugin_SRC})
add_dependencies(kdeconnectdeclarativeplugin libkdeconnect)
target_link_libraries(kdeconnectdeclarativeplugin
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KDECORE_LIBS}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTDECLARATIVE_LIBRARY}
    kdeconnect
)
install(TARGETS kdeconnectdeclarativeplugin DESTINATION ${IMPORTS_INSTALL_DIR}/org/kde/kdeconnect)
install(FILES qmldir DESTINATION ${IMPORTS_INSTALL_DIR}/org/kde/kdeconnect)


#Desktop file
install(FILES plasma-dataengine-kdeconnect.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
