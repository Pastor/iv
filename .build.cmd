@echo off


set sqlite_release=sqlite-amalgamation-3110100
rem x64
set platform="x86"
rem set platform="x64"
set buildtype=Debug
rem set buildtype=Release

set PATH=%PATH%;C:\cmake\bin
set cwd=%CD%
set downloader=%cwd%\.build\utils\bin\%buildtype%\hget.exe
set unzipper=%cwd%\utils\7za 
set zipper=%cwd%\utils\7za
set git=%githome%\git.exe

mkdir %cwd%\.build
mkdir %cwd%\.build\include
mkdir %cwd%\.build\lib

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

call :Depends
rem exit /b

rd /s /q %cwd%\.build\main
mkdir %cwd%\.build\main

cmake %cwd% -B%cwd%\.build\main -G %generator% ^
                                -DMongoose_DIR=%cwd%\.build\mongoose-master ^
                                -DSQLite_DIR=%cwd%\.build\%sqlite_release% ^
                                -DSTATIC=1 ^
                                -DCMAKE_BUILD_TYPE=%buildtype%
msbuild %cwd%\.build\main\main.sln /p:Platform=%vcplatform% /p:ReleaseBuild=true /p:Configuration=%buildtype%

%zipper% a %cwd%\.build\main_%platform%.zip ^
           %cwd%\.build\main\bin\%buildtype%\*.exe ^
           %cwd%\.build\main\bin\%buildtype%\*.dll ^
           %cwd%\3rdparty\ftdi\%platform%\*.dll ^
           %cwd%\3rdparty\redist.vs2015\vc_redist.%platform%.exe
%zipper% a %cwd%\.build\main-devel_%platform%.zip ^
           %cwd%\.build\include ^
           %cwd%\.build\lib

exit /b

rem ==========================================
:Depends

echo Utils libtaties and programs
rd /s /q %CWD%\.build\utils
cmake -H%cwd%\utils -B%cwd%\.build\utils -G %generator% -DCMAKE_BUILD_TYPE=%buildtype%
msbuild %cwd%\.build\utils\utils.sln /p:Platform=%vcplatform% /p:ReleaseBuild=true /p:Configuration=%buildtype%

del /q %cwd%\.build\mongoose-master.zip
del /q %cwd%\.build\%sqlite_release%.zip

%downloader% https://github.com/cesanta/mongoose/archive/master.zip        %cwd%\.build\mongoose-master.zip
%downloader% https://www.sqlite.org/2016/%sqlite_release%.zip              %cwd%\.build\%sqlite_release%.zip

rd /s /q %cwd%\.build\mongoose-master
rd /s /q %cwd%\.build\%sqlite_release%

%unzipper% x %cwd%\.build\mongoose-master.zip           -o%cwd%\.build
%unzipper% x %cwd%\.build\%sqlite_release%.zip          -o%cwd%\.build

exit /b