set(kdeconnect_share_SRCS
    shareplugin.cpp
    autoclosingqfile.cpp
)

add_library(kdeconnect_share MODULE ${kdeconnect_share_SRCS})

target_link_libraries(kdeconnect_share kdeconnectcore ${KDE4_KFILE_LIBS} KF5::KIOWidgets)

install(TARGETS kdeconnect_share DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_share.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


#######################################
# Config

set( kdeconnect_share_config_SRCS share_config.cpp )
qt5_wrap_ui( kdeconnect_share_config_SRCS share_config.ui )

add_library(kdeconnect_share_config MODULE ${kdeconnect_share_config_SRCS} )
target_link_libraries( kdeconnect_share_config
    
    KF5::KDELibs4Support
    KF5::KIOWidgets
)

install(TARGETS kdeconnect_share_config DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_share_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
