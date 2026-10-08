cmake_minimum_required(VERSION 3.22)
project(panelmod CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(preloader
  GIT_REPOSITORY https://github.com/LiteLDev/preloader-android.git
  GIT_TAG 0.2.2)
FetchContent_MakeAvailable(preloader)
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 11.2.0)
FetchContent_MakeAvailable(fmt)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui)
add_library(panelmod SHARED
  src/main.cpp
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_android.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(panelmod PRIVATE src ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(panelmod PRIVATE preloader fmt android log EGL GLESv3)
target_compile_options(panelmod PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)
