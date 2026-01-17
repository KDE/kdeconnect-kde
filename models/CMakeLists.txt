add_definitions(-DTRANSLATION_DOMAIN=\"kdeconnect-interfaces\")

set(models_SRC
    devicesmodel.cpp
    notificationsmodel.cpp
    devicessortproxymodel.cpp
    conversationmessage.cpp
    remotecommandsmodel.cpp
    remotesinksmodel.cpp
    devicespluginfilterproxymodel.cpp
    pluginmodel.cpp
    commandsmodel.cpp
)

add_library(kdeconnectmodels STATIC)
set_property(TARGET kdeconnectmodels PROPERTY POSITION_INDEPENDENT_CODE ON)

target_sources(kdeconnectmodels PRIVATE ${models_SRC})

ecm_qt_declare_logging_category(kdeconnectmodels
    HEADER models_debug.h
    IDENTIFIER KDECONNECT_MODELS CATEGORY_NAME kdeconnect.models
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (models)")


set_target_properties(kdeconnectmodels PROPERTIES
    VERSION ${KDECONNECT_VERSION}
    SOVERSION ${KDECONNECT_VERSION_MAJOR}
)

generate_export_header(kdeconnectmodels EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectmodels_export.h BASE_NAME kdeconnectmodels)

target_link_libraries(kdeconnectmodels
LINK_PUBLIC
    Qt::Gui
    Qt::DBus
    KF6::ConfigCore
LINK_PRIVATE
    KF6::I18n
    kdeconnectcore
    kdeconnectdbusinterfaces
    KF6::CoreAddons
)

install(TARGETS kdeconnectmodels EXPORT kdeconnectLibraryTargets ${KDECONNECT_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
