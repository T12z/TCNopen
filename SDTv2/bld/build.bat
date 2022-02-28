REM @echo on
pushd %~dp0
set BUILD_FAILURE=0

echo Welcome to build.bat, sit back and have a nice build.

if [%1]==[] (
    echo Error: No build.bat parameter!
    goto failure
) else (
    set BUILD_PARAM=%1
)

cd ..

echo Build_param is set to: %BUILD_PARAM%
goto %BUILD_PARAM%
:: if label do not exist system will report error and exit
:: but just in case
echo If you see this message, most likely the Build_param was set wrong!
goto failure

:hmi411_release
echo calling build_hmi411.bat RELEASE
call build_hmi411.bat RELEASE
if errorlevel 1 (
	goto failure
)
goto done

:hmi411
echo calling build_hmi411.bat 
call build_hmi411.bat
if errorlevel 1 (
    goto failure
)
goto done

:ccus_release
call build_ccus.bat RELEASE
if errorlevel 1 (
    goto failure
)
goto done

:ccus
call build_ccus.bat
if errorlevel 1 (
    goto failure
)
goto done

:tcngws_release
call build_tcngws.bat RELEASE
if errorlevel 1 (
    goto failure
)
goto done

:tcngws
call build_tcngws.bat
if errorlevel 1 (
    goto failure
)
goto done

:win32
call "%VS90COMNTOOLS%\vsvars32.bat"
set SDT_VERSION=0.0.0.0
IF "%2"=="RELEASE" (
    set SDT_VERSION=%VERSION%
)
echo [Build] SDTv2 Lib Release Win32
cd ms_visual_studio
vcbuild.exe sdt2.vcproj "Release|Win32"
if errorlevel 1 (
    goto failure
)
mkdir output
mkdir include
 
cd Release\dll
D:\build_tools\md5sum.exe sdt2.dll > sdt2.dll.md5

cd ..\..\..\api
D:\build_tools\md5sum.exe sdt_api.h  > sdt_api.h.md5

cd ..\ms_visual_studio
copy /Y Release\dll\sdt2.dll* output
copy /Y Release\dll\sdt2.lib output
copy /Y ..\api\sdt_api.* include

R:\buildtools\GBEFileTransfer\1.4.0.0\GBEFileTransfer.exe ul c output\* sdt %SDT_VERSION% output\win32\
R:\buildtools\GBEFileTransfer\1.4.0.0\GBEFileTransfer.exe ul c include\* sdt %SDT_VERSION% include\win32
goto done


:failure
set BUILD_FAILURE=1

:done
popd
exit /B %BUILD_FAILURE% 
@echo on
