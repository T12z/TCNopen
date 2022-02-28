all:
	ccppc -I . -I ../api ccus.c ..\output\integrity-ppc-ccu-s-rel\lib_a\libsdt_a.a ..\output\integrity-ppc-ccu-s-rel\lib_b\libsdt_b.a -o ccus-test.bin
	ccppc -OI -I . -I ../api ccus.c ..\output\integrity-ppc-ccu-s-rel\lib_a\libsdt_a.a ..\output\integrity-ppc-ccu-s-rel\lib_b\libsdt_b.a -o ccus-test-oi.bin
	ccppc -Ospeed -OI -I . -I ../api ccus.c ..\output\integrity-ppc-ccu-s-rel\lib_a\libsdt_a.a ..\output\integrity-ppc-ccu-s-rel\lib_b\libsdt_b.a -o ccus-test-oi-speed.bin
