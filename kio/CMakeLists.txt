include_directories(${CMAKE_SOURCE_DIR})

set(kio_kdeconnect_PART_SRCS
   kiokdeconnect.cpp
   kdebugnamespace.cpp)

kde4_add_plugin(kio_kdeconnect ${kio_kdeconnect_PART_SRCS})

target_link_libraries(kio_kdeconnect
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KIO_LIBRARY}
    ${KDE4_KDEUI_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    kdeconnectinterfaces
)

########### install files ###############
install(TARGETS kio_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kdeconnect.protocol DESTINATION ${SERVICES_INSTALL_DIR})
