#include <stdio.h>
#define LINUX

#include "../api/sdt_api.h"
#include "mvb_test_scenarios.h"
#include "wtb_test_scenarios.h"
#include "ipt_test_scenarios.h"
#include "uic_test_scenarios.h"
#include "etc_test_scenarios.h"

#include <stdint.h>        
#include <stdlib.h>

#define RESULTS(NAME)	case NAME: return #NAME;
#define DEF_UNKNOWN()	default:   return "UNKNOWN";   


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

                       
void helpme(void)
{
    fprintf(stderr, "\n   Usage:\n"
            "   test  <option>\n"
            "   Valid Options:\n"
            "\n"
            "       -a   IPT Latency Run ok\n"
            "       -b   IPT CM Run\n"
            "       -c   <testcase> MVB CM Run\n"
            "            1: 2 CRC errors within n_rxsafe\n"
            "            2: n CRC errors over n_rxsafe\n"
            "            3: 2 CRC errors within n_rxsafe one DUP\n"
            "            4: n CRC errors over n_rxsafe one DUP\n"
            "            5: 1 CRC error\n"
            "            6: n CRC errors stuck and subsequent gap in ssc\n"
            "            7: 2 CRC errors at the extreme points of the cmthr intevall\n"
            "            8: 2 CRC errors at the 2nd intervall at extreme points\n"
            "            9: CRC stuck over n_rxsafe and sequent CRC new SSC\n"
            "           10: CRC stuck over n_rxsafe and sequent CRC new SSC also stuck over n_rxsafe\n"
            "           11: CRC initially stuck, then stuck 2 then regular init\n"
            "           12: Sequence ok ok CRC1 ok CRC2 CRC2 ok ok\n"
            "       -d   WTB CM Run\n"
            "       -e   UIC CM Run\n"
            "       -f   IPT Init,DUP,Reinit\n"
            "       -g   MVB Init,DUP,Reinit\n"
            "       -h   WTB Init,DUP,Reinit\n"
            "       -i   UIC Init,DUP,Reinit\n"
            "       -j   MVB Init,DUP,short\n"
            "       -k   MVB Init,DUP,long\n"
            "       -l   MVB OOS\n"
            "       -m   MVB Wrong Init, DUP, Init, DUP,short\n"
            "       -o   MVB old VDPs (DUPs),Init, running\n"
            "       -p   <testcase> IPT Latency ok CM Run\n"
            "            1: IPT Latency ok + CRC test 1 (no effect - 2 falling into the DUP)\n"
            "            2: IPT Latency ok + CRC test 2 (effect    - 2 errors consecutively starting within DUP)\n"
            "            3: IPT Latency ok + CRC test 3 (effect    - 2 errors consecutively starting within DUP)\n"
            "            4: IPT Latency ok + CRC test 4 (effect    - n errors consecutively over n_rxsafe)\n"
            "            5: IPT Latency ok + CRC test 5 (effect    - n errors stuck over n_rxsafe)\n"
            "            6: IPT Latency ok + CRC test 6 (effect    - n errors stuck over n_rxsafe with subsequent gap in ssc)\n"
            "            7: IPT Latency ok + CRC test 7 (effect    - latency super triggers and 2 CRC errors all within n_rxsafe)\n"
            "            8: IPT Latency ok + CRC test 8 (effect    - latency super triggers and CRC errors all over n_rxsafe)\n"
            "            9: IPT Latency ok + CRC test 9 (effect    - CRC errors at the extreme points of the cmthr intevall)\n"
            "       -q   <testcase> MVB Reduncancy Run\n"
            "            1: standard switchover\n"
            "            2: switchover with T_guard violation in very initial phase\n"
            "            3: switchover while SRC1 gets into CM trouble\n"
            "            4: switchover with T_guard violation after initial phase\n"
            "       -r   <testcase> IPT Reduncancy Run\n"
            "            1: standard switchover\n"
            "            2: switchover with T_guard violation in very initial phase\n"
            "            3: switchover while SRC1 gets into CM trouble\n"
            "            4: switchover with T_guard violation after initial phase\n"
            "       -s   <testcase> SDSINK init\n"
            "            1: successful getting 10 SDSINKS\n"
            "            2: waiting for error for duplicate sid1\n"
            "            3: waiting for error for duplicate sid2\n"
            "       -t   <testcase> SDSINK inauguration\n"
            "            1: compound run MVB\n"
            "       -u   <testcase> UIC ed.5\n"
            "            1: fast verify\n"
            );
}
               
void initHandler(int testcase)
{
    switch (testcase)
    {
        case 1:
            initRun1();
            break;
        case 2:
            initRun2();
            break;
        case 3:
            initRun3();
            break;
        default:
            helpme();
    }
}

void inaugurationHandler(int testcase)
{
    switch (testcase)
    {
        case 1:
            mvbScen7();
            break;
#if 0
        case 2:
            initRun2();
            break;
        case 3:
            initRun3();
            break;
#endif
        default:
            helpme();
    }
}

