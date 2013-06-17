set(kded_androidshine_tests_SRCS
    backendtests.cpp
)

kde4_add_unit_test(kded_androidshine_tests ${kded_androidshine_tests_SRCS})

target_link_libraries(kded_androidshine_tests
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    kdnssd
    ${QT_QTNETWORK_LIBRARY}
)

add_test(kded_androidshine_tests ${CMAKE_CURRENT_BINARY_DIR}/kded_androidshine_tests)


