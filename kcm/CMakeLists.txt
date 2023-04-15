add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-kcm")


kcoreaddons_add_plugin(kcm_kdeconnect SOURCES kcm.cpp INSTALL_NAMESPACE plasma/kcms/systemsettings_qwidgets)
kcmutils_generate_desktop_file(kcm_kdeconnect)

ki18n_wrap_ui(kcm_kdeconnect kcm.ui)

target_link_libraries(kcm_kdeconnect
    Qt::Core
    Qt::Gui
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::KCMUtils
    kdeconnectinterfaces
    kdeconnectversion
)
