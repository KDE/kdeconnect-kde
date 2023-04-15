add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-kio")

set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER kdeconnectkio_debug.h
    IDENTIFIER KDECONNECT_KIO CATEGORY_NAME kdeconnect.kio
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (kio)")

set(kio_kdeconnect_PART_SRCS
   kiokdeconnect.cpp
   ${debug_file_SRCS}
)

add_library(kio_kdeconnect MODULE ${kio_kdeconnect_PART_SRCS})

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
