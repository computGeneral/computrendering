@echo off
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: CG1 GPU Simulator — Windows build script (MSVC)
::
:: This script configures and builds the project into the _BUILD_ directory
:: using CMake and MSBuild (Visual Studio 2022).
::
:: Usage (from project root):
::   tools\script\build.bat [Debug|Release] [Win32|x64]
::
:: Defaults: Debug, Win32
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

setlocal enabledelayedexpansion

:: --- Parse arguments ---
set CONFIG=%~1
set ARCH=%~2

if "%CONFIG%"=="" set CONFIG=Debug
if "%ARCH%"=="" set ARCH=Win32

:: Validate CONFIG
if /i not "%CONFIG%"=="Debug" if /i not "%CONFIG%"=="Release" (
    echo [ERROR] Invalid configuration: %CONFIG%. Use Debug or Release.
    exit /b 1
)

:: Validate ARCH
if /i not "%ARCH%"=="Win32" if /i not "%ARCH%"=="x64" (
    echo [ERROR] Invalid architecture: %ARCH%. Use Win32 or x64.
    exit /b 1
)

:: --- Locate project root (two levels up from this script) ---
set SCRIPT_DIR=%~dp0
pushd "%SCRIPT_DIR%..\.."
set PROJECT_ROOT=%CD%
popd

echo ============================================================
echo  CG1 GPU Simulator — Windows Build
echo  Configuration : %CONFIG%
echo  Architecture  : %ARCH%
echo  Project Root  : %PROJECT_ROOT%
echo ============================================================

:: --- CMake configure ---
echo.
echo [1/2] Configuring with CMake ...
cmake -S "%PROJECT_ROOT%" -B "%PROJECT_ROOT%\_BUILD_" -A %ARCH%
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

:: --- CMake build ---
echo.
echo [2/2] Building CG1SIM (%CONFIG%) ...
cmake --build "%PROJECT_ROOT%\_BUILD_" --config %CONFIG% --target CG1SIM -- /m /v:m
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo.
echo ============================================================
echo  Build succeeded!
echo  Binary: _BUILD_\arch\%CONFIG%\CG1SIM.exe
echo ============================================================

endlocal
