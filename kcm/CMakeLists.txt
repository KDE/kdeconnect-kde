
include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR})

set(kcm_SRCS
    kcm.cpp
    devicessortproxymodel.cpp
    kdebugnamespace.cpp
)

kde4_add_ui_files(kcm_SRCS kcm.ui)

kde4_add_plugin(kcm_kdeconnect ${kcm_SRCS})

add_dependencies(kcm_kdeconnect libkdeconnect)

target_link_libraries(kcm_kdeconnect
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${KDE4_KDEUI_LIBRARY}
    ${KDE4_KCMUTILS_LIBS}
    kdeconnect
)

install(TARGETS kcm_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES kcm_kdeconnect.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES kdeconnect.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
