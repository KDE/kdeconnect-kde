if(APPLE)
    kirigami_package_breeze_icons(ICONS
        network-connect dialog-ok dialog-cancel dialog-ok-apply entry-edit input-mouse-click-left input-mouse-click-middle input-mouse-click-right
        media-skip-backward media-playback-pause media-playback-start media-skip-forward view-fullscreen
        document-edit player-volume-muted player-volume view-refresh help-about
    )
endif()

qt5_add_resources(kdeconnect_SRCS resources.qrc)

add_executable(kdeconnect main.cpp ${kdeconnect_SRCS})
target_link_libraries(kdeconnect Qt5::Quick Qt5::Widgets KF5::CoreAddons KF5::I18n)

install(TARGETS kdeconnect ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kdeconnect.app.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
