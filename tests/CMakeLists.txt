find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS Test)

set(kdeconnect_libraries
    kdeconnectcore
    kdeconnectversion
    KF5::I18n
    KF5::KIOWidgets
    Qt::DBus
    Qt::Network
    Qt::Test
)

ecm_add_test(pluginloadtest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
ecm_add_test(sendfiletest.cpp LINK_LIBRARIES ${kdeconnect_libraries})
