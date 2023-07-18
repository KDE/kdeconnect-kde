add_definitions(-DTRANSLATION_DOMAIN=\"kdeconnect-cli\")

add_executable(kdeconnect-cli kdeconnect-cli.cpp)
ecm_mark_nongui_executable(kdeconnect-cli)

target_link_libraries(kdeconnect-cli
    kdeconnectinterfaces
    kdeconnectcore
    kdeconnectversion
    KF5::CoreAddons
    KF5::I18n
)

install(TARGETS kdeconnect-cli ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES kdeconnect.zsh RENAME _kdeconnect DESTINATION ${KDE_INSTALL_ZSHAUTOCOMPLETEDIR})
