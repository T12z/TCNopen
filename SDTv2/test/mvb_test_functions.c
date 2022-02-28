#define LINUX
#include "../api/sdt_api.h"
#include "test_utils.h"
#include <stdio.h>
#include <stdint.h>
#define SDT_SID 0x12345678U

extern const char* validity_string(sdt_validity_t v);
extern const char* result_string(sdt_result_t r);

void cmTestFuncMVB(int ssc,int crcOk, int inaugurize)
{
    static int firstcall=1;
    //static unsigned int       ssc = 1234;
    
    char pCtrlPv[32]={0U};
    static sdt_handle_t hnd2;
    static sid = 1;
    static sid_seed=0;
    sdt_result_t        result;
    sdt_result_t        errno;
    sdt_counters_t  counters;
    int n;
    uint32_t ssc_l;
    uint8_t conv_ssc = (uint8_t)ssc;

    if (firstcall == 1)
    {
        firstcall = 0;
        result = sdt_get_validator(SDT_MVB,  sid, 0, 0, 3, &hnd2);
        printf("get_validator : %i\n",result);
        printf("handle : %i\n",hnd2);
    }
    printf("---------------------------------------------------------------\n");
    sdt_mvb_secure_pd(pCtrlPv, sizeof(pCtrlPv), sid, 3, &conv_ssc);
    if (crcOk!=1)
    {
        /*rock the content baby*/
        pCtrlPv[0]^=1;
    }
    result = sdt_validate_pd(hnd2, pCtrlPv, sizeof(pCtrlPv));
    for (n=0;n<32;n++)
    {
        printf("%02x ", (unsigned char)pCtrlPv[n]);
    }
    printf("\n");
    sdt_get_errno(hnd2, &errno);
    sdt_get_ssc(hnd2, &ssc_l);

    printf("sdt_validate_pd MVB: ssc=%d, valid=%s errno=%s\n", ssc_l, validity_string(result), result_string(errno));
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
    if (inaugurize != 0)
    {
        sid_seed++;
        sdt_gen_sid(&sid,sid_seed,(uint8_t*)"",0);
        sdt_set_sid(hnd2,sid,0,0);
        printf("called sdt_set_sid\n");
	}

}


void redTestFuncMVB(int ssc,int source/*1/2 determine which source shall secure VDP*/, int empty /*create a DUP*/,int crcOk)
{
    static int firstcall=1;
    //static unsigned int       ssc = 1234;
    static char pSRC1[32]={0U};
    static char pSRC2[32]={0U};
    static sdt_handle_t hnd;
    sdt_result_t        result;
    sdt_result_t        errno;
    sdt_counters_t  counters;
    int n;
    uint8_t ssc_l=0;
    uint32_t sid1, sid2;
    uint8_t conv_ssc = (uint8_t)ssc;

    if (firstcall == 1)
    {

        /*make two SIDs*/
        sdt_gen_sid(&sid1,1234,(uint8_t*)"",0);
        sdt_gen_sid(&sid2,5678,(uint8_t*)"",0);
        /*get a SINK*/
        firstcall = 0;
        result = sdt_get_validator(SDT_MVB,  sid1, sid2, 1, 3, &hnd);
        printf("get_validator : %i\n",result);
        printf("handle : %i\n",hnd);
    }
    printf("---------------------------------------------------------------\n");
    if (empty != 1)
    {
        sdt_mvb_secure_pd(pSRC1, sizeof(pSRC1), sid1, 3, &conv_ssc/*from caller*/);
        sdt_mvb_secure_pd(pSRC2, sizeof(pSRC2), sid2, 3, &conv_ssc/*from caller*/);
    }

    if (crcOk!=1)
    {
        /*rock the content baby*/
        if (source == 1)
        {
            pSRC1[0]^=1;
        }
        else
        {
            pSRC2[0]^=1;
        }        
    }

    if (source == 1)
    {
        result = sdt_validate_pd(hnd, pSRC1, sizeof(pSRC1));
    }
    else
    {
        result = sdt_validate_pd(hnd, pSRC2, sizeof(pSRC2));
    }
    for (n=0;n<32;n++)
    {
        if (source == 1)
            printf("%02x ", (unsigned char)pSRC1[n]);
        else
            printf("%02x ", (unsigned char)pSRC2[n]);
    }

    printf("\n");
    sdt_get_errno(hnd, &errno);
    

    printf("sdt_validate_pd MVB: source=%d ssc=%d, valid=%s errno=%s\n",source,ssc_l,validity_string(result),result_string(errno));
    printf("SDT result %i\n",result);

    sdt_get_counters(hnd, &counters);
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
    pSRC1[0]=0;
    pSRC2[0]=0;


}

