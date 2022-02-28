echo "Sweep 1 with libdirs of vxWorks"
lint-nt +v -i.\api -i.\src -i"D:\Texas Instruments\ccsv4\tools\compiler\tms470\include" .\ccuslint\au-misra2.lnt -DARM_TI -DSDT_ENABLE_IPT -DSDT_ENABLE_MVB -DSDT_SECURE -wlib(1) -summary -u -zero src\sdt_validator.c src\sdt_ipt.c src\sdt_mvb.c src\sdt_uic.c src\sdt_wtb.c src\sdt_mutex.c
rem echo "Sweep 2 without libdirs, to check only sdt_mutex.c"
rem lint-nt +v -i.\api -i.\src -i%WIND_BASE%\target\h -i%WIND_BASE%\target\h\arch\ppc -DCPU=PPC860 -DVXWORKS -DSDT_ENABLE_IPT -DSDT_SECURE .\hmislint\au-misra2.lnt -summary -u -zero src\sdt_mutex.c
