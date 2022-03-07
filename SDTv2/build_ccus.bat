@echo off
pushd %~dp0
set BUILD_FAILURE=0

set BUILD_TOOLS=d:\buildtools
set SDT_VERSION=0.0.0.0
set BUILD_PATH=%CD%
set GBE_FILETRANSFER_PATH=%BUILD_TOOLS%\GBEFileTransfer\1.5.0.0
set LINT_HOME = %BUILD_TOOLS%\lint\pclint\9.00i
:: set GHS_LMHOST = 192.168.58.20
:: set GHS_LMWHICH = ghs
PATH = D:\GHS\ppc424_M6B;%LINT_HOME%;%PATH%

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
         
if "%1"=="RELEASE" (
    echo.
    echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    echo :: Fetch defined Tool-Environment
    echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    %GBE_FILETRANSFER_PATH%\GBEFileTransfer dl tc ccus\main\1.0.0.0\ppc424_M6B\* D:\GHS\ppc424_M6B
    dir d:\ghs
)

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Configure with GBE_CCUS_config
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::             
%BUILD_TOOLS%\gmake GBE_CCUS_config
if errorlevel 1 (
    goto failure
)  

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Clean OUTDIR
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
rmdir /S /Q output\integrity-ppc-ccu-s\
%BUILD_TOOLS%\gmake clean
if errorlevel 1 (
    goto failure
)

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Make Release Build
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::            
%BUILD_TOOLS%\gmake
if errorlevel 1 (
    goto failure
)

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Make Debug Build
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
%BUILD_TOOLS%\gmake DEBUG=TRUE
if errorlevel 1 (
    goto failure
)

mkdir output\integrity-ppc-ccu-s\lib_a
mkdir output\integrity-ppc-ccu-s\lib_b
copy /Y output\integrity-ppc-ccu-s-rel\lib_a\libsdt_a.a output\integrity-ppc-ccu-s\lib_a
copy /Y output\integrity-ppc-ccu-s-rel\lib_b\libsdt_b.a output\integrity-ppc-ccu-s\lib_b
mkdir include
copy /Y api\sdt_api_dual.h include\

echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Create MD5 Checksums
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::              
pushd %BUILD_PATH%\output\integrity-ppc-ccu-s\lib_a
%BUILD_TOOLS%\tbci_md5sum.exe libsdt_a.a > libsdt_a.a.md5
popd
pushd %BUILD_PATH%\output\integrity-ppc-ccu-s\lib_b
%BUILD_TOOLS%\tbci_md5sum.exe libsdt_b.a > libsdt_b.a.md5
popd
pushd %BUILD_PATH%\include
%BUILD_TOOLS%\tbci_md5sum.exe sdt_api_dual.h > sdt_api_dual.h.md5
popd


echo.
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo :: Copy Header and Library to the Repository Location
echo :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::               

%GBE_FILETRANSFER_PATH%\GBEFileTransfer ul c %BUILD_PATH%\output\integrity-ppc-ccu-s\* sdt %SDT_VERSION% output/integrity-ppc-ccu-s
if errorlevel 1 (
    goto failure
)

%GBE_FILETRANSFER_PATH%\GBEFileTransfer ul c %BUILD_PATH%\include\* sdt %SDT_VERSION% include/integrity-ppc-ccu-s
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
