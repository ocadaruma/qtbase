# Generated from imagescaling.pro.

cmake_minimum_required(VERSION 3.14)
project(imagescaling LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Concurrent)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(imagescaling
    imagescaling.cpp imagescaling.h
    main.cpp
)
target_link_libraries(imagescaling PUBLIC
    Qt::Concurrent
    Qt::Widgets
)

install(TARGETS imagescaling
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
