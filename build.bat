@echo off
setlocal EnableExtensions

set "CONFIG=Release"

if /I "%~1"=="Debug" set "CONFIG=Debug"
if /I "%~1"=="Res" goto :build_resources

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
    echo ERROR: vswhere.exe not found:
    echo "%VSWHERE%"
    exit /b 1
)

for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"

if not defined VSINSTALL (
    echo ERROR: Visual Studio 2022 Build Tools installation not found.
    exit /b 1
)

call "%VSINSTALL%\Common7\Tools\VsDevCmd.bat" -arch=x64
if errorlevel 1 exit /b 1

rem Checking required resources
if not exist "%~dp0build\resources\klti.ico" (
    echo ERROR: Required icon resource not found:
    echo   build/resources/klti.ico
    echo.
    echo Run "build.bat Res" first.
    exit /b 1
)

echo.
echo Building KLTI: %CONFIG% x64
echo.

MSBuild.exe "%~dp0klti.sln" ^
    /m ^
    /p:Configuration=%CONFIG% ^
    /p:Platform=x64

if errorlevel 1 (
    echo.
    echo BUILD FAILED.
    exit /b 1
)

rem Copy build artifact to dist dir
if /I "%CONFIG%"=="Release" (
    if not exist "%~dp0dist\Release" mkdir "%~dp0dist\Release"

    copy /Y "%~dp0build\Release\klti.exe" "%~dp0dist\Release\klti.exe" >nul
    if errorlevel 1 (
        echo.
        echo ERROR: Failed to copy klti.exe to dist/Release.
        exit /b 1
    )
)

echo.
echo BUILD SUCCEEDED.

endlocal
exit /b 0

:build_resources
set "ROOT=%~dp0"
set "SOURCE=%ROOT%assets"
set "RESOURCE_OUTPUT=%ROOT%build\resources"
set "DIST_OUTPUT=%ROOT%dist\Release\icons"

if not exist "%RESOURCE_OUTPUT%" mkdir "%RESOURCE_OUTPUT%"
if not exist "%DIST_OUTPUT%" mkdir "%DIST_OUTPUT%"

where magick >nul 2>&1
if errorlevel 1 (
    echo ERROR: ImageMagick ^(magick.exe^) was not found in PATH.
    exit /b 1
)

echo.
echo Building KLTI icons...
echo.

for %%F in ("%SOURCE%\layout-icons\*.svg") do (
    call :build_icon "%%F" "%DIST_OUTPUT%\%%~nF.ico"
    if errorlevel 1 exit /b 1
)

call :build_icon "%SOURCE%\unknown.svg" "%DIST_OUTPUT%\unknown.ico"
if errorlevel 1 exit /b 1

call :build_app_icon "%SOURCE%\klti.svg" "%RESOURCE_OUTPUT%\klti.ico"
if errorlevel 1 exit /b 1

echo.
echo ICON BUILD SUCCEEDED.

endlocal
exit /b 0

:build_icon
call :build_icon_with_sizes "%~1" "%~2" "16,20,24,32,40,48,64,256"
exit /b %errorlevel%

:build_app_icon
call :build_icon_with_sizes "%~1" "%~2" "16,32"
exit /b %errorlevel%

:build_icon_with_sizes
if not exist "%~1" (
    echo ERROR: Source file not found:
    echo   %~1
    exit /b 1
)

echo Building %~nx2

magick "%~1" ^
    -background none ^
    -resize "256x256" ^
    -gravity center ^
    -extent 256x256 ^
    -define icon:auto-resize=%~3 ^
    "%~2"

if errorlevel 1 (
    echo ERROR: Failed to create %~2
    exit /b 1
)

exit /b 0
