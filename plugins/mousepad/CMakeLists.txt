set(kdeconnect_mousepad_SRCS
    mousepadplugin.cpp
)

find_package(XTest REQUIRED)
find_package(X11 REQUIRED)

add_library(kdeconnect_mousepad MODULE ${kdeconnect_mousepad_SRCS})

include_directories(${XTEST_INCLUDE_DIRS} ${X11_INCLUDE_DIR})

target_link_libraries(kdeconnect_mousepad KF5::Service kdeconnectcore Qt5::Gui ${X11_LIBRARIES} ${XTEST_LIBRARIES})

install(TARGETS kdeconnect_mousepad DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_mousepad.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
