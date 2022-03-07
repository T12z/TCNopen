#include "ipt_test_functions.h"
#include "test_utils.h"
#include <stdio.h>
#include <stdio.h>

void iptLatencyCrcRun(void)
{
    int i;
    int lmc;
    int start=15;
    int ssc=0;
    int crcOk=1;
    printf("Normal Latency Run with two consecutive CRC errors NOT triggering CMTHR (2nd CRC is DUP)\n");
    for (i=0;i<1020;i++)
    {      
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        if ((i==10)||(i==11))
        {
            printf("Injecting CRC error\n");
            crcOk=0;
        }
        Function_1(ssc,crcOk/*crcOk*/);
        crcOk=1;
    }
}

void iptLatencyCrcRun2(void)
{
    int i;
    int lmc;
    int start=15;
    int ssc=0;
    int crcOk=1;
    printf("Normal Latency Run with two consecutive CRC errors triggering CMTHR, first CRC error occurs in DUP\n");
    for (i=0;i<1300;i++)
    {      
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        if ((i==11)||(i==12))
        {
            printf("Injecting CRC error\n");
            crcOk=0;
        }
        Function_1(ssc,crcOk/*crcOk*/);
        crcOk=1;
    }
}

void iptLatencyCrcRun3(void)
{
    int i;
    int lmc;
    int start=15;
    int ssc=0;
    int crcOk=1;
    printf("Normal Latency Run with two consecutive CRC errors triggering CMTHR\n");
    for (i=0;i<1300;i++)
    {      
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        if ((i==12)||(i==13))
        {
            printf("Injecting CRC error\n");
            crcOk=0;
        }
        Function_1(ssc,crcOk/*crcOk*/);
        crcOk=1;
    }
}

void iptLatencyCrcRun4(void)
{
    int i;
    int lmc;
    int start=15;
    int ssc=0;
    int crcOk=1;
    printf("Normal Latency Run with n consecutive CRC errors (over n_rxsafe with updated SSC) triggering CMTHR\n");
    for (i=0;i<20;i++)
    {      
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        if (i>=10)
        {
            printf("Injecting CRC error\n");
            crcOk=0;
        }
        Function_1(ssc,crcOk/*crcOk*/);
        crcOk=1;
    }
    for (i=20;i<1300;i++)
    {      
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        Function_1(ssc,crcOk/*crcOk*/);
        crcOk=1;
    }

}

void iptLatencyCrcRun5(void)
{
    int i;
    int lmc;
    int start=15;
    int ssc=0;
    int crcOk=1;
    printf("Normal Latency Run with n consecutive CRC errors (over n_rxsafe with stuck SSC) triggering CMTHR - theoretical\n");
    for (i=0;i<10;i++)
    {      
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        Function_1(ssc,crcOk/*crcOk*/);
        crcOk=1;
    }
    for (i=0;i<10;i++)
    {
        Function_1(ssc,0/*crcNok*/);
    }        
    for (i=10;i<1300;i++)
    {      
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        Function_1(ssc,crcOk/*crcOk*/);
        crcOk=1;
    }

}

void iptLatencyCrcRun6(void)
{
    int i;
    int lmc;
    int start=15;
    int ssc=0;
    int crcOk=1;
    printf("Normal Latency Run with n consecutive CRC errors (over n_rxsafe with stuck SSC) triggering CMTHR with gap in ssc\n");
    for (i=0;i<10;i++)
    {      
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        Function_1(ssc,crcOk/*crcOk*/);
        crcOk=1;
    }
    for (i=0;i<10;i++)
    {
        Function_1(ssc,0/*crcNok*/);
    }        
    for (i=27;i<1300;i++)
    {      
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        Function_1(ssc,crcOk/*crcOk*/);
        crcOk=1;
    }

}

void iptLatencyCrcRun7(void)
{
    int i;
    int lmc;
    int start=15;
    int ssc=0;
    int crcOk=1;
    printf("Trouble Latency Run with two consecutive CRC errors triggering CMTHR (2nd CRC is DUP)\n");
    for (i=0;i<38;i++)
    {      
        Function_1(i+start,crcOk/*crcOk*/);
    }
    start=23;  /*start value compensation - now everything should be perfect except for triggered LTM and following CRC*/
    for (i=38;i<1500;i++)
    {      
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        if ((i==38)||(i==39))
        {
            printf("Injecting CRC error\n");
            crcOk=0;
        }
        Function_1(ssc,crcOk/*crcOk*/);
        crcOk=1;
    }

}

void iptLatencyCrcRun8(void)
{
    int i;
    int lmc;
    int start=15;
    int ssc=0;
    int crcOk=1;
    printf("Trouble Latency Run with a set of consecutive CRC errors triggering CMTHR\n");
    for (i=0;i<38;i++)
    {      
        Function_1(i+start,crcOk/*crcOk*/);
    }
    start=23;  /*start value compensation - now everything should be perfect except for triggered LTM and following CRC*/
    for (i=38;i<1500;i++)
    {      
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        if ((i>=38)&&(i<45))
        {
            printf("Injecting CRC error\n");
            crcOk=0;
        }
        Function_1(ssc,crcOk/*crcOk*/);
        crcOk=1;
    }

}

