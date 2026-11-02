add_library(kvoicememos_core STATIC
    voicememoscontroller.cpp
    voicememoscontroller.h
)

target_include_directories(kvoicememos_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(kvoicememos_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    KF6::CoreAddons
    KF6::I18n
)

add_executable(kvoicememos
    main.cpp
    resources.qrc
)

target_link_libraries(kvoicememos PRIVATE
    kvoicememos_core
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    KF6::CoreAddons
    KF6::I18n
    KF6::Kirigami
)

install(TARGETS kvoicememos DESTINATION ${KDE_INSTALL_BINDIR})
install(FILES org.cesar.kvoicememos.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES icons/org.cesar.kvoicememos.svg DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)
