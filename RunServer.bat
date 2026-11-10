@echo off
setlocal

rem ===== Settings =====
set ENGINE_DIR=G:\DediCateProject\UnrealEngine5.8
set PROJECT=%~dp0DedicateTPS.uproject
set MAP=/Game/SoulCity/Maps/LV_Soul_Slum
set PORT=7777

rem DebugGame = Rider "DebugGame Editor" build, Development = "Development Editor" build
set CONFIG=DebugGame
if /I "%~1"=="dev" set CONFIG=Development

if /I "%CONFIG%"=="DebugGame" (
    set EDITOR_EXE=%ENGINE_DIR%\Engine\Binaries\Win64\UnrealEditor-Win64-DebugGame.exe
) else (
    set EDITOR_EXE=%ENGINE_DIR%\Engine\Binaries\Win64\UnrealEditor.exe
)

if not exist "%EDITOR_EXE%" (
    echo [RunServer] Not found: %EDITOR_EXE%
    echo [RunServer] Build "%CONFIG% Editor" in Rider first.
    pause
    exit /b 1
)

echo [RunServer] Config : %CONFIG%
echo [RunServer] Map    : %MAP%
echo [RunServer] Port   : %PORT%
echo [RunServer] Client : open 127.0.0.1:%PORT%

"%EDITOR_EXE%" "%PROJECT%" %MAP% -server -log -port=%PORT% -unattended -NoSound

endlocal