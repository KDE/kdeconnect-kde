add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-kio")

kcoreaddons_add_plugin(kio_kdeconnect SOURCES kiokdeconnect.cpp INSTALL_NAMESPACE "kf6/kio")

set_target_properties(kio_kdeconnect PROPERTIES OUTPUT_NAME "kdeconnect")
ecm_qt_declare_logging_category(kio_kdeconnect
    HEADER kdeconnectkio_debug.h
    IDENTIFIER KDECONNECT_KIO CATEGORY_NAME kdeconnect.kio
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (kio)")


target_link_libraries(kio_kdeconnect
    kdeconnectinterfaces
    Qt::Core
    Qt::Network
    KF6::KIOCore
    KF6::I18n
)
