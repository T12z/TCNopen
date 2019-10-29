make LINUX_armhf_config
make clean DEBUG=TRUE libtrdp
make libtrdp

make ELINOS_x86_64_config
make DEBUG=TRUE libtrdp
make libtrdp

make LINUX_X86_64_config
make DEBUG=TRUE libtrdp
make libtrdp

