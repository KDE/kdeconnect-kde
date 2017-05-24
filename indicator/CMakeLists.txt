find_package(KF5Notifications REQUIRED)
find_package(KF5KCMUtils REQUIRED)

add_executable(kdeconnect-indicator main.cpp deviceindicator.cpp)
target_link_libraries(kdeconnect-indicator Qt5::Widgets KF5::CoreAddons KF5::I18n KF5::Notifications KF5::KCMUtils kdeconnectinterfaces)

install(TARGETS kdeconnect-indicator ${INSTALL_TARGETS_DEFAULT_ARGS})
