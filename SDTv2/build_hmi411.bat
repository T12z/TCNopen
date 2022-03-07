@echo off
pushd %~dp0
set BUILD_FAILURE=0

set BUILD_TOOLS=D:\HMI411_BUILD_TOOLS
set SDT_VERSION=0.0.0.0
set BUILD_PATH=%CD%
set GBE_FILETRANSFER_PATH=%BUILD_TOOLS%\GBEFileTransfer\1.4.0.0

echo Calling wind_base torVars.bat
call D:\WIND_BASE\host\x86-win32\bin\torVars.bat

IF "%1"=="RELEASE" (
	:: R_OUTPUT_VERSION is set in the safe build machine
	set SDT_VERSION=%R_OUTPUT_VERSION%
)

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if "%1"=="RELEASE" (
    echo :: Building TAG
) else (
    echo :: Building TRUNK
)
echo :: component_version=%SDT_VERSION%
echo :: build_environments=%build_environments%
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 
::::::::::::::::::::::::::::::::::::::::::::::
:: Call build script to build SDTv2
::::::::::::::::::::::::::::::::::::::::::::::
if not defined WIND_BASE (
    @echo Error! WIND_BASE not defined.
    goto failure
)

if not defined WIND_HOST_TYPE (
    @echo Error! WIND_HOST_TYPE not defined.
    goto failure
)

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Configure with GBE_VXWORKS_HMI_config
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
make GBE_VXWORKS_HMI_config
if errorlevel 1 (
    goto failure
)

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Clean OUTDIR
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
rmdir /S /Q output\vxworks-ppc-hmi411
rmdir /S /Q include  
make clean
if errorlevel 1 (
    goto failure
)

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Make Release Build
echo ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: 
echo make
make
if errorlevel 1 (
    goto failure
)

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Make Debug Build
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
make DEBUG=TRUE
if errorlevel 1 (
    goto failure
)

mkdir output\vxworks-ppc-hmi411
copy /Y output\vxworks-ppc-hmi411-rel\libsdt.a output\vxworks-ppc-hmi411\
mkdir include
copy /Y api\sdt_api.h include\

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Create MD5 Checksums
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::                            
pushd %BUILD_PATH%\include
%BUILD_TOOLS%\md5sum -b sdt_api.h > sdt_api.h.md5
popd

pushd %BUILD_PATH%\output\vxworks-ppc-hmi411
%BUILD_TOOLS%\md5sum -b libsdt.a > libsdt.a.md5
popd

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Copy Header and Library to the Repository Location
echo ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: 

%GBE_FILETRANSFER_PATH%\GBEFileTransfer ul c %BUILD_PATH%\output\vxworks-ppc-hmi411\* sdt %SDT_VERSION% output\vxworks-ppc-hmi411
if errorlevel 1 (
    goto failure
)
	
%GBE_FILETRANSFER_PATH%\GBEFileTransfer ul c %BUILD_PATH%\include\* sdt %SDT_VERSION% include\vxworks-ppc-hmi411
if errorlevel 1 (
   goto failure
)

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: BUILD PROCESS COMPLETED
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

goto done

:failure
echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: BUILD PROCESS FAILED
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set BUILD_FAILURE=1

:done
popd
exit /B %BUILD_FAILURE% 
@echo on
