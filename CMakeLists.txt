cmake_minimum_required(VERSION 3.20)
project(USBunker VERSION 1.0.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)
find_package(OpenSSL REQUIRED)

set(SOURCES
    src/main.cpp
    src/core/crypto.cpp
    src/core/vault.cpp
    src/core/usb.cpp
    src/ui/main_window.cpp
    src/ui/widgets.cpp
    src/ui/file_explorer.cpp
)

set(HEADERS
    src/core/types.h
    src/core/crypto.h
    src/core/vault.h
    src/core/usb.h
    src/ui/main_window.h
    src/ui/widgets.h
    src/ui/file_explorer.h
    src/ui/style.h
)

set(RESOURCES
    resources/resources.qrc
)

add_executable(usbunker ${SOURCES} ${HEADERS} ${RESOURCES})

target_include_directories(usbunker PRIVATE src)

target_link_libraries(usbunker PRIVATE
    Qt6::Widgets
    Qt6::Svg
    OpenSSL::Crypto
)

if(WIN32)
    set_target_properties(usbunker PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_libraries(usbunker PRIVATE setupapi)
endif()

install(TARGETS usbunker RUNTIME DESTINATION bin)
