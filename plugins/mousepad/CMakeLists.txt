kdeconnect_add_plugin(kdeconnect_mousepad SOURCES mousepadplugin.cpp abstractremoteinput.cpp)

target_include_directories(kdeconnect_mousepad PRIVATE ${CMAKE_BINARY_DIR})

if(UNIX AND NOT APPLE)
    target_sources(kdeconnect_mousepad PUBLIC waylandremoteinput.cpp ${SRCS})
    target_sources(kdeconnect_mousepad PRIVATE ${wayland_SRCS})
    target_link_libraries(kdeconnect_mousepad Wayland::Client Qt::WaylandClient PkgConfig::XkbCommon PkgConfig::PKG_libei)
endif()

if(WITH_X11)
    target_sources(kdeconnect_mousepad PUBLIC x11remoteinput.cpp)
    target_include_directories(kdeconnect_mousepad PRIVATE ${XTEST_INCLUDE_DIRS} ${X11_INCLUDE_DIR} ${LibFakeKey_INCLUDE_DIRS})
    target_link_libraries(kdeconnect_mousepad Qt6::GuiPrivate ${X11_LIBRARIES} ${XTEST_LIBRARIES} ${LibFakeKey_LIBRARIES})
endif()

target_link_libraries(kdeconnect_mousepad kdeconnectcore kdeconnectdbusinterfaces Qt::Gui KF6::I18n)

if (WIN32)
    target_sources(kdeconnect_mousepad PUBLIC windowsremoteinput.cpp)
endif()

if (APPLE)
    target_sources(kdeconnect_mousepad PUBLIC macosremoteinput.mm)
    target_link_libraries(kdeconnect_mousepad "-framework CoreGraphics" "-framework ApplicationServices" "-framework Cocoa")
endif()
