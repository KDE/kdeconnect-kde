find_package(KF5 COMPONENTS Declarative)

qt5_add_resources(kcapp_SRCS resources.qrc)

add_executable(kcapp main.cpp ${kcapp_SRCS})
target_link_libraries(kcapp Qt5::Quick Qt5::Widgets KF5::Declarative KF5::CoreAddons KF5::I18n)

install(TARGETS kcapp ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kdeconnect.app.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
