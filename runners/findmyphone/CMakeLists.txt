add_definitions(-DTRANSLATION_DOMAIN="plasma_runner_kdeconnect_findmyphone")

add_library(krunner_kdeconnect_findmyphone MODULE findmyphonerunner.cpp)
target_link_libraries(krunner_kdeconnect_findmyphone PUBLIC KF5::Runner KF5::I18n kdeconnectinterfaces)

install(TARGETS krunner_kdeconnect_findmyphone DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES plasma-runner-kdeconnect-findmyphone.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