void iptLatencyCrcRun9(void)
{
    int i;
    int lmc;
    int start=15;
    int ssc=0;
    int crcOk=1;
    printf("Simulate SDSRC failing, rebooting validation\n");
    start=23;  /*start value compensation - now everything should be perfect except for triggered LTM and following CRC*/
    for (i=38;i<100;i++)
    {      
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  5;
        ssc=start  + ((i * lmc ) / 10);
        if ((i>70)&&(i<90))
        {
          /*devie boot up, iptcom transmits zero buffer*/
          complexValiLoop(ssc, 100/*Trx*/, 200/*Ttx*/, 1/*zero out VDP*/);
        }
        else
        {
          if ((i>45)&&(i<=70))
          {
            /*ssc is stuck now*/
            complexValiLoop(46, 100/*Trx*/, 200/*Ttx*/, 0/*not zeroed out VDP*/);
          }
          else
          {
            if ((i > 90) && (i<=93))
            {
              printf("### INJECT CRC Trouble again cycle %d\n",i);
              complexValiLoop(ssc, 100/*Trx*/, 200/*Ttx*/, 1/*zero out VDP*/);
            }
            else  
              complexValiLoop(ssc, 100/*Trx*/, 200/*Ttx*/, 0/*not zeroed out VDP*/);
          }                                                                       
        }
    }
}



void iptLatencyRun(void)
{
    int i;
    int start=15;
    int ssc=0;
    for (i=0;i<1000;i++)
    {
        ssc=latency_ssc_generator(start,i,0.8);
        Function_1(ssc,1/*crcOk*/);
    }
}

/*2CRC at the limits of cmthr intervall - latency ok*/
void iptLatencyRun2(void)
{
    int i;
    int lmc;
    int start=15;
    int ssc=0;
    for (i=0;i<2500;i++)
    {              
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        if ((i==5)||(i==1005))
        {
            Function_1(ssc,0/*crcOk*/);
        }
        else
            Function_1(ssc,1/*crcOk*/);
    }
}

/*standard red switch over*/
void iptRedRun1(void)
{
    int i,start=17;
    int ssc=0;
    int lmc;
    for (i=0;i<17;i++)
    {
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=i+start;//start  + ((i * lmc ) / 10);
        redTestFuncIPT(ssc,1,0,1);
    }   
    redTestFuncIPT(ssc,1,0,1);
    start=99;
    ssc=start;
    redTestFuncIPT(ssc,2,0,1);
    redTestFuncIPT(ssc,2,0,1);
    for (i=1;i<15;i++)
    {
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=i+start;//start  + ((i * lmc ) / 10);
        redTestFuncIPT(ssc,2,0,1);
    }
}

/*T_guard violation during red switch over in very initial phase*/
void iptRedRun2(void)
{
    int i,start=17;
    int ssc=0;
    int lmc;
    for (i=0;i<17;i++)
    {
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        redTestFuncIPT(ssc,1,0,1);
    }   
    start=99;
    for (i=0;i<80;i++)
    {
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        redTestFuncIPT(ssc,2,0,1);
        if (i==0)
        {
            redTestFuncIPT(30,1,0,1);/*just a VDP from SRC1, forcing error*/
        }
    }
}

/*CMTHR and sequent red switch over*/
void iptRedRun3(void)
{
    int i,start=17;
    int ssc=0;
    int lmc;
    for (i=0;i<30;i++)
    {
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        if ((i==20)||(i==22)||(i>23))
        {
            /*forces CMTHR*/
            redTestFuncIPT(ssc,1,0,0);
        }
        else
        {
            redTestFuncIPT(ssc,1,0,1);
        }
    } 
    for (i=99;i<120;i++)
    {
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        redTestFuncIPT(ssc,2,0,1);
    } 
}

/*T_guard violation during red switch over after initial phase*/
void iptRedRun4(void)
{
    int i,start=17;
    int ssc=0;
    int lmc;
    for (i=0;i<17;i++)
    {
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        redTestFuncIPT(ssc,1,0,1);
    }   
    start=99;
    for (i=0;i<80;i++)
    {
        /**< Latency Time Monitor weighting factor of received fresh VDP */
        lmc =  8;
        ssc=start  + ((i * lmc ) / 10);
        redTestFuncIPT(ssc,2,0,1);
        if (i==2)
        {
            redTestFuncIPT(30,1,0,1);/*just a VDP from SRC1, forcing error*/
        }
    }
}

void iptCMRun(void)
{
    int i;
    cmTestFuncIPT(1/*ssc*/,1/*crcOk*/);
    cmTestFuncIPT(2/*ssc*/,1/*crcOk*/);
    cmTestFuncIPT(3/*ssc*/,1/*crcOk*/);
    cmTestFuncIPT(4/*ssc*/,0/*crcOk*/);
    cmTestFuncIPT(4/*ssc*/,0/*crcOk*/);
    cmTestFuncIPT(5/*ssc*/,0/*crcOk*/);
    for (i=5;i<1005;i++)
    {
        cmTestFuncIPT(i/*ssc*/,1/*crcOk*/);
    }
}

void iptScen1(void)
{
    int i;
    cmTestFuncIPT(1/*ssc*/,1/*crcOk*/);
    cmTestFuncIPT(2/*ssc*/,1/*crcOk*/);
    cmTestFuncIPT(3/*ssc*/,1/*crcOk*/);
    cmTestFuncIPT(4/*ssc*/,1/*crcOk*/);
    for (i=0;i<10;i++)
    {
        cmTestFuncIPT(5/*ssc*/,1/*crcOk*/);
    } 
    cmTestFuncIPT(6/*ssc*/,1/*crcOk*/);
    cmTestFuncIPT(7/*ssc*/,1/*crcOk*/);
}

