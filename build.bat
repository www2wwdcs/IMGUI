@echo off
echo ================================
echo Building ImGui DX11 Template
echo ================================

if not exist "imgui" (
    echo ERROR: imgui/ not found. Run setup.bat first.
    pause
    exit /b 1
)

REM MSVC build (cl.exe must be in PATH, or run from VS Developer Command Prompt)
REM Compile as single translation unit — matches official example build method

set IMGUI_SRC=imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_win32.cpp imgui/backends/imgui_impl_dx11.cpp

set INCLUDES=/I imgui /I imgui/backends
set LIBS=d3d11.lib dxguid.lib winmm.lib d3dcompiler.lib

cl.exe /nologo /EHsc /O2 /Fe:ImGuiDX11Template.exe main.cpp %IMGUI_SRC% %INCLUDES% %LIBS% /link /SUBSYSTEM:WINDOWS

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build succeeded: ImGuiDX11Template.exe
) else (
    echo.
    echo Build failed. Make sure you run this from a Developer Command Prompt for VS.
)
pause
