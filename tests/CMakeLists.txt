find_package(Qt6Test ${QT_MIN_VERSION} REQUIRED)

add_executable(test_kvoicememos
    test_kvoicememos.cpp
    ${CMAKE_SOURCE_DIR}/src/resources.qrc
)

target_compile_definitions(test_kvoicememos PRIVATE KVOICEMEMOS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

target_link_libraries(test_kvoicememos PRIVATE
    kvoicememos_core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Test
)

add_test(NAME test_kvoicememos COMMAND test_kvoicememos)
