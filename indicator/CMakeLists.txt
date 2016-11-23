add_executable(kdeconnect-indicator main.cpp deviceindicator.cpp)
target_link_libraries(kdeconnect-indicator Qt5::Widgets KF5::CoreAddons KF5::I18n kdeconnectinterfaces)
