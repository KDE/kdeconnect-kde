find_package(KF5 REQUIRED COMPONENTS KIO)

add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-fileitemaction")
include_directories(${CMAKE_SOURCE_DIR})

add_library(kdeconnectfileitemaction MODULE sendfileitemaction.cpp)
target_link_libraries(kdeconnectfileitemaction
    KF5::KIOWidgets
    KF5::I18n
    kdeconnectinterfaces
)
install(TARGETS kdeconnectfileitemaction DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kdeconnectsendfile.desktop DESTINATION ${SERVICES_INSTALL_DIR})
