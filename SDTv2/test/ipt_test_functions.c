#define LINUX
#include "../api/sdt_api.h"
#include "test_utils.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#define SDT_SID 0x12345678U

extern const char* validity_string(sdt_validity_t v);
extern const char* result_string(sdt_result_t r);

void redTestFuncIPT(unsigned int ssc,int source/*1/2 determine which source shall secure VDP*/, int empty /*create a DUP*/,int crcOk)
{
    static int firstcall=1;
    static char pSRC1[32]={0U};
    static char pSRC2[32]={0U};
    static sdt_handle_t hnd;
    sdt_result_t        result;
    sdt_result_t        errno;
    sdt_counters_t  counters;
    int n;
    uint8_t ssc_l=0;
    uint32_t sid1, sid2;
    printf("firstcall: %d\n",firstcall);

    if (firstcall == 1)
    {         
        /*make two SIDs*/
        sdt_gen_sid(&sid1,1234,(uint8_t*)"123.567.901.345.",0);
        sdt_gen_sid(&sid2,5678,(uint8_t*)"123.567.901.345.",0);
        /*get a SINK*/   
        result = sdt_get_validator(SDT_IPT,  sid1, sid2, 1, 3, &hnd);
        printf("get_validator : %i\n",result);
        printf("handle : %i\n",hnd);
        sdt_set_sdsink_parameters(hnd,100/*ms Trx*/,120/*ms Ttx*/,5/*n_rxsafe*/,50/*n_guard*/,1000/*cmthr*/,200 /*lmi_max*/); 
        firstcall=0;
    }
        printf("---------------------------------------------------------------\n");
    if (empty != 1)
    {
        sdt_ipt_secure_pd(pSRC1, sizeof(pSRC1), sid1, 3, &ssc/*from caller*/);
        sdt_ipt_secure_pd(pSRC2, sizeof(pSRC2), sid2, 3, &ssc/*from caller*/);
    }

    if (crcOk!=1)
    {
        pSRC1[0]=0xFF;
        pSRC2[0]=0xFF;
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

    printf("sdt_validate_pd IPT: source=%d ssc=%d, valid=%s errno=%s\n",source,ssc_l,validity_string(result),result_string(errno));
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
    pSRC1[0]=0x00;
    pSRC2[0]=0x00;
}

void cmTestFuncIPT(unsigned int ssc,int crcOk)
{
    static int firstcall=1;
    //static unsigned int       ssc = 1234;
    char pCtrlPv[32]={0U};
    static sdt_handle_t hnd2;
    sdt_result_t        result;
    sdt_result_t        errno;
    sdt_counters_t  counters;
    int n;
    uint32_t ssc_l;

    if (firstcall == 1)
    {
        firstcall = 0;
        result = sdt_get_validator(SDT_IPT,  SDT_SID, 0, 0, 2, &hnd2);
        printf("get_validator : %i\n",result);
        printf("handle : %i\n",hnd2);
        sdt_set_sdsink_parameters(hnd2,120/*ms Trx*/,100/*ms Ttx*/,3/*n_rxsafe*/,2/*n_guard*/,1000/*cmthr*/,200 /*lmi_max*/);

    }
    printf("---------------------------------------------------------------\n");
    sdt_ipt_secure_pd(pCtrlPv, sizeof(pCtrlPv), SDT_SID, 2, &ssc);
    if (crcOk!=1)
    {
        /*rock the content baby*/
        pCtrlPv[0]^=1;
    }
    result = sdt_validate_pd(hnd2, pCtrlPv, sizeof(pCtrlPv));
    for (n=1;n<32;n++)
    {
        printf("%02x ", (unsigned char)pCtrlPv[n]);
    }
    printf("\n");
    sdt_get_errno(hnd2, &errno);
    sdt_get_ssc(hnd2, &ssc_l);
    printf("sdt_validate_pd IPT: ssc=%u, valid=%s errno=%s\n", ssc_l, validity_string(result), result_string(errno));
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

void Function_1(unsigned int ssc, int crcOk)
{
    static int firstcall=1;
    //static unsigned int       ssc = 1234;
    char pCtrlPv[32]={0U};
    static sdt_handle_t hnd2;
    sdt_result_t        result;
    sdt_result_t        errno;
    sdt_counters_t  counters;

    int n;
    unsigned int ssc_l;

    if (firstcall == 1)
    {
        firstcall = 0;
#if 0
        result = sdt_get_validator(SDT_IPT,  SDT_SID, 0, 0, 2, &hnd);
        printf("get_validator : %i\n",result);
        printf("handle : %i\n",hnd);
#endif
        result = sdt_get_validator(SDT_IPT,  SDT_SID, 0, 0, 2, &hnd2);
        printf("get_validator : %i\n",result);
        printf("handle : %i\n",hnd2);
        sdt_set_sdsink_parameters(hnd2,100/*ms Trx*/,120/*ms Ttx*/,5/*n_rxsafe*/,2/*n_guard*/,1000/*cmthr*/,200 /*lmi_max*/);

    }
    else
    {
#if 0
        result = sdt_validate_pd(hnd, pStatePv, sizeof(pStatePv));
        for (n=1;n<32;n++)
        {
            printf("%02x ", (unsigned char)pStatePv[n]);
        }
        printf("\n");
        sdt_get_errno(hnd, &errno);
        sdt_get_ssc(hnd, &ssc_l);
        printf("sdt_validate_pd IPT: ssc=%u, valid=%s errno=%s\n", ssc, validity_string(result), result_string(errno));
#endif
    }

    sdt_ipt_secure_pd(pCtrlPv, sizeof(pCtrlPv), SDT_SID, 2, &ssc);
    if (crcOk==1)
    {
        pCtrlPv[0]=0x00;
    }
    else
    {
        pCtrlPv[0]=0xFF;
    }
    result = sdt_validate_pd(hnd2, pCtrlPv, sizeof(pCtrlPv));
    for (n=0;n<32;n++)
    {
        printf("%02x ", (unsigned char)pCtrlPv[n]);
    }
    printf("\n");

    sdt_get_errno(hnd2, &errno);
    sdt_get_ssc(hnd2, &ssc_l);
    printf("sdt_validate_pd IPT: ssc=%u, valid=%s errno=%s\n", ssc, validity_string(result), result_string(errno));
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


void complexValiLoop(unsigned int ssc, int Trx, int Ttx, int zeroVDP)
{
    static int firstcall=1;
    //static unsigned int       ssc = 1234;
    char pCtrlPv[32]={0U};
    char pCtrlPvZERO[32]={0U};
    static sdt_handle_t hnd2;
    sdt_result_t        result;
    sdt_result_t        errno;
    sdt_counters_t  counters;

    int n;
    unsigned int ssc_l;

    if (firstcall == 1)
    {
        firstcall = 0;
        result = sdt_get_validator(SDT_IPT,  SDT_SID, 0, 0, 2, &hnd2);
        printf("get_validator : %i\n",result);
        printf("handle : %i\n",hnd2);
        sdt_set_sdsink_parameters(hnd2,Trx,Ttx,5/*n_rxsafe*/,2/*n_guard*/,1000/*cmthr*/,200 /*lmi_max*/);

    }

    sdt_ipt_secure_pd(pCtrlPv, sizeof(pCtrlPv), SDT_SID, 2, &ssc);

    if (zeroVDP !=0)
    {
      memset(pCtrlPv,0,sizeof(pCtrlPv));
    }

    result = sdt_validate_pd(hnd2, pCtrlPv, sizeof(pCtrlPv));
    for (n=0;n<32;n++)
    {
        printf("%02x ", (unsigned char)pCtrlPv[n]);
    }
    printf("\n");

    sdt_get_errno(hnd2, &errno);
    sdt_get_ssc(hnd2, &ssc_l);
    printf("sdt_validate_pd IPT: ssc=%u, valid=%s errno=%s\n", ssc, validity_string(result), result_string(errno));
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

