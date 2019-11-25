add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-fileitemaction")

add_library(kdeconnectfileitemaction MODULE sendfileitemaction.cpp)
target_link_libraries(
    kdeconnectfileitemaction
    KF5::KIOWidgets
    KF5::I18n
    kdeconnectinterfaces
    kdeconnectcore
)
install(TARGETS kdeconnectfileitemaction DESTINATION ${PLUGIN_INSTALL_DIR}/kf5/kfileitemaction)
