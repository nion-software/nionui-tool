# https://doc.qt.io/qt-5/cmake-get-started.html#build-a-gui-executable
# https://gitlab.kitware.com/cmake/cmake/issues/19820
# https://cmake.org/cmake/help/latest/prop_tgt/MACOSX_BUNDLE_INFO_PLIST.html
# https://cmake.org/cmake/help/latest/prop_tgt/RESOURCE.html#prop_tgt:RESOURCE
# https://stackoverflow.com/questions/41121000/cmake-os-x-bundle-recursively-copy-directory-to-resources
# https://cmake.org/cmake/help/latest/module/FindPython3.html
# https://pastebin.com/FHnd1SRR

# Windows: cmake -B. -S../launcher
# Windows: msbuild NionUILauncher.vcxproj -p:Configuration=Release

cmake_minimum_required(VERSION 3.20)

option(USE_CONSOLE "Enable console" ON)

# app name
set(APP_NAME "NionUILauncher")

# app version
set(APP_VERSION "5.1.4")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

# must go before project
# set(CMAKE_OSX_ARCHITECTURES arm64;x86_64)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for Mac OS X" FORCE)

# set the project name
project(${APP_NAME} VERSION ${APP_VERSION})

# set up Qt
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
qt_standard_project_setup()
add_compile_definitions(QT_NO_KEYWORDS)
add_compile_definitions(GIT_REPO="${GIT_REPO}")
add_compile_definitions(GIT_REF="${GIT_REF}")
add_compile_definitions(GIT_SHA="${GIT_SHA}")

# set up Python
find_package (Python3 COMPONENTS Development)
add_compile_definitions(MS_NO_COREDLL)  # do not allow brain dead library inclusion

# add the executable
add_executable(${APP_NAME}
    Application.cpp
    DocumentWindow.cpp
    main.cpp
    PythonSelectDialog.cpp
    PythonStubs.cpp
    PythonSupport.cpp
    resources.qrc
    stylesheet.qss
    bootstrap.py)

# configure include files
target_include_directories(${APP_NAME} PRIVATE ${Python3_INCLUDE_DIRS})

# Qt libraries
target_link_libraries(${APP_NAME} Qt6::Core Qt6::Gui Qt6::Widgets)

# Python libraries (intentional not linked since they are loaded dynamically)
# target_link_libraries(${APP_NAME} ${PYTHON_LIBRARIES})

# Resources
set(RESOURCE_FILES
    stylesheet.qss
    bootstrap.py)

