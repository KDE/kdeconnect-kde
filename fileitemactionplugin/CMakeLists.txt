add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-fileitemaction")

add_library(kdeconnectfileitemaction MODULE sendfileitemaction.cpp)

ecm_qt_declare_logging_category(kdeconnectfileitemaction
    HEADER kdeconnect_fileitemaction_debug.h
    IDENTIFIER KDECONNECT_FILEITEMACTION CATEGORY_NAME kdeconnect.fileitemaction
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (fileitemaction)")
target_link_libraries(
    kdeconnectfileitemaction
    KF6::KIOWidgets
    KF6::I18n
    kdeconnectinterfaces
    kdeconnectcore
)
install(TARGETS kdeconnectfileitemaction DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/kfileitemaction)
