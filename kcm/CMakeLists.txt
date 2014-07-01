
include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR})

set(kcm_SRCS
    kcm.cpp
    devicessortproxymodel.cpp
)

qt5_wrap_ui(kcm_SRCS kcm.ui)

add_library(kcm_kdeconnect MODULE ${kcm_SRCS})

target_link_libraries(kcm_kdeconnect
    Qt5::Core
    Qt5::Gui
    KF5::KDELibs4Support
    KF5::KCMUtils
    kdeconnectinterfaces
)

install(TARGETS kcm_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES kcm_kdeconnect.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES kdeconnect.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
