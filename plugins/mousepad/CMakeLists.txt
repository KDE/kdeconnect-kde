set(kdeconnect_mousepad_SRCS
    mousepadplugin.cpp
)

find_package(XTest REQUIRED)
find_package(X11 REQUIRED)
find_package(LibFakeKey REQUIRED)

kdeconnect_add_plugin(kdeconnect_mousepad JSON kdeconnect_mousepad.json SOURCES  ${kdeconnect_mousepad_SRCS})

include_directories(${XTEST_INCLUDE_DIRS} ${X11_INCLUDE_DIR} ${LibFakeKey_INCLUDE_DIRS})

target_link_libraries(kdeconnect_mousepad kdeconnectcore Qt5::Gui ${X11_LIBRARIES} ${XTEST_LIBRARIES} ${LibFakeKey_LIBRARIES})
