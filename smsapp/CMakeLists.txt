qt5_add_resources(KCSMS_SRCS resources.qrc)

find_package(KF5People)

set(smshelper_debug_file_SRCS)
ecm_qt_declare_logging_category(
    smshelper_debug_file_SRCS HEADER smshelper_debug.h
    IDENTIFIER KDECONNECT_SMS_SMSHELPER CATEGORY_NAME kdeconnect.sms.smshelper
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (smshelper)")

set(sms_debug_files_SRCS)
ecm_qt_declare_logging_category(
    sms_debug_files_SRCS HEADER sms_conversation_debug.h
    IDENTIFIER KDECONNECT_SMS_CONVERSATION_MODEL CATEGORY_NAME kdeconnect.sms.conversation
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (sms conversation model)")

ecm_qt_declare_logging_category(
    sms_debug_files_SRCS HEADER sms_conversations_list_debug.h
    IDENTIFIER KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL CATEGORY_NAME kdeconnect.sms.conversations_list
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (sms conversations list)")

add_library(kdeconnectsmshelper
	smshelper.cpp
	gsmasciimap.cpp
        ${smshelper_debug_file_SRCS}
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
    Qt5::Qml
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
    ${KCSMS_SRCS}
    ${sms_debug_files_SRCS})

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
