include_directories(
    ${QJSON_INCLUDE_DIR}
    ${QCA2_INCLUDE_DIR}
    ${KDEConnectCore_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
)

set(kdeconnect_libraries
    KF5::KIOWidgets
    Qt5::Network
    ${QJSON_LIBRARIES}
    ${QCA2_LIBRARIES}
    Qt5::Test
    kdeconnectcore
)

ecm_add_test(networkpackagetests.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(testsocketlinereader.cpp ../core/backends/lan/socketlinereader.cpp TEST_NAME testsocketlinereader LINK_LIBRARIES ${kdeconnect_libraries})
