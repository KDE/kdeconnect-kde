kdeconnect_add_plugin(kdeconnect_mousepad SOURCES mousepadplugin.cpp abstractremoteinput.cpp)

if(UNIX AND NOT APPLE)
    qt_add_dbus_interface(
        SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/xdp_dbus_remotedesktop_interface.xml
        xdp_dbus_remotedesktop_interface
    )

    target_sources(kdeconnect_mousepad PUBLIC waylandremoteinput.cpp ${SRCS})

    target_sources(kdeconnect_mousepad PRIVATE ${wayland_SRCS})
    target_link_libraries(kdeconnect_mousepad Wayland::Client Qt::WaylandClient PkgConfig::XkbCommon)
    target_sources(kdeconnect_mousepad PUBLIC waylandremoteinput.cpp)

    if (WITH_X11)
        find_package(LibFakeKey REQUIRED)
        set_package_properties(LibFakeKey PROPERTIES DESCRIPTION "fake key events"
                            URL "https://github.com/dtapuska/fakekey"
                            TYPE REQUIRED
                            PURPOSE "Needed for the remote mouse input plugin"
                            )

        if (QT_MAJOR_VERSION EQUAL "5")
            find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS X11Extras)
        endif()
        find_package(XTest REQUIRED)
        find_package(X11 REQUIRED)
        target_include_directories(kdeconnect_mousepad PRIVATE ${XTEST_INCLUDE_DIRS} ${X11_INCLUDE_DIR} ${LibFakeKey_INCLUDE_DIRS})
    endif()
endif()

configure_file(config-mousepad.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-mousepad.h)

target_link_libraries(kdeconnect_mousepad kdeconnectcore Qt::Gui KF${QT_MAJOR_VERSION}::I18n)

if (WIN32)
    target_sources(kdeconnect_mousepad PUBLIC windowsremoteinput.cpp)
endif()

if(WITH_X11)
    target_sources(kdeconnect_mousepad PUBLIC x11remoteinput.cpp)
    if (QT_MAJOR_VERSION EQUAL "5")
        target_link_libraries(kdeconnect_mousepad Qt5::X11Extras)
    endif()
    target_link_libraries(kdeconnect_mousepad ${X11_LIBRARIES} ${XTEST_LIBRARIES} ${LibFakeKey_LIBRARIES})
endif()

if (APPLE)
    target_sources(kdeconnect_mousepad PUBLIC macosremoteinput.mm)
    target_link_libraries(kdeconnect_mousepad "-framework CoreGraphics" "-framework ApplicationServices" "-framework Cocoa")
endif()
