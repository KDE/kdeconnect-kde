find_package(LibFakeKey)
set_package_properties(LibFakeKey PROPERTIES DESCRIPTION "fake key events"
                       URL "https://www.yoctoproject.org/tools-resources/projects/matchbox"
                       TYPE OPTIONAL
                       PURPOSE "Needed for the remote mousepad plugin"
                      )

if (LibFakeKey_FOUND)
    find_package(XTest REQUIRED)
    find_package(X11 REQUIRED)
    find_package(Qt5X11Extras REQUIRED)
endif()
find_package(KF5Wayland 5.3.90)

set(HAVE_X11 ${LibFakeKey_FOUND})
set(HAVE_WAYLAND ${KF5Wayland_FOUND})
configure_file(config-mousepad.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-mousepad.h )

kdeconnect_add_plugin(kdeconnect_mousepad JSON kdeconnect_mousepad.json SOURCES mousepadplugin.cpp)

if (HAVE_X11)
    include_directories(${XTEST_INCLUDE_DIRS} ${X11_INCLUDE_DIR} ${LibFakeKey_INCLUDE_DIRS})
endif()

target_link_libraries(kdeconnect_mousepad kdeconnectcore Qt5::Gui KF5::I18n)

if(HAVE_WAYLAND)
    target_link_libraries(kdeconnect_mousepad KF5::WaylandClient)
endif()
if(HAVE_X11)
    target_link_libraries(kdeconnect_mousepad Qt5::X11Extras ${X11_LIBRARIES} ${XTEST_LIBRARIES} ${LibFakeKey_LIBRARIES})
endif()
