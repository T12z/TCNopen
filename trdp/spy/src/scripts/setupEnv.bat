@echo off
set WORKDIR=%cd%
set CYGWIN=nodosfilewarning
set WIRESHARK_BASE_DIR=%WORKDIR%
set WIRESHARK_TARGET_PLATFORM=win64
set QT5_BASE_DIR=C:\Qt\5.9.5\msvc2017_64

set WIRESHARK_VERSION_EXTRA=TRDP
set POWERSHELL=C:\Windows\winsxs\amd64_microsoft-windows-powershell-exe_31bf3856ad364e35_6.1.7600.16385_none_c50af05b1be3aa2b\
set PATH=%POWERSHELL%;%PATH%;C:\Program Files\CMake\bin
set PLATFORM=win64
set VisualStudioVersion=17.0

rem Add the Visual Studio Build script
rem set PATH=%PATH%;C:\Program Files (x86)\MSBuild\10.0\Bin

rem If your Cygwin installation path is not automatically detected by CMake:
set WIRESHARK_CYGWIN_INSTALL_PATH=C:\cygwin

rem Python with PIP: python module installation tool;
set PATH=%PATH%;C:\Python37;C:\Python37\Scripts;%WIRESHARK_CYGWIN_INSTALL_PATH%\bin

rem SET Proxy if necessary
rem set http_proxy=http://<my proxy>:8080
rem set https_proxy=http://<my proxy>:8080

echo =============================================
echo 
echo Run the following command to setup the IDE
rem  cmake -DENABLE_CHM_GUIDES=on -G "Visual Studio 10 2010" ..\wireshark
echo cmake -DENABLE_CHM_GUIDES=on -G "Visual Studio 15 2017 Win64" ..\wireshark
