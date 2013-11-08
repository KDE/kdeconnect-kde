set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include(KDE4Defaults)

find_package(KDE4 REQUIRED)
find_package(QJSON 0.8.1 REQUIRED)
find_package(QCA2 REQUIRED)

include_directories(
    ${QJSON_INCLUDE_DIR}
    ${QCA2_INCLUDE_DIR}
    ${KDE4_INCLUDES}
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
)

#NetworkPackage
kde4_add_unit_test(kdeconnect_tests ../kded/networkpackage.cpp ../kded/kdebugnamespace.cpp ../kded/filetransferjob.cpp networkpackagetests.cpp)
target_link_libraries(kdeconnect_tests ${kdeconnect_libraries})

