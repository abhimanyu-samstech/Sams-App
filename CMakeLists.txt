cmake_minimum_required(VERSION 3.16)
project(CamTest1 VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 REQUIRED COMPONENTS Quick Multimedia Network WebView)
qt_standard_project_setup(REQUIRES 6.10)
qt_add_executable(appCamTest1
    main.cpp
    videoplayer.h
    videoplayer.cpp
    cameradiscovery.h
    cameradiscovery.cpp
    cameramanager.h
    cameramanager.cpp
    recordingmanager.h
    recordingmanager.cpp
    streammanager.h
    streammanager.cpp
)
qt_add_qml_module(appCamTest1
    URI CamTest1
    QML_FILES
        Main.qml
)
set_target_properties(appCamTest1 PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
)
target_link_libraries(appCamTest1 PRIVATE
    Qt6::Quick
    Qt6::Multimedia
    Qt6::Network
    Qt6::WebView
)
install(TARGETS appCamTest1
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)