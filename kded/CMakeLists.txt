include_directories(${CMAKE_SOURCE_DIR})

kde4_add_executable(kdeconnectd kdeconnectd.cpp)
target_link_libraries(kdeconnectd kdeconnectcore ${KDE4_KDEUI_LIBS})
install(TARGETS kdeconnectd DESTINATION ${LIBEXEC_INSTALL_DIR})

kde4_add_plugin(kded_kdeconnect kded.cpp)
target_link_libraries(kded_kdeconnect ${KDE4_KDECORE_LIBS})
install(TARGETS kded_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kdeconnect.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)

include(DbusActivationMacros)
dbus_add_activation_service(org.kde.kdeconnect.service.in)
