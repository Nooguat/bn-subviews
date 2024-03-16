cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# Plugin settings
set(PLUGIN_NAME bwindows)
set(PLUGIN_DIR "/home/gsd/.rev/.tools/binja_plugins")
set(PLUGIN_CDEFS BWINDOW=1)
set(PLUGIN_BUILD "${PLUGIN_DIR}/bwindows/build/bin")

set(PLUGIN_SOURCE Plugin.cpp Plugin.h)
set(UI_PLUGIN_SOURCE UI/priv.h
          UI/subview.h UI/subview.cpp)


if (UI_BUILD)
    # You may need to replace this with your path
    if(APPLE)
        set(CMAKE_PREFIX_PATH "$ENV{HOME}/Qt/6.4.3/clang_64/lib/cmake")
    elseif(MSVC) # doesnt work on gh runners
        set(CMAKE_PREFIX_PATH "$ENV{HOME}/Qt/6.4.3/msvc2022_64/lib/cmake")
    else()
        set(CMAKE_PREFIX_PATH "${PLUGIN_DIR}/Qt/6.4.3/gcc_64/lib/cmake")
    endif()
endif()

# BinaryNinja API Directory
set(BINJA_API_DIR "${PLUGIN_DIR}/binaryninja-api" ${PLUGIN_BUILD})

# Non-user-serviceable parts below ---------------------------------------------

project(${PLUGIN_NAME})

add_subdirectory(${BINJA_API_DIR})

include_directories(${CMAKE_SOURCE_DIR})

set(_UI_PLUGIN_SOURCES )
set(UI_PLUGIN_LIBRARIES )
set(UI_COMPILE_DEFS )

if (UI_BUILD)

    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    set(_UI_PLUGIN_SOURCES ${UI_PLUGIN_SOURCE})
    set(UI_PLUGIN_LIBRARIES Qt6::Core Qt6::Gui Qt6::Widgets)
    set(UI_COMPILE_DEFS UI_BUILD=1)

    find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
    find_package(QT NAMES Qt6 COMPONENTS Widgets REQUIRED)

endif()

set(_PLUGIN_SOURCE ${PLUGIN_SOURCE} ${_UI_PLUGIN_SOURCES})
set("${CMAKE_C_FLAGS} -include${PROJECT_SOURCE_DIR}/Prefix.h")


add_library(${PLUGIN_NAME} SHARED ${_PLUGIN_SOURCE})

if(APPLE)
    target_link_options(${PLUGIN_NAME} PUBLIC -undefined dynamic_lookup)
elseif(MSVC)
    target_link_options(${PLUGIN_NAME} PUBLIC "LINKER:/FORCE:UNRESOLVED")
else()
    target_link_options(${PLUGIN_NAME} PUBLIC "LINKER:--allow-shlib-undefined")
endif()
target_link_libraries(${PLUGIN_NAME} binaryninjaapi  ${UI_PLUGIN_LIBRARIES})
target_compile_features(${PLUGIN_NAME} PRIVATE cxx_std_17 c_std_99)
target_compile_definitions(${PLUGIN_NAME} PRIVATE
        DEV_MODE=${DEV_MODE} ${UI_COMPILE_DEFS} ${PLUGIN_CDEFS})
target_include_directories(${PLUGIN_NAME} PUBLIC )
# Library targets linking against the Binary Ninja API need to be compiled with
# position-independent code on Linux.
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    target_compile_options(${PLUGIN_NAME} PRIVATE "-fPIC")
endif()

list(APPEND fcl ${_PLUGIN_SOURCE})
list(LENGTH fcl file_count)
message(STATUS "")

if (UI_BUILD)
    message(STATUS "BinaryNinja UI Plugin Config -=-=-=-======")
else()
    message(STATUS "BinaryNinja Headless Plugin Config -=-=-=-======")
endif()

message(STATUS "Project Name: ${PLUGIN_NAME}")
message(STATUS "Tracked Source Files: ${file_count}")
message(STATUS "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-======")

message(STATUS "")

# Support cmake --install
bn_install_plugin(${PLUGIN_NAME})
