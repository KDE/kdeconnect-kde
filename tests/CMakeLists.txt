set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

find_package(QJSON REQUIRED)
find_package(QCA2 REQUIRED)

include_directories(
    ${QJSON_INCLUDE_DIR}
    ${QCA2_INCLUDE_DIR}
)

set(kdeconnect_libraries
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${QJSON_LIBRARIES}
    ${QCA2_LIBRARIES}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
)

#NetworkPackage
kde4_add_unit_test(kdeconnect_tests ../kded/networkpackage.cpp networkpackagetests.cpp)
target_link_libraries(kdeconnect_tests ${kdeconnect_libraries})

