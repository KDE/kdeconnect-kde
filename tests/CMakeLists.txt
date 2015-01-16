include_directories(
    ${KDEConnectCore_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
)

set(kdeconnect_libraries
    KF5::KIOWidgets
    Qt5::Network
    Qt5::Test
    kdeconnectcore
    qca-qt5
)

ecm_add_test(networkpackagetests.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(testsocketlinereader.cpp ../core/backends/lan/socketlinereader.cpp TEST_NAME testsocketlinereader LINK_LIBRARIES ${kdeconnect_libraries})
