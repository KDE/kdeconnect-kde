set(kdeconnect_mousepad_SRCS
    mousepadplugin.cpp
)

find_package(XTest REQUIRED)
find_package(X11 REQUIRED)
find_package(LibFakeKey REQUIRED)
find_package(Qt5X11Extras REQUIRED)
find_package(KF5Wayland)

set(HAVE_WAYLAND ${KF5Wayland_FOUND})
configure_file(config-mousepad.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-mousepad.h )

kdeconnect_add_plugin(kdeconnect_mousepad JSON kdeconnect_mousepad.json SOURCES  ${kdeconnect_mousepad_SRCS})

include_directories(${XTEST_INCLUDE_DIRS} ${X11_INCLUDE_DIR} ${LibFakeKey_INCLUDE_DIRS})

target_link_libraries(kdeconnect_mousepad kdeconnectcore Qt5::Gui Qt5::X11Extras ${X11_LIBRARIES} ${XTEST_LIBRARIES} ${LibFakeKey_LIBRARIES} KF5::I18n)

if(HAVE_WAYLAND)
    target_link_libraries(kdeconnect_mousepad KF5::WaylandClient)
endif()
