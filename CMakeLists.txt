cmake_minimum_required(VERSION 3.16)
project(ImGuiDX11Template)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)

# ImGui core
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PUBLIC d3d11.lib dxguid.lib winmm.lib)

# Main executable
add_executable(ImGuiDX11Template WIN32
    main.cpp
)

target_link_libraries(ImGuiDX11Template PRIVATE imgui)
