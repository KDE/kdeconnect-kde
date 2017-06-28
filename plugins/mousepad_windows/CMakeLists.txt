kdeconnect_add_plugin(kdeconnect_mousepad JSON kdeconnect_mousepad.json SOURCES mousepadplugin_windows.cpp)
target_link_libraries(kdeconnect_mousepad kdeconnectcore Qt5::Gui KF5::I18n)
