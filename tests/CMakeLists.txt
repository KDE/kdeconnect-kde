find_package(Qt5 REQUIRED COMPONENTS Test)
find_package(KF5 REQUIRED COMPONENTS KIO)

include_directories(
    ${KDEConnectCore_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
)

set(kdeconnect_libraries
    kdeconnectcore
    KF5::KIOWidgets
    Qt5::Network
    Qt5::Test
    qca-qt5
)

ecm_add_test(sendfiletest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(networkpackagetests.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(testsocketlinereader.cpp ../core/backends/lan/socketlinereader.cpp TEST_NAME testsocketlinereader LINK_LIBRARIES ${kdeconnect_libraries})
