make LINUX_armhf_config
make clean DEBUG=TRUE MD_SUPPORT=0 libtrdp libtrdpap
make libtrdp libtrdpap MD_SUPPORT=0

make ELINOS_x86_64_config
make DEBUG=TRUE MD_SUPPORT=0 libtrdp libtrdpap
make libtrdp libtrdpap MD_SUPPORT=0

make LINUX_X86_64_config
make DEBUG=TRUE MD_SUPPORT=0 libtrdp libtrdpap
make libtrdp libtrdpap MD_SUPPORT=0

