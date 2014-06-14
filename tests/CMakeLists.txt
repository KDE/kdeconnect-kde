include_directories(
    ${QJSON_INCLUDE_DIR}
    ${QCA2_INCLUDE_DIR}
    ${KDE4_INCLUDES}
    ${KDEConnectCore_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
)

set(kdeconnect_libraries
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
	${KDE4_KIO_LIBS}
    ${QT_QTNETWORK_LIBRARY}
    ${QJSON_LIBRARIES}
    ${QCA2_LIBRARIES}
    ${QT_QTTEST_LIBRARY}
    kdeconnectcore
)

#NetworkPackage
kde4_add_unit_test(kdeconnect_tests networkpackagetests.cpp)
target_link_libraries(kdeconnect_tests ${kdeconnect_libraries})

#Socketlinereader
kde4_add_unit_test(testsocketlinereader testsocketlinereader.cpp ../core/backends/lan/socketlinereader.cpp)
target_link_libraries(testsocketlinereader ${kdeconnect_libraries})
