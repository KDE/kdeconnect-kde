include_directories(${CMAKE_SOURCE_DIR})

add_executable(kdeconnect-cli kdeconnect-cli.cpp)

target_link_libraries(kdeconnect-cli kdeconnectinterfaces Qt5::Gui KF5::CoreAddons KF5::I18n)

install(TARGETS kdeconnect-cli ${INSTALL_TARGETS_DEFAULT_ARGS})
