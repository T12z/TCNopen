echo "Sweep 1 with libdirs of vxWorks"
lint-nt +v -i.\api -i.\src -i"d:\h" -i"d:\h\types" -i"d:\h\wrn\coreip" -i"d:\usr\h" .\ccuslint\au-misra2.lnt -DVXWORKS -DSDT_ENABLE_IPT -DSDT_ENABLE_MVB -DSDT_ENABLE_WTB -DSDT_ENABLE_UIC -DSDT_SECURE -DCPU=PPC603 +libdir("d:\h","d:\h\types","d:\usr\h") -wlib(0) -summary -u -zero src\sdt_validator.c src\sdt_ipt.c src\sdt_mvb.c src\sdt_uic.c src\sdt_wtb.c
echo "Sweep 2 without libdirs, to check only sdt_mutex.c"
lint-nt +v -i.\api -i.\src -i"d:\h" -i"d:\h\types" -i"d:\h\wrn\coreip" -i"d:\usr\h" .\ccuslint\au-misra2.lnt -DVXWORKS -DSDT_ENABLE_IPT -DSDT_ENABLE_MVB -DSDT_ENABLE_WTB -DSDT_ENABLE_UIC -DSDT_SECURE -DCPU=PPC603 -wlib(1) -summary -u -zero src\sdt_mutex.c
echo "Sweep 2 without libdirs, to check all except sdt_mutex.c"
lint-nt +v -i.\api -i.\src -i"d:\h" -i"d:\h\types" -i"d:\h\wrn\coreip" -i"d:\usr\h" .\ccuslint\au-misra2.lnt -DVXWORKS -DSDT_ENABLE_IPT -DSDT_ENABLE_MVB -DSDT_ENABLE_WTB -DSDT_ENABLE_UIC -DSDT_SECURE -DCPU=PPC603 +libdir("d:\h","d:\h\types","d:\usr\h") -wlib(1) -summary -u -zero src\sdt_validator.c src\sdt_ipt.c src\sdt_mvb.c src\sdt_uic.c src\sdt_wtb.c

