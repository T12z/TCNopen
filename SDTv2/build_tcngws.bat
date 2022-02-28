@echo off

set BUILD_FAILURE=0

set BUILD_TOOLS=d:\buildtools
set SDT_VERSION=0.0.0.0
set BUILD_PATH=%CD%
set GBE_FILETRANSFER_PATH=%BUILD_TOOLS%\GBEFileTransfer\1.4.0.0
set LINT_HOME = %BUILD_TOOLS%\lint\pclint\9.00i

REM setting version of used sub-projects to be downlaoded via SVN
IF "%1"=="RELEASE" (
	set build_environments=tags/1.4.3.0
	:: R_OUTPUT_VERSION is set in the safe build machine
	set SDT_VERSION=%R_OUTPUT_VERSION%
) else (
	REM trunk is built
	set build_environments=tags/0.0.0.0
	set VERSION=trunk
	:: set from jenkins
	set r_username=%build_username%
	set r_password=%build_password%
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

if "%1"=="RELEASE" (
    echo.
    echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    echo :: Fetch defined Tool-Environment
    echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    %GBE_FILETRANSFER_PATH%\GBEFileTransfer dl b mtpe_build_env  "D:/buildtools/mtpe"
    %GBE_FILETRANSFER_PATH%\GBEFileTransfer dl b "tcn-gw/TMS470 Code Generation Tools/5.0.4.0/*" "D:/buildtools/TMS470_Code_Generation_Tools/5.0.4.0"
)

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Configure with GBE_TCNGWS_config
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set GMAKE=%BUILD_TOOLS%\mtpe\1.0.0.0\gmake
%GMAKE% GBE_TCNGWS_config
if errorlevel 1 (
    goto failure
)

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Clean OUTDIR
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
rmdir /S /Q output\proprietary-tcngws-tms470
rmdir /S /Q include 
%GMAKE% clean
if errorlevel 1 (
    goto failure
)

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Make Release Build
echo ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: 
%GMAKE%
if errorlevel 1 (
    goto failure
)

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Make Debug Build
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
%GMAKE% DEBUG=TRUE
if errorlevel 1 (
    goto failure
)

mkdir output\proprietary-tcngws-tms470
copy /Y output\ti-arm-gw-s-rel\libsdt.a output\proprietary-tcngws-tms470\
mkdir include
copy /Y api\sdt_api.h include\

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Create MD5 Checksums
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::                            
pushd %BUILD_PATH%\include
%BUILD_TOOLS%\md5deep\md5deep.exe -b sdt_api.h > sdt_api.h.md5
popd
     
pushd %BUILD_PATH%\output\proprietary-tcngws-tms470
%BUILD_TOOLS%\md5deep\md5deep.exe libsdt.a > libsdt.a.md5
popd

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Copy Header and Library to the Repository Location
echo ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: 
%GBE_FILETRANSFER_PATH%\GBEFileTransfer ul c %BUILD_PATH%\output\proprietary-tcngws-tms470\* sdt %SDT_VERSION% output\proprietary-tcngws-tms470
if errorlevel 1 (
    goto failure
)

%GBE_FILETRANSFER_PATH%\GBEFileTransfer ul c %BUILD_PATH%\include\* sdt %SDT_VERSION% include\proprietary-tcngws-tms470
if errorlevel 1 (
   goto failure
)
    
echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: BUILD PROCESS COMPLETED
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

::::::::::::::::::::::::::::::::::::::::::::::
:: Call lint script
::::::::::::::::::::::::::::::::::::::::::::::
::call ..\run_lint.bat

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