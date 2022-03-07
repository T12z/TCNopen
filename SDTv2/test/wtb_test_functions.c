#define LINUX
#include "../api/sdt_api.h"
#include <stdio.h>
#include <stdint.h>
#define SDT_SID 0x12345678U

extern const char* validity_string(sdt_validity_t v);
extern const char* result_string(sdt_result_t r);

void cmTestFuncWTB(unsigned int ssc,int crcOk)
{
    static int firstcall=1;
    //static unsigned int       ssc = 1234;
    char pCtrlPv[40]={0U};
    static sdt_handle_t hnd2;
    sdt_result_t        result;
    sdt_result_t        errno;
    sdt_counters_t  counters;
    int n;
    uint32_t ssc_l;
    pCtrlPv[0]=0x30;/*R3 telegram*/
    pCtrlPv[31]=99; /*minor udv*/
    if (firstcall == 1)
    {
        firstcall = 0;
        result = sdt_get_validator(SDT_WTB,  SDT_SID, 0, 0, 2, &hnd2);
        printf("get_validator : %i\n",result);
        printf("handle : %i\n",hnd2);
    }
    printf("---------------------------------------------------------------\n");
    sdt_wtb_secure_pd(pCtrlPv, SDT_SID, 2, &ssc);
    if (crcOk!=1)
    {
        /*rock the content baby*/
        pCtrlPv[5]^=1;
    }
    result = sdt_validate_pd(hnd2, pCtrlPv, sizeof(pCtrlPv));
    for (n=0;n<40;n++)
    {
        printf("%02x ", (unsigned char)pCtrlPv[n]);
    }
    printf("\n");
    sdt_get_errno(hnd2, &errno);
    sdt_get_ssc(hnd2, &ssc_l);
    printf("sdt_validate_pd WTB: ssc=%d, valid=%s errno=%s\n", ssc_l, validity_string(result), result_string(errno));
    printf("SDT result %i\n",result);

    sdt_get_counters(hnd2, &counters);
    printf("sdt_counters: rx(%u) err(%u) sid(%u) oos(%u) dpl(%u) udv(%u) lmg(%u)\n", 
           counters.rx_count,
           counters.err_count,
           counters.sid_count,
           counters.oos_count,
           counters.dpl_count,
           counters.udv_count,
           counters.lmg_count);
printf("EXCEL, %d, %d, %d, %d, %d, %d, %d\n", ssc_l, result, errno, counters.rx_count,counters.err_count,counters.oos_count,counters.dpl_count);

    printf("---------------------------------------------------------------\n");

}

