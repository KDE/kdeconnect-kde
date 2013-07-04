set(kded_kdeconnect_tests_SRCS
    ../daemon/networkpackage.cpp
    backendtests.cpp
)

kde4_add_unit_test(kded_kdeconnect_tests ${kded_kdeconnect_tests_SRCS})

target_link_libraries(kded_kdeconnect_tests
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    kdnssd
    qjson
    ${QT_QTTEST_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
)

add_test(kded_kdeconnect_tests ${CMAKE_CURRENT_BINARY_DIR}/kded_kdeconnect_tests)

