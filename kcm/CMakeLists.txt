add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-kcm")


add_library(kcm_kdeconnect MODULE kcm.cpp)

ki18n_wrap_ui(kcm_kdeconnect kcm.ui)
kcoreaddons_desktop_to_json(kcm_kdeconnect "kcm_kdeconnect.desktop")

target_link_libraries(kcm_kdeconnect
    Qt5::Core
    Qt5::Gui
    KF5::I18n
    KF5::KCMUtils
    kdeconnectinterfaces
    kdeconnectversion
)

install(TARGETS kcm_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kcm_kdeconnect.desktop DESTINATION ${SERVICES_INSTALL_DIR})