# Linux Specific
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(${APP_NAME} ${CMAKE_DL_LIBS})

    # in conda environments, it is necessary to have lib in the dynamic
    # library search path. add it here. this has the side effect that if
    # the user installs Qt in the environment, it will be used over the
    # system Qt. this would be the recommended installation option if/when
    # conda releases a version of Qt newer than 5.9.
    set_target_properties(${APP_NAME} PROPERTIES
        # this cmake file only a build phase, so do not skip
        SKIP_BUILD_RPATH FALSE
        # and ensure the rpath is set during build
        BUILD_WITH_INSTALL_RPATH TRUE
        # but it uses the one defined here
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../lib"
        # do not add the automatically determined parts
        INSTALL_RPATH_USE_LINK_PATH FALSE)

    add_custom_command(
      TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/bootstrap.py
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)

    add_custom_command(
      TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/stylesheet.qss
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)

    add_custom_command(
      TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/Graphics)

    # add the graphics
    file(GLOB_RECURSE graphics "Graphics/*.png")
    foreach(graphic ${graphics})
        file(RELATIVE_PATH graphic_path "${PROJECT_SOURCE_DIR}" ${graphic})
        target_sources(${APP_NAME} PRIVATE ${graphic_path})
        add_custom_command(
          TARGET ${PROJECT_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/${graphic_path}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/Graphics)
    endforeach(graphic)

endif()

# Windows Specific
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    if (${USE_CONSOLE})
        set_target_properties(${APP_NAME} PROPERTIES LINK_FLAGS "/LTCG")
    else()
        set_target_properties(${APP_NAME} PROPERTIES LINK_FLAGS "/LTCG /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
    endif()

    target_sources(${APP_NAME} PRIVATE Application.rc)
    target_sources(${APP_NAME} PRIVATE Graphics/WinIcon.ico)

    add_custom_command(
      TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/bootstrap.py
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)

    add_custom_command(
      TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/stylesheet.qss
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)

    add_custom_command(
      TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/Graphics)

    add_custom_command(
      TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/Graphics/WinIcon.ico
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/Graphics)

    # add the graphics
    file(GLOB_RECURSE graphics "Graphics/*.png")
    foreach(graphic ${graphics})
        file(RELATIVE_PATH graphic_path "${PROJECT_SOURCE_DIR}" ${graphic})
        target_sources(${APP_NAME} PRIVATE ${graphic_path})
        add_custom_command(
          TARGET ${PROJECT_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/${graphic_path}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/Graphics)
    endforeach(graphic)

    # Get reference to deployqt
    get_target_property(uic_location Qt6::uic IMPORTED_LOCATION)
    get_filename_component( _dir ${uic_location} DIRECTORY)
    set(deployqt "${_dir}/windeployqt.exe")
    if(NOT EXISTS ${deployqt})
      set(deployqt "${_dir}/bin/windeployqt.exe")
      if(NOT EXISTS ${deployqt})
        message(FATAL_ERROR "Failed to locate deployqt executable: [${deployqt}]")
      endif()
    endif()

    add_custom_command(TARGET ${APP_NAME}
        POST_BUILD COMMAND
        ${deployqt} --no-translations --no-compiler-runtime --no-opengl-sw --release "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${APP_NAME}.exe")
endif()

# Mac Specific
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

    set(APP_EXECUTABLE "Nion UI Launcher")
    set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
    set(MACOSX_BUNDLE_BUNDLE_NAME ${APP_EXECUTABLE})
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.nion.ui.launcher")
    set(MACOSX_BUNDLE_ICON_FILE "MacIcon.icns")
    #set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${APP_VERSION})
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2012-2023 Nion Company. All rights reserved.")
    # mark the icons file as part of the sources
    target_sources(${APP_NAME} PRIVATE Graphics/MacIcon.icns)
    # set the location of the icon file
    set_source_files_properties(Graphics/MacIcon.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    # install the other resource files too
    set_target_properties(${APP_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        OUTPUT_NAME "${APP_EXECUTABLE}"
        RESOURCE "${RESOURCE_FILES}")

    # add the graphics
    file(GLOB_RECURSE graphics "Graphics/*.png")
    foreach(graphic ${graphics})
        file(RELATIVE_PATH graphic_path "${PROJECT_SOURCE_DIR}" ${graphic})
        target_sources(${APP_NAME} PRIVATE ${graphic_path})
        set_property(SOURCE ${graphic_path} PROPERTY MACOSX_PACKAGE_LOCATION "Resources/${graphic_path}")
    endforeach(graphic)

    # Get reference to deployqt
    get_target_property(uic_location Qt6::uic IMPORTED_LOCATION)
    get_filename_component( _dir ${uic_location} DIRECTORY)
    set(deployqt "${_dir}/../bin/macdeployqt")
    if(NOT EXISTS ${deployqt})
      set(deployqt "${_dir}/bin/macdeployqt")
      if(NOT EXISTS ${deployqt})
        message(FATAL_ERROR "Failed to locate deployqt executable: [${deployqt}]")
      endif()
    endif()

    add_custom_command(TARGET ${APP_NAME}
        POST_BUILD COMMAND
        ${deployqt} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${APP_EXECUTABLE}.app")

    add_custom_command(TARGET ${APP_NAME}
        POST_BUILD COMMAND
        ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@loader_path/../../Contents/Frameworks" "$<TARGET_FILE:${APP_NAME}>")

endif()

# Debugging

# cmake -U*PYTHON* --trace-expand .

# export CMAKE_PREFIX_PATH=/Users/cmeyer/Developer/Qt/5.14.1/clang_64/lib/cmake/Qt5
# export Python3_ROOT=/Users/cmeyer/Developer/miniconda3/envs/py38-forge
# set(CMAKE_PREFIX_PATH "/Users/cmeyer/Developer/Qt/5.14.1/clang_64/lib/cmake/Qt5")
# set(Python3_ROOT "/Users/cmeyer/Developer/miniconda3/envs/py38-forge")

#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()