void uicEd5Handler(int testcase)
{
    switch (testcase)
    {
        case 1:
            uicEd5Run();
            break;
#if 0
        case 2:
            initRun2();
            break;
        case 3:
            initRun3();
            break;
#endif
        default:
            helpme();
    }
}


void iptCMRunHandler(int testcase)
{
    switch (testcase)
    {
        case 1:
            iptLatencyCrcRun();
            break;
        case 2:
            iptLatencyCrcRun2();
            break;
        case 3:
            iptLatencyCrcRun3();
            break;
        case 4:
            iptLatencyCrcRun4();
            break;
        case 5:
            iptLatencyCrcRun5();
            break;
        case 6:
            iptLatencyCrcRun6();
            break;
        case 7:
            iptLatencyCrcRun7();
            break;
        case 8:
            iptLatencyCrcRun8();
            break;
        case 9:
            iptLatencyRun2();
            break;
        case 10:
            iptLatencyCrcRun9();
            break;
        default:
            helpme();
    }

}




void mvbCMRunHandler(int testcase)
{
    switch (testcase)
    {
        case 1:
            mvbCMRun1();
            break;
        case 2:
            mvbCMRun2();
            break;
        case 3:
            mvbCMRun3();
            break;
        case 4:
            mvbCMRun4();
            break;
        case 5:
            mvbCMRun5();
            break;
        case 6:
            mvbCMRun6();
            break;
        case 7:
            mvbCMRun7();
            break;   
        case 8:
            mvbCMRun8();
            break;   
        case 9:
            mvbCMRun9();
            break;   
        case 10:
            mvbCMRun10();
            break;
        case 11:
            mvbCMRun11();
            break;  
        case 12:
            mvbCMRun12();
            break;
        default:
            helpme();
    }
}

void mvbRedRunHandler(int testcase)
{ 
    switch (testcase)
    {
        case 1:
            mvbRedRun1();
            break;
        case 2:
            mvbRedRun2();
            break;
        case 3:
            mvbRedRun3();
            break;
        case 4:
            mvbRedRun4();
            break;  
        default:
            helpme();
    }
}

void iptRedRunHandler(int testcase)
{ 
    switch (testcase)
    {
        case 1:
            iptRedRun1();
            break;
        case 2:
            iptRedRun2();
            break;
        case 3:
            iptRedRun3();
            break;
        case 4:
            iptRedRun4();
            break;
        default:
            helpme();
    }
}       







int main(int argc, char** argv)
{   
    int             i;

    if (argc==1)
    {
        helpme();
        return 1;
    }
    /*commandline extension*/
    printf("EXCEL, ssc, valid, errno, rx_count, err_count, oos_count, dpl_count\n");
    for (i=1; i<argc; i++)
    {
        if (argv[i][0]=='-')
        {
            switch (argv[i][1])
            {
                case 'a':
                    iptLatencyRun();
                    return 0;
                case 'b':
                    iptCMRun();
                    return 0;
                case 'c':
                    if ((i+1)<argc)
                    { 
                        mvbCMRunHandler(atoi(argv[i+1]));
                        i++;
                    }              
                    /*mvbCMRun();*/
                    return 0;
                case 'd':
                    wtbCMRun();
                    return 0;
                case 'e':
                    uicCMRun();
                    return 0;
                case 'f':
                    iptScen1();
                    return 0;
                case 'g':
                    mvbScen1();
                    return 0;
                case 'h':
                    wtbScen1();
                    return 0;
                case 'i':
                    uicScen1();
                    return 0;
                case 'j':
                    mvbScen2();
                    return 0;
                case 'k':
                    mvbScen3();
                    return 0;
                case 'l':
                    mvbScen4();
                    return 0;
                case 'm':
                    mvbScen5();
                    return 0;
                case 'o':
                    mvbScen6();
                    return 0;
                case 'p':
                    if ((i+1)<argc)
                    { 
                        iptCMRunHandler(atoi(argv[i+1]));
                        i++;
                    }              
                    return 0;
                case 'q':
                    if ((i+1)<argc)
                    { 
                        mvbRedRunHandler(atoi(argv[i+1]));
                        i++;
                    }              
                    return 0;
                case 'r':
                    if ((i+1)<argc)
                    { 
                        iptRedRunHandler(atoi(argv[i+1]));
                        i++;
                    }              
                    return 0;
                case 's':
                    if ((i+1)<argc)
                    { 
                        initHandler(atoi(argv[i+1]));
                        i++;
                    }              
                    return 0;
                case 't':
                    if ((i+1)<argc)
                    { 
                        inaugurationHandler(atoi(argv[i+1]));
                        i++;
                    }              
                    return 0;
                case 'u':
                    if ((i+1)<argc)
                    { 
                        uicEd5Handler(atoi(argv[i+1]));
                        i++;
                    }              
                    return 0; 
                default: 
                    helpme();
                    return 1; 
            }
        }
        else
            helpme();
    }
    return 0;
}






