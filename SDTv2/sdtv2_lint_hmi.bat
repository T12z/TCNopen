echo "Sweep 1 with options.lnt from HMI-S"
set WIND_BASE=d:\WIND_BASE
lint-nt +v -i.\api -i.\src -DVXWORKS -DSDT_ENABLE_IPT -DSDT_SECURE .\hmislint\au-misra2.lnt .\hmislint\options.lnt  -summary -u -zero src\sdt_validator.c src\sdt_ipt.c src\sdt_mvb.c src\sdt_uic.c src\sdt_wtb.c src\sdt_mutex.c
echo "Sweep 2 with libdirs of vxWorks"
lint-nt +v -i.\api -i.\src -i%WIND_BASE%\target\h -i%WIND_BASE%\target\h\arch\ppc -DCPU=PPC860 -DVXWORKS -DSDT_ENABLE_IPT -DSDT_SECURE .\hmislint\au-misra2.lnt -wlib(1) +libdir("%WIND_BASE%/target/h") +libdir("%WIND_BASE%/target/h/drv/timer") +libdir("%WIND_BASE%/target/h/arch/ppc") -summary -u -zero src\sdt_validator.c src\sdt_ipt.c src\sdt_mvb.c src\sdt_uic.c src\sdt_wtb.c src\sdt_mutex.c
echo "Sweep 3 without libdirs, to check only sdt_mutex.c"
lint-nt +v -i.\api -i.\src -i%WIND_BASE%\target\h -i%WIND_BASE%\target\h\arch\ppc -DCPU=PPC860 -DVXWORKS -DSDT_ENABLE_IPT -DSDT_SECURE .\hmislint\au-misra2.lnt -summary -u -zero src\sdt_mutex.c
