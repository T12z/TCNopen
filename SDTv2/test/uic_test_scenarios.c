#include "uic_test_functions.h"
void uicCMRun(void)
{
    int i;
    cmTestFuncUIC(1/*ssc*/,1/*crcOk*/);
    cmTestFuncUIC(2/*ssc*/,1/*crcOk*/);
    cmTestFuncUIC(3/*ssc*/,1/*crcOk*/);
    cmTestFuncUIC(4/*ssc*/,0/*crcOk*/);
    cmTestFuncUIC(4/*ssc*/,0/*crcOk*/);
    cmTestFuncUIC(5/*ssc*/,0/*crcOk*/);
    for (i=6;i<1010;i++)
    {
        cmTestFuncUIC(i/*ssc*/,1/*crcOk*/);
    }
}




void uicScen1(void)
{
    int i;
    cmTestFuncUIC(1/*ssc*/,1/*crcOk*/);
    cmTestFuncUIC(2/*ssc*/,1/*crcOk*/);
    cmTestFuncUIC(3/*ssc*/,1/*crcOk*/);
    cmTestFuncUIC(4/*ssc*/,1/*crcOk*/);
    for (i=0;i<10;i++)
    {
        cmTestFuncUIC(5/*ssc*/,1/*crcOk*/);
    } 
    cmTestFuncUIC(6/*ssc*/,1/*crcOk*/);
    cmTestFuncUIC(7/*ssc*/,1/*crcOk*/);
}

void uicEd5Run(void)
{
    ed5TestFuncUIC();
}


