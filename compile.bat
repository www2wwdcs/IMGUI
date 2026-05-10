@echo off
call "D:\application\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d "C:\Users\Administrator\Documents\code\ui"

set IMGUI_DIR=imgui
set IMGUI_SRC=%IMGUI_DIR%/imgui.cpp %IMGUI_DIR%/imgui_demo.cpp %IMGUI_DIR%/imgui_draw.cpp %IMGUI_DIR%/imgui_tables.cpp %IMGUI_DIR%/imgui_widgets.cpp %IMGUI_DIR%/backends/imgui_impl_win32.cpp %IMGUI_DIR%/backends/imgui_impl_dx11.cpp
set INCLUDES=/I %IMGUI_DIR% /I %IMGUI_DIR%/backends

cl.exe /nologo /EHsc /O2 /utf-8 /Fe:ImGuiDX11Template.exe main.cpp %IMGUI_SRC% %INCLUDES% d3d11.lib dxguid.lib winmm.lib /link /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup

if %ERRORLEVEL% EQU 0 (
    echo BUILD SUCCESS: ImGuiDX11Template.exe
) else (
    echo BUILD FAILED
)
