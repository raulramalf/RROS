# RROS Ajustes: la aplicación nativa (Qt 6 Quick) de ajustes, tienda, bienvenida e instalador de RROS.
cmake_minimum_required(VERSION 3.16)
project(rros-ajustes LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 DBus)

file(GLOB UI_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} qml/*.qml qml/qmldir icons/*.svg icons/*.png)
add_executable(rros-ajustes main.cpp backend.cpp backend.h)
qt_add_resources(rros-ajustes ui PREFIX "/" FILES ${UI_FILES})
target_compile_options(rros-ajustes PRIVATE -Wall -Wextra)
target_link_libraries(rros-ajustes PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::DBus)
install(TARGETS rros-ajustes RUNTIME DESTINATION bin)
