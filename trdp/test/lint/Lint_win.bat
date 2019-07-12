"c:\Program Files (x86)\Lint\Lint-nt" +v -i"c:\Program Files (x86)\Lint\lnt" std_win.lnt -DWIN32;MD_SUPPORT=1;L_ENDIAN;_CRT_SECURE_NO_WARNINGS;_WINSOCK_DEPRECATED_NO_WARNINGS -os(_LINT_WIN.TMP) vos_utils.c vos_mem.c vos_thread.c vos_sock.c vos_shared_mem.c tlc_if.c tlp_if.c tlm_if.c  trdp_mdcom.c trdp_pdcom.c trdp_pdindex.c trdp_utils.c trdp_stats.c tau_marshall.c trdp_xml.c tau_xml.c tau_ctrl.c tau_dnr.c tau_tti.c
type _LINT_WIN.TMP | more
@echo off
echo ---
echo  output placed in _LINT_WIN.TMP
echo --- 
