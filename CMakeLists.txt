cmake_minimum_required(VERSION 3.16)

project(Studio3DViewer)
set(EXE_NAME Studio3DViewer)

# --- Настройки стандартов C++ и Qt ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# include: Общепринятый стандарт (от корня src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# --- Настройки для Windows ---
if(WIN32)
    set(QT_NO_PACKAGE_VERSION_CHECK TRUE)
    set(QT_NO_PACKAGE_VERSION_INCOMPATIBLE_WARNING TRUE)

    # --- Настройки для Windows из-под Linux ---
    if(CMAKE_CROSSCOMPILING)
        # Используем CACHE PATH, чтобы это можно было переопределить из консоли при желании
        set(STATIC_BASE "/usr/x86_64-w64-mingw32/static" CACHE PATH "Path to static base")

        # Сначала ищем в статической Qt, потом в обычном mingw
        set(CMAKE_FIND_ROOT_PATH "${STATIC_BASE};/usr/x86_64-w64-mingw32")
    # --- Настройки для Windows из-под Windows ---
    else()
        if(DEFINED ENV{MINGW_PREFIX})
            # Динамически считываем путь MSYS2 (работает и на GitHub Actions, и локально)
            set(STATIC_BASE "$ENV{MINGW_PREFIX}/qt6-static" CACHE PATH "Path to static base")

            # Сначала ищем в статической Qt, потом в обычном mingw
            set(CMAKE_FIND_ROOT_PATH "${STATIC_BASE};$ENV{MINGW_PREFIX}")
        elseif(EXISTS "C:/msys64")
            # Используем CACHE PATH, чтобы это можно было переопределить из консоли при желании
            set(STATIC_BASE "C:/msys64/mingw64/qt6-static" CACHE PATH "Path to static base")

            # Сначала ищем в статической Qt, потом в обычном mingw
            set(CMAKE_FIND_ROOT_PATH "${STATIC_BASE};C:/msys64/mingw64")
        endif()
    endif()

    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

    set(CMAKE_PREFIX_PATH "${STATIC_BASE}")
    set(Qt6_DIR "${STATIC_BASE}/lib/cmake/Qt6")

    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

    # Линковка системных библиотек компилятора
    set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")

    # Принудительно подключаем Winsock2, Crypt32 и OpenGL в самый конец линковки
    set(CMAKE_CXX_STANDARD_LIBRARIES "-lws2_32 -lcrypt32 -lopengl32 ${CMAKE_CXX_STANDARD_LIBRARIES}")

    add_definitions(-DQT_STATIC)
endif()

# --- Поиск зависимостей ---
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Concurrent)

if(WIN32)
    # --- Настройки для Windows из-под Linux ---
    if(CMAKE_CROSSCOMPILING)
        set(LIB_PATH "/usr/x86_64-w64-mingw32/lib")
    # --- Настройки для Windows из-под Windows ---
    else()
        if(DEFINED ENV{MINGW_PREFIX})
            set(LIB_PATH "$ENV{MINGW_PREFIX}/lib")
        elseif(EXISTS "C:/msys64")
            set(LIB_PATH "C:/msys64/mingw64/lib")
        endif()
    endif()

    # 1. Harfbuzz + Graphite2
    find_library(HARFBUZZ_STATIC NAMES harfbuzz PATHS "${LIB_PATH}" NO_DEFAULT_PATH REQUIRED)
    find_library(GRAPHITE2 NAMES graphite2 PATHS "${LIB_PATH}" NO_DEFAULT_PATH REQUIRED)

    if(TARGET harfbuzz::harfbuzz)
        set_target_properties(harfbuzz::harfbuzz PROPERTIES
            IMPORTED_LOCATION         "${HARFBUZZ_STATIC}"
            IMPORTED_IMPLIB           "${HARFBUZZ_STATIC}"
            IMPORTED_LOCATION_RELEASE "${HARFBUZZ_STATIC}"
            IMPORTED_IMPLIB_RELEASE   "${HARFBUZZ_STATIC}"
            INTERFACE_LINK_LIBRARIES  "${GRAPHITE2}"
        )
    endif()

    # 2. Freetype + Brotli + BZip2 + ZSTD
    find_library(BROTLIDEC NAMES brotlidec PATHS "${LIB_PATH}" NO_DEFAULT_PATH REQUIRED)
    find_library(BROTLI_COMMON NAMES brotlicommon PATHS "${LIB_PATH}" NO_DEFAULT_PATH REQUIRED)
    find_library(BZ2 NAMES bz2 PATHS "${LIB_PATH}" NO_DEFAULT_PATH REQUIRED)
    find_library(ZSTD_STATIC NAMES zstd PATHS "${LIB_PATH}" NO_DEFAULT_PATH REQUIRED)

    if(TARGET Freetype::Freetype)
        set_target_properties(Freetype::Freetype PROPERTIES INTERFACE_LINK_LIBRARIES "${HARFBUZZ_STATIC};${BROTLIDEC};${BROTLI_COMMON};${BZ2};${ZSTD_STATIC}")
    endif()

    # 3. WrapZSTD
    if(TARGET WrapZSTD)
        set_target_properties(WrapZSTD PROPERTIES INTERFACE_LINK_LIBRARIES "${ZSTD_STATIC}")
    endif()
endif()

# --- Исходные файлы ---
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" "src/*.c")

# --- Ресурсы ---
set(RESOURCES assets/resources.qrc)
qt_add_resources(SOURCES ${RESOURCES})

# --- Сборка ---
if(WIN32)
    # --- Настройки для Windows ---
    set(WIN_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/assets/resource.rc")

     # ВАЖНО: Включаем ресурсы напрямую в список файлов
    qt_add_executable(${EXE_NAME} WIN32 MANUAL_FINALIZATION ${SOURCES} ${WIN_RESOURCES})

    # Линковка (порядок важен для статических библиотек!)
    target_link_libraries(${EXE_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Concurrent)

    # --- Настройки для Windows из-под Windows ---
    if(NOT CMAKE_CROSSCOMPILING)
        # Добавляем системные зависимости libtiff и harfbuzz
        set(CMAKE_CXX_STANDARD_LIBRARIES "-llzma -llerc -ldeflate -ljbig -ljpeg -lrpcrt4 ${CMAKE_CXX_STANDARD_LIBRARIES}")
    endif()

    # Финализация (обязательна для Qt6 на Windows)
    qt_finalize_executable(${EXE_NAME})
else()
    # --- Настройки для Linux ---
    # Обычная сборка для Linux
    add_executable(${EXE_NAME} ${SOURCES})

    target_link_libraries(${EXE_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Concurrent)
endif()
