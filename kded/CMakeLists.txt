include_directories(${CMAKE_SOURCE_DIR})

add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-kded")

configure_file(config-kded.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kded.h)
add_library(kded_kdeconnect MODULE kded.cpp)
target_link_libraries(kded_kdeconnect KF5::Service KF5::DBusAddons)
install(TARGETS kded_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kdeconnect.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)

add_executable(kdeconnectd kdeconnectd.cpp)
target_link_libraries(kdeconnectd kdeconnectcore KF5::DBusAddons KF5::ConfigWidgets)
install(TARGETS kdeconnectd DESTINATION ${LIBEXEC_INSTALL_DIR})

include(DbusActivationMacros)
dbus_add_activation_service(org.kde.kdeconnect.service.in)
