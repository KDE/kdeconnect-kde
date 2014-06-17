include_directories(${CMAKE_SOURCE_DIR})

kde4_add_executable(kdeconnectd kdeconnectd.cpp)
target_link_libraries(kdeconnectd kdeconnectcore ${KDE4_KDEUI_LIBS})

kde4_add_plugin(kded_kdeconnect kded.cpp)

target_link_libraries(kded_kdeconnect ${KDE4_KDECORE_LIBS})

install(TARGETS kdeconnectd DESTINATION ${LIBEXEC_INSTALL_DIR})
install(TARGETS kded_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kdeconnect.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)

#TODO: Split this into several files for core and for each plugin
install(FILES kdeconnect.notifyrc DESTINATION ${DATA_INSTALL_DIR}/kdeconnect)

