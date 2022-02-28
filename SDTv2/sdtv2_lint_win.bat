echo "Sweep 1: sdt_util.h treated as library header"
lint-nt +v -i.\api -i.\src -DINTEGRITY -DSDT_ENABLE_IPT -DSDT_ENABLE_MVB -DSDT_ENABLE_WTB -DSDT_SECURE -DOS_INTEGRITY .\ccuslint\au-misra2.lnt  -summary -u -zero src\sdt_validator.c src\sdt_ipt.c src\sdt_mvb.c src\sdt_uic.c src\sdt_wtb.c src\sdt_mutex.c
echo "Sweep 2: sdt_util.h included in check, to scrutinize its internals"
lint-nt +v -i.\api -i.\src -DINTEGRITY -DSDT_ENABLE_IPT -DSDT_ENABLE_MVB -DSDT_ENABLE_WTB -DSDT_SECURE -DOS_INTEGRITY .\ccuslint\rules-C3.lnt  -summary -u -zero src\sdt_validator.c src\sdt_ipt.c src\sdt_mvb.c src\sdt_uic.c src\sdt_wtb.c src\sdt_mutex.c
echo "Sweep 3: sdt_util.h included in check, to scrutinize its internals"
lint-nt +v -i.\api -i.\src -DINTEGRITY -DSDT_ENABLE_IPT -DSDT_ENABLE_MVB -DSDT_ENABLE_WTB -DSDT_SECURE -DOS_INTEGRITY .\ccuslint\integrity-C3.lnt  -summary -u -zero src\sdt_validator.c src\sdt_ipt.c src\sdt_mvb.c src\sdt_uic.c src\sdt_wtb.c src\sdt_mutex.c
