add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-kio")

add_library(kio_kdeconnect MODULE kiokdeconnect.cpp)
ecm_qt_declare_logging_category(kio_kdeconnect
    HEADER kdeconnectkio_debug.h
    IDENTIFIER KDECONNECT_KIO CATEGORY_NAME kdeconnect.kio
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (kio)")


target_link_libraries(kio_kdeconnect
    kdeconnectinterfaces
    Qt::Core
    Qt::Network
    KF${QT_MAJOR_VERSION}::KIOCore
    KF${QT_MAJOR_VERSION}::I18n
)

########### install files ###############
set_target_properties(kio_kdeconnect PROPERTIES OUTPUT_NAME "kdeconnect")
set_target_properties(kio_kdeconnect PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf5/kio")
install(TARGETS kio_kdeconnect DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)
