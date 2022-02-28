#!/bin/sh   

./test -a > ipt_latency1_log ; cat ipt_latency1_log | grep EXCEL > ipt_latency1_xlog
./test -b > ipt_cm_log ; cat ipt_cm_log | grep EXCEL > ipt_cm_xlog
./test -d > wtb_cm_log ; cat wtb_cm_log | grep EXCEL > wtb_cm_xlog                                                                                    
./test -e > uic_cm_log ; cat uic_cm_log | grep EXCEL > uic_cm_xlog                                                                                    

./test -f > ipt_indupre_log ; cat ipt_indupre_log | grep EXCEL > ipt_indupre_xlog
./test -g > mvb_indupre_log ; cat mvb_indupre_log | grep EXCEL > mvb_indupre_xlog
./test -h > wtb_indupre_log ; cat wtb_indupre_log | grep EXCEL > wtb_indupre_xlog
./test -i > uic_indupre_log ; cat uic_indupre_log | grep EXCEL > uic_indupre_xlog

./test -j > mvb_indupshort_log ; cat mvb_indupshort_log | grep EXCEL > mvb_indupshort_xlog
./test -k > mvb_induplong_log  ; cat mvb_induplong_log  | grep EXCEL > mvb_induplong_xlog
./test -l > mvb_oos_log ; cat mvb_oos_log | grep EXCEL > mvb_oos_xlog

./test -m > mvb_winitcombo_log ; cat mvb_winitcombo_log | grep EXCEL > mvb_winitcombo_xlog
./test -o > mvb_oldcombo_log ; cat mvb_oldcombo_log | grep EXCEL > mvb_oldcombo_xlog

./test -c 1 > mvb_cm1_log ; cat mvb_cm1_log | grep EXCEL > mvb_cm1_xlog                                                                                    
./test -c 2 > mvb_cm2_log ; cat mvb_cm2_log | grep EXCEL > mvb_cm2_xlog                                                                                    
./test -c 3 > mvb_cm3_log ; cat mvb_cm3_log | grep EXCEL > mvb_cm3_xlog                                                                                    
./test -c 4 > mvb_cm4_log ; cat mvb_cm4_log | grep EXCEL > mvb_cm4_xlog                                                                                    
./test -c 5 > mvb_cm5_log ; cat mvb_cm5_log | grep EXCEL > mvb_cm5_xlog                                                                                    
./test -c 6 > mvb_cm6_log ; cat mvb_cm6_log | grep EXCEL > mvb_cm6_xlog 
./test -c 7 > mvb_cm7_log ; cat mvb_cm7_log | grep EXCEL > mvb_cm7_xlog 
./test -c 8 > mvb_cm8_log ; cat mvb_cm8_log | grep EXCEL > mvb_cm8_xlog 
./test -c 9 > mvb_cm9_log ; cat mvb_cm9_log | grep EXCEL > mvb_cm9_xlog 
./test -c 10 > mvb_cm10_log ; cat mvb_cm10_log | grep EXCEL > mvb_cm10_xlog 
./test -c 11 > mvb_cm11_log ; cat mvb_cm11_log | grep EXCEL > mvb_cm11_xlog 
./test -c 12 > mvb_cm12_log ; cat mvb_cm12_log | grep EXCEL > mvb_cm12_xlog      

./test -p 1 > ipt_latency1_log ; cat ipt_latency1_log | grep EXCEL > ipt_latency1_xlog
./test -p 2 > ipt_latency2_log ; cat ipt_latency2_log | grep EXCEL > ipt_latency2_xlog
./test -p 3 > ipt_latency3_log ; cat ipt_latency3_log | grep EXCEL > ipt_latency3_xlog
./test -p 4 > ipt_latency4_log ; cat ipt_latency4_log | grep EXCEL > ipt_latency4_xlog
./test -p 5 > ipt_latency5_log ; cat ipt_latency5_log | grep EXCEL > ipt_latency5_xlog
./test -p 6 > ipt_latency6_log ; cat ipt_latency6_log | grep EXCEL > ipt_latency6_xlog
./test -p 7 > ipt_latency7_log ; cat ipt_latency7_log | grep EXCEL > ipt_latency7_xlog
./test -p 8 > ipt_latency8_log ; cat ipt_latency8_log | grep EXCEL > ipt_latency8_xlog
./test -p 9 > ipt_latency9_log ; cat ipt_latency9_log | grep EXCEL > ipt_latency9_xlog
./test -p 10 > ipt_latency10_log ; cat ipt_latency10_log | grep EXCEL > ipt_latency10_xlog

./test -q 1 > mvb_red1_log ; cat mvb_red1_log | grep EXCEL > mvb_red1_xlog
./test -q 2 > mvb_red2_log ; cat mvb_red2_log | grep EXCEL > mvb_red2_xlog
./test -q 3 > mvb_red3_log ; cat mvb_red3_log | grep EXCEL > mvb_red3_xlog    
./test -q 4 > mvb_red4_log ; cat mvb_red4_log | grep EXCEL > mvb_red4_xlog

./test -s 1 > mvb_init1_log ; cat mvb_init1_log | grep EXCEL > mvb_init1_xlog
./test -s 2 > mvb_init2_log ; cat mvb_init2_log | grep EXCEL > mvb_init2_xlog
./test -s 3 > mvb_init3_log ; cat mvb_init3_log | grep EXCEL > mvb_init3_xlog

./test -t 1 > mvb_sink_res_log ; cat mvb_sink_res_log | grep EXCEL > mvb_sink_res_xlog

./test -u 1 > uic_ed5_sink_log ; cat uic_ed5_sink_log | grep EXCEL > uic_ed5_sink_xlog

