add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-kcm")

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR})

set(kcm_SRCS
    kcm.cpp
)

set(kcm_wrapper_SRCS main.cpp)
add_executable(kdeconnect-settings ${kcm_wrapper_SRCS})
target_link_libraries(kdeconnect-settings KF5::I18n KF5::KCMUtils)

ki18n_wrap_ui(kcm_SRCS kcm.ui)

add_library(kcm_kdeconnect MODULE ${kcm_SRCS})

target_link_libraries(kcm_kdeconnect
    Qt5::Core
    Qt5::Gui
    KF5::I18n
    KF5::KCMUtils
    kdeconnectinterfaces
)

install(TARGETS kcm_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})
install(TARGETS kdeconnect-settings ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES org.kde.kdeconnect.kcm.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES kcm_kdeconnect.desktop DESTINATION ${SERVICES_INSTALL_DIR})
