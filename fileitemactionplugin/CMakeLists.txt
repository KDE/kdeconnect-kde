include_directories(${CMAKE_SOURCE_DIR})

kde4_add_plugin(kdeconnectfiletiemaction sendfileitemaction.cpp)
target_link_libraries(kdeconnectfiletiemaction ${KDE4_KIO_LIBS} kdeconnectinterfaces)
install(TARGETS kdeconnectfiletiemaction DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kdeconnectsendfile.desktop DESTINATION ${SERVICES_INSTALL_DIR})
