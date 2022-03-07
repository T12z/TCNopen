/*******************************************************************************
 *  COPYRIGHT     : This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, 
 *                  You can obtain one at http://mozilla.org/MPL/2.0/.
 *                  Copyright Alstom Transport SA or its subsidiaries and others, 2011-2022. All rights reserved. 
 *******************************************************************************

#include "sdt_api_dual.h"

#define SDT_SID 0x12345678U

#define RESULTS(NAME)	case NAME: return #NAME;
#define DEF_UNKNOWN()	default:   return "UNKNOWN";

void Function_1(int ssc);          
void helpme(void);

void cmTestFuncIPT(int ssc,int crcOk);
void cmTestFuncMVB(int ssc,int crcOk);
void cmTestFuncWTB(int ssc,int crcOk);
void cmTestFuncUIC(int ssc,int crcOk);

void iptLatencyRun(void);

void iptCMRun(void);
void mvbCMRun(void);
void wtbCMRun(void);
void uicCMRun(void);

void iptScen1(void);
void mvbScen1(void);
void wtbScen1(void);
void uicScen1(void);

void useAllSdtFuncs_ccus(U32 ssc);

const char* validity_string(sdt_validity_t v)
{
    switch (v)
    {
        RESULTS(SDT_FRESH)
        RESULTS(SDT_INVALID)
        RESULTS(SDT_ERROR)
        DEF_UNKNOWN()       
    }
}

const char* result_string(sdt_result_t r)
{
    switch (r)
    {
        RESULTS(SDT_OK)
        RESULTS(SDT_ERR_SIZE)
        RESULTS(SDT_ERR_VERSION)
        RESULTS(SDT_ERR_HANDLE)
        RESULTS(SDT_ERR_CRC)
        RESULTS(SDT_ERR_DUP)
        RESULTS(SDT_ERR_LOSS)
        RESULTS(SDT_ERR_SID)
        RESULTS(SDT_ERR_PARAM)
        RESULTS(SDT_ERR_REDUNDANCY)
        RESULTS(SDT_ERR_SYS)
        RESULTS(SDT_ERR_LTM)
        RESULTS(SDT_ERR_INIT)
        RESULTS(SDT_ERR_CMTHR)
        DEF_UNKNOWN()       
    }
}



int main(int argc, char** argv)
{
    int i;
    if (argc==1)
    {
        return 1;
    }
    /*commandline extension*/
    useAllSdtFuncs_ccus(1);
    return 0;
}






