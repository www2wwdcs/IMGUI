@echo off
echo ================================
echo ImGui DX11 Template Setup
echo ================================

REM Clone official Dear ImGui from GitHub
if not exist "imgui" (
    echo Cloning Dear ImGui...
    git clone --depth 1 https://github.com/ocornut/imgui.git imgui
) else (
    echo imgui/ already exists, skipping clone.
)

echo.
echo Setup complete!
echo Run build.bat to compile, or use CMake:
echo   mkdir build ^&^& cd build ^&^& cmake .. ^&^& cmake --build .
pause
