add_subdirectory( applets )


########### next target ###############

set( amarokpud_LIB_SRCS
    popupdropper/libpud/PopupDropper.cpp
    popupdropper/libpud/PopupDropperItem.cpp
    popupdropper/libpud/PopupDropperView.cpp
)

add_library( amarokpud SHARED ${amarokpud_LIB_SRCS} )

if(WIN32)
    target_compile_definitions(amarokpud PRIVATE MAKE_POPUPDROPPER_LIB)
endif()

target_include_directories( amarokpud PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/popupdropper/libpud
)

target_link_libraries(amarokpud Qt::Core Qt::Gui Qt::Svg Qt::SvgWidgets Qt::Widgets)

install( TARGETS amarokpud ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )


set(qml_plugin_qml_files
    qml_plugin/qmldir
    qml_plugin/Applet.qml
    qml_plugin/AppletHeader.qml
)

set(qml_plugin_SRCS
    qml_plugin/src/RatingItem.cpp
    qml_plugin/src/Plugin.cpp
    qml_plugin/src/PixmapItem.cpp
)

add_library(qml_plugin SHARED ${qml_plugin_SRCS})

target_link_libraries(qml_plugin
    amarokcore
    Qt::Quick
    KF${KF_MAJOR_VERSION}::WidgetsAddons
)

install( TARGETS qml_plugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/qml )

install( FILES ${qml_plugin_qml_files} DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/amarok/qml )

kpackage_install_package( context_qml_package org.kde.amarok.context genericqml )
