cmake_minimum_required(VERSION 3.16)

# QT
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_PREFIX_PATH C:/Qt/6.6.0/msvc2019_64/lib/cmake)
include(QtCommon)

# Project Info
project(asset-manager VERSION 1.0)
enable_language(C)
enable_language(CXX)

# Makefile generators on apple need this flag to compile mixed objective/c++
if(APPLE AND NOT XCODE)
	set(CMAKE_CXX_FLAGS "-ObjC++")
endif()
if(MSVC AND (MSVC_VERSION GREATER_EQUAL 1914))
	add_compile_options("/Zc:__cplusplus")
endif()

# CMake Settings
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_SUPPRESS_REGENERATION true)
set(DCMAKE_GENERATOR_PLATFORM "x64")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
if(NOT CMAKE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX d)
endif()

# Set PROJECT_VERSION_PATCH and PROJECT_VERSION_TWEAK to 0 if not present, needed by add_project_meta
fix_project_version()

# Set additional project information
set(COMPANY "Marcus Madland")
set(COPYRIGHT "Copyright (c) 2023 Marcus Madland. All rights reserved.")
set(IDENTIFIER "com.marcusmadland.assetmanager")

# Sources
file(GLOB_RECURSE SOURCE_FILES RELATIVE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)
add_project_meta(META_FILES_TO_INCLUDE)
set(RESOURCE_FILES example.qrc)

# Dependencies ================================================
if(WIN32)
    set(FBX_ROOT "C:/Program Files/Autodesk/FBX/FBX SDK/2020.0.1")

    set(RUNTIME_ROOT "${FBX_ROOT}/lib/vs2017/x64/debug")

    set(DLL_DEST "${CMAKE_BINARY_DIR}/bin")

    set(INCLUDE_DIR
        ${INCLUDE_DIR}
        "${FBX_ROOT}/include"
    )

    set(LINK_LIBS
        "${RUNTIME_ROOT}/libfbxsdk.lib"
    )

    # Copy the DLL using configure_file
    #configure_file("${RUNTIME_ROOT}/libfbxsdk.dll" "${DLL_DEST}" COPYONLY)

endif(WIN32) # fbxsdk

find_package(Qt6 COMPONENTS Widgets REQUIRED) # qt

add_subdirectory(../../mengine/ ${CMAKE_BINARY_DIR}/mengine) # mengine
# =============================================================

# Add executable
add_executable(
	${PROJECT_NAME} 
	${OS_BUNDLE} 
   	"${SOURCE_FILES}"
	${META_FILES_TO_INCLUDE} 
	${RESOURCE_FILES}
)

# Link to engine and qt
target_precompile_headers(${PROJECT_NAME} INTERFACE QtWidgets.h)
target_link_libraries(
	${PROJECT_NAME} 
	Qt6::Widgets
	mengine
	${LINK_LIBS}
)
install(TARGETS ${PROJECT_NAME})
target_include_directories(
	${PROJECT_NAME} 
	PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/include/
	${INCLUDE_DIR}
)

# Change output dir to bin
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
# Change working directory to bin
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()
