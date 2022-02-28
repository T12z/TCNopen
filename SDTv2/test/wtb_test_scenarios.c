#include "wtb_test_functions.h"
void wtbCMRun(void)
{
    int i;
    cmTestFuncWTB(1/*ssc*/,1/*crcOk*/);
    cmTestFuncWTB(2/*ssc*/,1/*crcOk*/);
    cmTestFuncWTB(3/*ssc*/,1/*crcOk*/);
    cmTestFuncWTB(4/*ssc*/,0/*crcOk*/);
    cmTestFuncWTB(4/*ssc*/,0/*crcOk*/);
    cmTestFuncWTB(5/*ssc*/,0/*crcOk*/);
    for (i=6;i<1008;i++)
    {
        cmTestFuncWTB(i/*ssc*/,1/*crcOk*/);
    }
}

void wtbScen1(void)
{
    int i;
    cmTestFuncWTB(1/*ssc*/,1/*crcOk*/);
    cmTestFuncWTB(2/*ssc*/,1/*crcOk*/);
    cmTestFuncWTB(3/*ssc*/,1/*crcOk*/);
    cmTestFuncWTB(4/*ssc*/,1/*crcOk*/);
    for (i=0;i<10;i++)
    {
        cmTestFuncWTB(5/*ssc*/,1/*crcOk*/);
    } 
    cmTestFuncWTB(6/*ssc*/,1/*crcOk*/);
    cmTestFuncWTB(7/*ssc*/,1/*crcOk*/);
}

