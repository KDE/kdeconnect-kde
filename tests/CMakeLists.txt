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

#Socketlinereader
set(_testname testsocketlinereader)
qt4_generate_moc(${_testname}.cpp ${CMAKE_CURRENT_BINARY_DIR}/${_testname}.moc)
include_directories(${QT_INCLUDES} ${KDE4_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})
kde4_add_unit_test(${_testname} ${_testname}.cpp ${_testname}.moc ../kded/backends/lan/socketlinereader.cpp ../kded/kdebugnamespace.cpp)
target_link_libraries(${_testname} ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY})
