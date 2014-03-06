add_subdirectory(plugins)
add_subdirectory(autotests)

find_package (QJSON 0.8.1 REQUIRED)
find_package (QCA2 REQUIRED)

include_directories(
    ${KDE4_KIO_LIBS}
    ${QJSON_INCLUDE_DIR}
    ${QCA2_INCLUDE_DIR}
)

add_subdirectory(backends/lan)
add_subdirectory(backends/loopback)

set(kded_kdeconnect_SRCS
    ${kded_kdeconnect_SRCS}

    backends/linkprovider.cpp
    backends/devicelink.cpp

    plugins/kdeconnectplugin.cpp
    plugins/pluginloader.cpp

    networkpackage.cpp
    filetransferjob.cpp
    daemon.cpp
    device.cpp
    kdebugnamespace.cpp
)

kde4_add_executable(kdeconnectd kdeconnectd.cpp ${kded_kdeconnect_SRCS})

target_link_libraries(kdeconnectd
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${QT_QTNETWORK_LIBRARY}
    ${QJSON_LIBRARIES}
    ${QCA2_LIBRARIES}
)

kde4_add_plugin(kded_kdeconnect kded.cpp ${kded_kdeconnect_SRCS})

target_link_libraries(kded_kdeconnect
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${QT_QTNETWORK_LIBRARY}
    ${QJSON_LIBRARIES}
    ${QCA2_LIBRARIES}
)

include(../macros.cmake)

generate_and_install_dbus_interface(
    kdeconnectd
    daemon.h
    org.kde.kdeconnect.daemon.xml
    OPTIONS -a
)

generate_and_install_dbus_interface(
    kdeconnectd
    device.h
    org.kde.kdeconnect.device.xml
    OPTIONS -a
)

generate_and_install_dbus_interface(
    kded_kdeconnect
    kded.h
    org.kde.kded.kdeconnect.xml
    OPTIONS -a
)

install(TARGETS kdeconnectd DESTINATION ${LIBEXEC_INSTALL_DIR})
install(TARGETS kded_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kdeconnect.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
install(FILES kdeconnect.notifyrc DESTINATION ${DATA_INSTALL_DIR}/kdeconnect)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
 
	set(kdeconnect_standalone_SRCS
		${kded_kdeconnect_SRCS}
		main.cpp
	)
	kde4_add_executable(kdeconnect_standalone ${kdeconnect_standalone_SRCS})
	target_link_libraries(kdeconnect_standalone
		${KDE4_KDECORE_LIBS}
		${KDE4_KDEUI_LIBS}
		${KDE4_KIO_LIBS}
		${QT_QTNETWORK_LIBRARY}
		${QJSON_LIBRARIES}
		${QCA2_LIBRARIES}
	)
	install(TARGETS kdeconnect_standalone ${INSTALL_TARGETS_DEFAULT_ARGS})
 
 endif()