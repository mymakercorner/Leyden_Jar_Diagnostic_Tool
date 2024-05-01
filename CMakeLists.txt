cmake_minimum_required(VERSION 3.21.0)

project(
  Leyden_Jar_Diagnostic_Tool
  VERSION 1.0
  LANGUAGES C CXX)

# We used straight C++11, no need of extras features found in later C++ standards
set(CMAKE_CXX_STANDARD 11)

# We remove all JSONCPP tests to unclutter the whole project a bit
set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Compile and (for jsoncpp_check) run JsonCpp test executables" FORCE)
set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "Automatically run unit-tests as a post build step" FORCE)

# This removes the console for Windows platform
if(WIN32)
  set(CMAKE_WIN32_EXECUTABLE ON)
endif()

add_subdirectory(external/hidapi)
add_subdirectory(external/jsoncpp)
add_subdirectory(external/minlzma/minlzlib)
add_subdirectory(external/SDL)

# Find OpenGL package, used by SDLMain and ImGui for Windows/Linux/Mac
find_package(OpenGL)

# Leyden Jar Diagnostic Tool executable
add_executable(Leyden_Jar_Diagnostic_Tool
  src/Main.cpp
  src/LeydenJarProtocol.cpp
  src/LeydenJarProtocol.h
  src/LeydenJarAgent.cpp
  src/LeydenJarAgent.h
  src/LeydenJarImGuiHelpers.cpp
  src/LeydenJarImGuiHelpers.h
  src/LeydenJarDiagnosticTool.cpp
  src/LeydenJarDiagnosticTool.h
  external/imgui/imgui.cpp
  external/imgui/imgui_demo.cpp
  external/imgui/imgui_draw.cpp
  external/imgui/imgui_tables.cpp
  external/imgui/imgui_widgets.cpp
  external/imgui/imgui_widgets.cpp
  external/imgui/backends/imgui_impl_opengl3.cpp
  external/imgui/backends/imgui_impl_sdl2.cpp
)

# Adding ImGui include path
target_include_directories(Leyden_Jar_Diagnostic_Tool PRIVATE external/imgui)

# Adding minlzma include path
target_include_directories(Leyden_Jar_Diagnostic_Tool PRIVATE external/minlzma)

# Adding JSONCPP include path as it's CmakeLists file does not look to do that for us
target_include_directories(Leyden_Jar_Diagnostic_Tool PRIVATE  external/jsoncpp/include)

# If SDL2 is used, SDL2Main library is required for Windows platforms
if(TARGET SDL2::SDL2main)
    target_link_libraries(Leyden_Jar_Diagnostic_Tool PRIVATE SDL2::SDL2main)
endif()

# Link with SDL2 library
target_link_libraries(Leyden_Jar_Diagnostic_Tool PRIVATE SDL2::SDL2)

# Link with hidapi, jsoncpp_lib and minlzlib
target_link_libraries(Leyden_Jar_Diagnostic_Tool PRIVATE hidapi jsoncpp_lib minlzlib)

# Link with OpenGL libraies because used by SDLMain and ImGui for Windows/Linux/Mac
target_link_libraries(Leyden_Jar_Diagnostic_Tool PRIVATE ${OPENGL_LIBRARIES})