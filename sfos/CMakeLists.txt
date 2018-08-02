find_package(Qt5 5.2 REQUIRED COMPONENTS DBus)

pkg_check_modules(NNQT5 REQUIRED nemonotifications-qt5)
include_directories(${NNQT5_INCLUDE_DIRS})

set(kdeconnectsfos_SRCS
    kdeconnect-sfos.cpp
)

add_executable(kdeconnect-sfos ${kdeconnectsfos_SRCS})
target_link_libraries(kdeconnect-sfos Qt5::Quick sailfishapp)

install(TARGETS kdeconnect-sfos ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS kdeconnect-sfos.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
INSTALL( DIRECTORY qml DESTINATION ${SHARE_INSTALL_PREFIX}/kdeconnect-sfos/ )

#Daemon
add_executable(kdeconnectd sailfishdaemon.cpp)
target_link_libraries(kdeconnectd kdeconnectcore KF5::DBusAddons ${NNQT5_LIBRARIES} KF5::I18n)

configure_file(kdeconnectd.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectd.desktop)
configure_file(org.kde.kdeconnect.service.in ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.service)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectd.desktop DESTINATION ${AUTOSTART_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.service DESTINATION ${DBUS_SERVICES_INSTALL_DIR})
install(TARGETS kdeconnectd DESTINATION ${LIBEXEC_INSTALL_DIR})
