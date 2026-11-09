cmake_minimum_required(VERSION 3.20)
project(MyGame LANGUAGES CXX)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB SRC_FILES CONFIGURE_DEPENDS
    "src/*.c"
    "src/*.cpp"
)

file(GLOB LIB_FILES CONFIGURE_DEPENDS
    "extlibs/*.c"
    "extlibs/*.cpp"
)

add_executable(main ${SRC_FILES} ${LIB_FILES})

target_include_directories(main PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/extlibs
)

find_program(CLANG_FORMAT_EXE clang-format)
if(CLANG_FORMAT_EXE)
    file(GLOB FMT CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/*.c"
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/*.h"
        "${CMAKE_SOURCE_DIR}/src/*.hpp"
        "${CMAKE_SOURCE_DIR}/src/*.inl"
        "${CMAKE_SOURCE_DIR}/include/*.c"
        "${CMAKE_SOURCE_DIR}/include/*.cpp"
        "${CMAKE_SOURCE_DIR}/include/*.h"
        "${CMAKE_SOURCE_DIR}/include/*.hpp"
        "${CMAKE_SOURCE_DIR}/include/*.inl")

    file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/RunClangFormat.cmake CONTENT "
set(files \"${FMT}\")
foreach(f \${files})
    message(STATUS \"Formatting: \${f}\")
    execute_process(COMMAND \"${CLANG_FORMAT_EXE}\" -i \"\${f}\")
endforeach()
")
    add_custom_target(format
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/RunClangFormat.cmake
        VERBATIM)
endif()

find_program(CLANG_TIDY_EXE clang-tidy)
if(CLANG_TIDY_EXE)
    file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/RunClangTidy.cmake CONTENT "
set(files \"${SRC_FILES}\")
foreach(f \${files})
    message(STATUS \"Checking: \${f}\")
    execute_process(
        COMMAND \"${CLANG_TIDY_EXE}\"
                -p=\"${CMAKE_BINARY_DIR}\"
                --checks=*
                \"--header-filter=^${CMAKE_SOURCE_DIR}/(src|include)/.*\"
                \"\${f}\"
    )
endforeach()
")
    add_custom_target(check
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/RunClangTidy.cmake
        VERBATIM)
endif()

set(DEBUG_SANITIZERS "address,undefined")

if(UNIX AND NOT APPLE)
    set(DEBUG_SANITIZERS "address,undefined,leak")
endif()

target_compile_options(main PRIVATE

    -Wno-deprecated-declarations
    -fsigned-char

    $<$<CONFIG:Debug>:
        -O0
        -Wall
        -fsanitize=${DEBUG_SANITIZERS}
        -fno-omit-frame-pointer
        -fno-inline
        -fno-eliminate-unused-debug-types
        -g3
        -gdwarf-5
        -glldb
        -pthread
    >

    $<$<CONFIG:RelWithDebInfo>:
        -O2
        -g3
        -fno-omit-frame-pointer
        -pthread
    >

    $<$<CONFIG:Release>:
        -O3
        -DNDEBUG
        -fomit-frame-pointer
        -ffunction-sections
        -fdata-sections
        -pthread
    >
)

target_link_options(main PRIVATE

    $<$<CONFIG:Debug>:
        -fsanitize=${DEBUG_SANITIZERS}
        -pthread
    >

    $<$<CONFIG:RelWithDebInfo>:
        -pthread
    >

    $<$<CONFIG:Release>:
        -Wl,--gc-sections
        -Wl,--strip-all
        -mwindows
    >
)

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Audio)

if(UNIX AND NOT APPLE)
    set(OpenGL_GL_PREFERENCE GLVND)
endif()

find_package(OpenGL REQUIRED)

target_link_libraries(main PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
)

if(UNIX AND NOT APPLE)
    target_link_libraries(main PRIVATE
        OpenGL::OpenGL
        OpenGL::GLX
    )
else()
    target_link_libraries(main PRIVATE
        OpenGL::GL
    )
endif()

if(WIN32)
    target_link_libraries(main PRIVATE ws2_32 winmm)
endif()

set_target_properties(main PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