void useAllSdtFuncs_ccus(U32 ssc)
{
    static int firstcall=1;
    U32 sidIpt_A;
    U32 sidMvb_A;
    U32 sidWtb_A;
    U32 sidUic_A;

    U32 sidIpt_B;
    U32 sidMvb_B;
    U32 sidWtb_B;
    U32 sidUic_B;

    U32 fill_A;
    U32 fill_B;

    char iptRecBuf_A[32]={0U};
    char mvbRecBuf_A[32]={0U};
    char uicRecBuf_A[40]={0U};
    char wtbRecBuf_A[40]={0U};

    char iptRecBuf_B[32]={0U};
    char mvbRecBuf_B[32]={0U};
    char uicRecBuf_B[40]={0U};
    char wtbRecBuf_B[40]={0U};

    static sdt_handle_t iptHnd_A;
    static sdt_handle_t iptHnd_B;
    static sdt_handle_t mvbHnd_A;
    static sdt_handle_t mvbHnd_B;
    static sdt_handle_t uicHnd_A;
    static sdt_handle_t uicHnd_B;
    static sdt_handle_t wtbHnd_A;
    static sdt_handle_t wtbHnd_B;

    sdt_result_t        errno;
    sdt_result_t        result;
    unsigned int ssc_l;

    if (firstcall == 1)
    {
        firstcall = 0;
        result = sdt_gen_sid_A(&sidIpt_A, (U32)100,"local",0);
        result = sdt_gen_sid_A(&sidMvb_A, (U32)200,"local",0);
        result = sdt_gen_sid_A(&sidUic_A, (U32)300,"local",0);
        result = sdt_gen_sid_A(&sidWtb_A, (U32)400,"local",0);

        result = sdt_gen_sid_B(&sidIpt_B, (U32)100,"local",0);
        result = sdt_gen_sid_B(&sidMvb_B, (U32)200,"local",0);
        result = sdt_gen_sid_B(&sidUic_B, (U32)300,"local",0);
        result = sdt_gen_sid_B(&sidWtb_B, (U32)400,"local",0);

        sdt_get_validator_A(SDT_IPT,  sidIpt_A, 0, 0, 2, &iptHnd_A);
#if 0
        sdt_set_lat_moni_params_A(iptHnd_A,150/*ms SRC*/,100/*ms SINK*/,200 /* cycles */);
#endif
        sdt_set_sdsink_parameters_A(iptHnd_A, 100/*ms SINK*/, 150/*ms SRC*/, 10, 5, 300,20);

        sdt_get_validator_B(SDT_IPT,  sidIpt_B, 0, 0, 2, &iptHnd_B);
#if 0
        sdt_set_lat_moni_params_B(iptHnd_B,150/*ms SRC*/,100/*ms SINK*/,200 /* cycles */);
#endif
        sdt_set_sdsink_parameters_B(iptHnd_B, 100/*ms SINK*/, 150/*ms SRC*/, 10, 5, 300,20);
        sdt_get_validator_A(SDT_MVB,  sidMvb_A, 0, 0, 0x20, &mvbHnd_A);
        sdt_get_validator_B(SDT_MVB,  sidMvb_B, 0, 0, 0x20, &mvbHnd_B);

        sdt_get_validator_A(SDT_UIC,  sidUic_A, 0, 0, 2, &uicHnd_A);
        sdt_get_validator_B(SDT_UIC,  sidUic_B, 0, 0, 2, &uicHnd_B);

        sdt_get_validator_A(SDT_WTB,  sidWtb_A, 0, 0, 2, &wtbHnd_A);
        sdt_get_validator_B(SDT_WTB,  sidWtb_B, 0, 0, 2, &wtbHnd_B);
    }

    sdt_ipt_secure_pd_A(iptRecBuf_A, sizeof(iptRecBuf_A), sidIpt_A, 2, &ssc);
    sdt_validate_pd_A(iptHnd_A, iptRecBuf_A, sizeof(iptRecBuf_A));
    
    sdt_ipt_secure_pd_B(iptRecBuf_B, sizeof(iptRecBuf_B), sidIpt_B, 2, &ssc);
    sdt_validate_pd_B(iptHnd_B, iptRecBuf_B, sizeof(iptRecBuf_B));

    sdt_mvb_secure_pd_A(mvbRecBuf_A, sizeof(mvbRecBuf_A), sidMvb_A, 0x20, &ssc);
    sdt_validate_pd_A(mvbHnd_A, mvbRecBuf_A, sizeof(mvbRecBuf_A));
    
    sdt_mvb_secure_pd_B(mvbRecBuf_B, sizeof(mvbRecBuf_B), sidMvb_B, 0x20, &ssc);
    sdt_validate_pd_B(mvbHnd_B, mvbRecBuf_B, sizeof(mvbRecBuf_B));

    sdt_uic_secure_pd_A(uicRecBuf_A, sizeof(uicRecBuf_A), sidUic_A);
    sdt_uic_ed5_secure_pd_A(uicRecBuf_A, sizeof(uicRecBuf_A), sidUic_A, 0x123/*fillvalue*/);
    sdt_validate_pd_A(uicHnd_A, uicRecBuf_A, sizeof(uicRecBuf_A));
    
    sdt_uic_secure_pd_B(uicRecBuf_B, sizeof(uicRecBuf_B), sidUic_B);
    sdt_validate_pd_B(uicHnd_B, uicRecBuf_B, sizeof(uicRecBuf_B));
    sdt_uic_ed5_secure_pd_B(uicRecBuf_B, sizeof(uicRecBuf_B), sidUic_B, 0x123/*fillvalue*/);

    sdt_wtb_secure_pd_A(wtbRecBuf_A, sizeof(wtbRecBuf_A), sidWtb_A, &ssc);
    sdt_validate_pd_A(wtbHnd_A, wtbRecBuf_A, sizeof(wtbRecBuf_A));

    sdt_wtb_secure_pd_B(wtbRecBuf_B, sizeof(wtbRecBuf_B), sidWtb_B, &ssc);
    sdt_validate_pd_B(wtbHnd_B, wtbRecBuf_B, sizeof(wtbRecBuf_B));

    sdt_get_errno_A(iptHnd_A, &errno);
    sdt_get_ssc_A(iptHnd_A, &ssc_l);

    sdt_set_sid_A(iptHnd_A, sidUic_A, sidUic_B, 1);
    sdt_set_sid_B(iptHnd_B, sidUic_B, sidUic_A, 1);

    sdt_set_uic_fillvalue_A(uicHnd_A,0x4711);
    sdt_set_uic_fillvalue_B(uicHnd_B,0x4711);

    sdt_get_uic_fillvalue_A(uicHnd_A,&fill_A);
    sdt_get_uic_fillvalue_B(uicHnd_B,&fill_B);
}




