qt5_add_resources(KCSMS_SRCS resources.qrc)

find_package(KF5People)

add_library(kdeconnectsmshelper
	smshelper.cpp
	gsmasciimap.cpp
)

set_target_properties(kdeconnectsmshelper PROPERTIES
    VERSION ${KDECONNECT_VERSION}
    SOVERSION ${KDECONNECT_VERSION_MAJOR}
)

generate_export_header(kdeconnectsmshelper EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectsms_export.h BASE_NAME KDEConnectSmsAppLib)
target_include_directories(kdeconnectsmshelper PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(kdeconnectsmshelper
PUBLIC
    Qt5::Core
    Qt5::DBus
    KF5::People
    kdeconnectinterfaces
)

# If ever this library is actually used by someone else, we should export these headers
set(libkdeconnectsmshelper_HEADERS
    smshelper.h
    ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectsms_export.h
)

add_executable(kdeconnect-sms
    main.cpp
    conversationlistmodel.cpp
    conversationmodel.cpp
    conversationssortfilterproxymodel.cpp
    ${KCSMS_SRCS})

target_include_directories(kdeconnect-sms PUBLIC ${CMAKE_BINARY_DIR})

target_link_libraries(kdeconnect-sms
    kdeconnectsmshelper
    kdeconnectinterfaces
    Qt5::Quick
    Qt5::Widgets
    KF5::CoreAddons
    KF5::DBusAddons
    KF5::I18n
    KF5::People
    )

install(TARGETS kdeconnect-sms ${INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS kdeconnectsmshelper ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
install(PROGRAMS org.kde.kdeconnect.sms.desktop DESTINATION ${KDE_INSTALL_APPDIR})
