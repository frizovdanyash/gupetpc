cmake_minimum_required(VERSION 3.16)
project(GUPet LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
    add_definitions(-DUNICODE -D_UNICODE)

    set(SOURCES
        src/main.cpp
        src/pet_renderer.cpp
        src/skin_store.cpp
        src/audio_monitor.cpp
        src/settings_manager.cpp
        src/speech_bubble.cpp
        src/poop_manager.cpp
        src/pet_overlay.cpp
        src/settings_dialog.cpp
        src/tray_icon.cpp
        src/web_server.cpp
        src/miniz.c
        res/resource.rc
    )

    add_executable(GUPet WIN32 ${SOURCES})

    target_link_libraries(GUPet PRIVATE
        gdiplus
        gdi32
        comctl32
        shlwapi
        ole32
        uxtheme
        msimg32
        winmm
        ws2_32
    )

    if (MINGW)
        target_link_options(GUPet PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0 cairo pangocairo)

    set(SOURCES
        src/main_linux.cpp
        src/pet_renderer_cairo.cpp
        src/pet_overlay_linux.cpp
        src/settings_manager.cpp
        src/skin_store.cpp
        src/web_server.cpp
        src/miniz.c
    )

    add_executable(gupet ${SOURCES})

    target_include_directories(gupet PRIVATE ${GTK3_INCLUDE_DIRS})
    target_link_libraries(gupet PRIVATE ${GTK3_LIBRARIES} pthread)
    target_compile_options(gupet PRIVATE ${GTK3_CFLAGS_OTHER})
endif()
