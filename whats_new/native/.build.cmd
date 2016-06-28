@echo off

rem x64
rem set platform="x86"
set platform="x64"
rem set buildtype=Debug
set buildtype=Release

set PATH=%PATH%;C:\cmake\bin
set cwd=%CD%
set trd=%cwd%\..\3rdparty

mkdir %cwd%\.build
mkdir %cwd%\.build\include
mkdir %cwd%\.build\lib
mkdir %cwd%\.build\bin

if %platform%=="x86" set vcplatform=Win32
if %platform%=="x86" set vcenvironment=x86
if %platform%=="x86" set generator="Visual Studio 14"
if %platform%=="x64" set vcplatform="x64"
if %platform%=="x64" set vcenvironment=amd64
if %platform%=="x64" set generator="Visual Studio 14 Win64"

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" %vcenvironment%


set INCLUDE=%cwd%\.build\include;%INCLUDE%
set LIB=%cwd%\.build\lib;%LIB%

echo   Platform: %platform%
echo VcPlatform: %vcplatform%
echo      Build: %buildtype%
echo  Generator: %generator%
echo        CWD: %cwd%
echo Downloader: %downloader%

rd /s /q %cwd%\.build\support
mkdir %cwd%\.build\support

cmake %cwd% -B%cwd%\.build\support -G %generator% -DPROJECT_PLATFORM=%vcplatform% -DJAVA_HOME=C:\jdk1.8.0_91 -DCMAKE_BUILD_TYPE=%buildtype% -DCMAKE_CONFIGURATION_TYPES="Debug;Release"
msbuild %cwd%\.build\support\support.sln /p:Platform=%vcplatform% /p:ReleaseBuild=true /p:Configuration=%buildtype%

rem %zipper% a %cwd%\.build\support_%platform%.zip ^
rem            %cwd%\.build\support\bin\%buildtype%\*.exe ^
rem            %cwd%\.build\support\bin\%buildtype%\*.dll ^
rem            %trd%\ftdi\%platform%\*.dll ^
rem            %trd%\redist.vs2015\vc_redist.%platform%.exe
rem %zipper% a %cwd%\.build\support-devel_%platform%.zip ^
rem            %cwd%\.build\include ^
rem            %cwd%\.build\lib

exit /b

