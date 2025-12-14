kdeconnect_add_plugin(kdeconnect_mousepad SOURCES mousepadplugin.cpp abstractremoteinput.cpp)

if(UNIX AND NOT APPLE)
    pkg_check_modules(PKG_libei REQUIRED IMPORTED_TARGET libei-1.0)
    target_sources(kdeconnect_mousepad PUBLIC waylandremoteinput.cpp ${SRCS})
    target_sources(kdeconnect_mousepad PRIVATE ${wayland_SRCS})
    target_link_libraries(kdeconnect_mousepad Wayland::Client Qt::WaylandClient PkgConfig::XkbCommon PkgConfig::PKG_libei)


    if (WITH_X11)
        find_package(LibFakeKey REQUIRED)
        set_package_properties(LibFakeKey PROPERTIES DESCRIPTION "fake key events"
                            URL "https://github.com/dtapuska/fakekey"
                            TYPE REQUIRED
                            PURPOSE "Needed for the remote mouse input plugin"
                            )

        find_package(XTest REQUIRED)
        find_package(X11 REQUIRED)
        target_include_directories(kdeconnect_mousepad PRIVATE ${XTEST_INCLUDE_DIRS} ${X11_INCLUDE_DIR} ${LibFakeKey_INCLUDE_DIRS})
    endif()
endif()

configure_file(config-mousepad.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-mousepad.h)

target_link_libraries(kdeconnect_mousepad kdeconnectcore kdeconnectdbusinterfaces Qt::Gui KF6::I18n)

if (WIN32)
    target_sources(kdeconnect_mousepad PUBLIC windowsremoteinput.cpp)
endif()

if(WITH_X11)
    target_sources(kdeconnect_mousepad PUBLIC x11remoteinput.cpp)
    target_link_libraries(kdeconnect_mousepad Qt6::GuiPrivate ${X11_LIBRARIES} ${XTEST_LIBRARIES} ${LibFakeKey_LIBRARIES})
endif()

if (APPLE)
    target_sources(kdeconnect_mousepad PUBLIC macosremoteinput.mm)
    target_link_libraries(kdeconnect_mousepad "-framework CoreGraphics" "-framework ApplicationServices" "-framework Cocoa")
endif()
