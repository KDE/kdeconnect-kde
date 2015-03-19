add_definitions(-DTRANSLATION_DOMAIN="kdeconnect-fileitemaction")
include_directories(${CMAKE_SOURCE_DIR})

add_library(kdeconnectfiletiemaction MODULE sendfileitemaction.cpp)
target_link_libraries(kdeconnectfiletiemaction KF5::KIOWidgets KF5::I18n kdeconnectinterfaces)
install(TARGETS kdeconnectfiletiemaction DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kdeconnectsendfile.desktop DESTINATION ${SERVICES_INSTALL_DIR})
