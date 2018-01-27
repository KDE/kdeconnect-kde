add_definitions(-DTRANSLATION_DOMAIN=\"kdeconnect-cli\")

add_executable(kdeconnect-cli kdeconnect-cli.cpp)
ecm_mark_nongui_executable(kdeconnect-cli)

target_link_libraries(kdeconnect-cli
    kdeconnectinterfaces
    KF5::CoreAddons
    KF5::I18n
)

install(TARGETS kdeconnect-cli ${INSTALL_TARGETS_DEFAULT_ARGS})
