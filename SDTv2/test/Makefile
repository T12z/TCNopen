
CFILES = ipt_test_functions.c\
         ipt_test_scenarios.c\
         mvb_test_functions.c\
         mvb_test_scenarios.c\
         uic_test_functions.c\
         uic_test_scenarios.c\
         wtb_test_functions.c\
         wtb_test_scenarios.c\
	 etc_test_scenarios.c\
	 etc_test_functions.c\
	 test_utils.c\
         test.c


all:
	gcc -m32 -Wall -O2 $(CFILES) ../output/linux-x86-rel/libsdt.a -o test
	gcc -g -m32 $(CFILES) ../output/linux-x86-dbg/libsdt.a -o test_dbg
