#define LINUX
#include "../api/sdt_api.h"
#include "test_utils.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#define SDT_SID 0x12345678U

extern const char* validity_string(sdt_validity_t v);
extern const char* result_string(sdt_result_t r);
static void __test_set_be32(uint8_t buf[], uint16_t offset, uint32_t value)
{
    buf[offset   ] = (uint8_t)((value >> 24) & 0xffU);
    buf[offset+1U] = (uint8_t)((value >> 16) & 0xffU);
    buf[offset+2U] = (uint8_t)((value >>  8) & 0xffU);
    buf[offset+3U] = (uint8_t)( value        & 0xffU);
}
void cmTestFuncUIC(int ssc,int crcOk)
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
    char udv=2;
    pCtrlPv[0]=3;/*R3 telegram*/
    memcpy(&pCtrlPv[14],&ssc,4);
    pCtrlPv[32]=udv;
    if (firstcall == 1)
    {
        firstcall = 0;
        result = sdt_get_validator(SDT_UIC,  SDT_SID, 0, 0, 2, &hnd2);
        printf("get_validator : %i\n",result);
        printf("handle : %i\n",hnd2);
    }
    printf("---------------------------------------------------------------\n");
    sdt_uic_secure_pd(pCtrlPv, 40, SDT_SID);
    printf("generated UIC telegram: ");
    for (n=0;n<40;n++)
    {
        printf("%02x ", (unsigned char)pCtrlPv[n]);
    }
    printf("\n");
    if (crcOk!=1)
    {
        /*rock the content baby*/
        pCtrlPv[5]^=1;
    }
    result = sdt_validate_pd(hnd2, pCtrlPv, sizeof(pCtrlPv));
    printf("received  UIC telegram: ");

    for (n=0;n<40;n++)
    {
        printf("%02x ", (unsigned char)pCtrlPv[n]);
    }
    printf("\n");
    sdt_get_errno(hnd2, &errno);
    sdt_get_ssc(hnd2, &ssc_l);
    printf("sdt_validate_pd UIC: ssc=%d, valid=%s errno=%s\n", ssc_l, validity_string(result), result_string(errno));
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

void ed5TestFuncUIC(void)
{
    static int firstcall=1;
    //static unsigned int       ssc = 1234;
    char pCtrlPv[40]={0U};
    static sdt_handle_t hnd2;
    sdt_result_t        result;
    sdt_result_t        errno;
    sdt_counters_t  counters;
    int n;
    uint32_t ssc = 0x00004000;
    uint32_t ssc_l;
    char udv=2;
    uint32_t crcOk = 1;
    uint32_t fill_display;
    pCtrlPv[0]=3;/*R3 telegram*/
    pCtrlPv[32]=udv;
    __test_set_be32(pCtrlPv,14,ssc);
    /*memcpy(&pCtrlPv[14],&ssc,4);*/
    if (firstcall == 1)
    {
        firstcall = 0;
        result = sdt_get_validator(SDT_UIC,  SDT_SID, 0, 0, 2, &hnd2);
        printf("get_validator : %i\n",result);
        printf("handle : %i\n",hnd2);
    }
    printf("---------------------------------------------------------------\n");
    sdt_uic_secure_pd(pCtrlPv, 40, SDT_SID);
    printf("generated UIC telegram: ");
    for (n=0;n<40;n++)
    {
        printf("%02x ", (unsigned char)pCtrlPv[n]);
    }
    printf("\n");
    if (crcOk!=1)
    {
        /*rock the content baby*/
        pCtrlPv[5]^=1;
    }
    result = sdt_validate_pd(hnd2, pCtrlPv, sizeof(pCtrlPv));
    printf("received  UIC telegram: ");

    for (n=0;n<40;n++)
    {
        printf("%02x ", (unsigned char)pCtrlPv[n]);
    }
    printf("\n");
    sdt_get_uic_fillvalue(hnd2,&fill_display);
    printf("fillvalue UIC: fillvaue=%x\n", fill_display);
    sdt_get_errno(hnd2, &errno);
    sdt_get_ssc(hnd2, &ssc_l);
    printf("sdt_validate_pd UIC: ssc=%d, valid=%s errno=%s\n", ssc_l, validity_string(result), result_string(errno));
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
    printf("Now turn SINK into ed5 - will cause CRC error - see counter\n");
    printf("---------------------------------------------------------------\n");
    ssc = 0x00005a00;
    __test_set_be32(pCtrlPv,14,ssc);
    sdt_uic_secure_pd(pCtrlPv, 40, SDT_SID);
    printf("generated UIC telegram: ");
    for (n=0;n<40;n++)
    {
        printf("%02x ", (unsigned char)pCtrlPv[n]);
    }
    printf("\n");
    if (crcOk!=1)
    {
        /*rock the content baby*/
        pCtrlPv[5]^=1;
    }
    sdt_set_uic_fillvalue(hnd2,0x12341234);
    result = sdt_validate_pd(hnd2, pCtrlPv, sizeof(pCtrlPv));
    printf("received  UIC telegram: ");

    for (n=0;n<40;n++)
    {
        printf("%02x ", (unsigned char)pCtrlPv[n]);
    }
    printf("\n");
    sdt_get_uic_fillvalue(hnd2,&fill_display);
    printf("fillvalue UIC: fillvaue=%x\n", fill_display);
    sdt_get_errno(hnd2, &errno);
    sdt_get_ssc(hnd2, &ssc_l);
    printf("sdt_validate_pd UIC: ssc=%d, valid=%s errno=%s\n", ssc_l, validity_string(result), result_string(errno));
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
    printf("Now use ed5 SRC with ed5 SINK\n");
    printf("---------------------------------------------------------------\n");

    sdt_uic_ed5_secure_pd(pCtrlPv, 40, SDT_SID,0x12341234);
    printf("generated UIC telegram: ");
    for (n=0;n<40;n++)
    {
        printf("%02x ", (unsigned char)pCtrlPv[n]);
    }
    printf("\n");
    if (crcOk!=1)
    {
        /*rock the content baby*/
        pCtrlPv[5]^=1;
    }
    
    result = sdt_validate_pd(hnd2, pCtrlPv, sizeof(pCtrlPv));
    printf("received  UIC telegram: ");

    for (n=0;n<40;n++)
    {
        printf("%02x ", (unsigned char)pCtrlPv[n]);
    }
    printf("\n");
    sdt_get_errno(hnd2, &errno);
    sdt_get_uic_fillvalue(hnd2,&fill_display);
    printf("fillvalue UIC: fillvaue=%x\n", fill_display);
    sdt_get_ssc(hnd2, &ssc_l);
    printf("sdt_validate_pd UIC: ssc=%d, valid=%s errno=%s\n", ssc_l, validity_string(result), result_string(errno));
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
}
