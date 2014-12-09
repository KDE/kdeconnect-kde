include_directories(${CMAKE_SOURCE_DIR})

add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-kded")

configure_file(config-kded.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kded.h)

add_executable(kdeconnectd kdeconnectd.cpp)
target_link_libraries(kdeconnectd kdeconnectcore KF5::DBusAddons KF5::ConfigWidgets)

set(NEW_DAEMON_STARTUP_DESC "WIP: Start kdeconnectd without kded")
option(WITH_AUTOSTART NEW_DAEMON_STARTUP_DESC Off)
add_feature_info(WITH_AUTOSTART WITH_AUTOSTART NEW_DAEMON_STARTUP_DESC)

if (NOT WITH_AUTOSTART)
    add_library(kded_kdeconnect MODULE kded.cpp)
    target_link_libraries(kded_kdeconnect KF5::Service KF5::DBusAddons)
    install(TARGETS kded_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})
    install(FILES kdeconnect.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
else()
    configure_file(kdeconnectd.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectd.desktop)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectd.desktop DESTINATION ${AUTOSTART_INSTALL_DIR})
endif()

install(TARGETS kdeconnectd DESTINATION ${LIBEXEC_INSTALL_DIR})

include(DbusActivationMacros)
dbus_add_activation_service(org.kde.kdeconnect.service.in)