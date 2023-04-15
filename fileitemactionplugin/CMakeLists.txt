add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-fileitemaction")

set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER kdeconnect_fileitemaction_debug.h
    IDENTIFIER KDECONNECT_FILEITEMACTION CATEGORY_NAME kdeconnect.fileitemaction
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (fileitemaction)")

add_library(kdeconnectfileitemaction MODULE sendfileitemaction.cpp ${debug_file_SRCS})
target_link_libraries(
    kdeconnectfileitemaction
    KF${QT_MAJOR_VERSION}::KIOWidgets
    KF${QT_MAJOR_VERSION}::I18n
    kdeconnectinterfaces
    kdeconnectcore
)
install(TARGETS kdeconnectfileitemaction DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kfileitemaction)
