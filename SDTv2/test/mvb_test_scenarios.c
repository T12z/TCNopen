#include "mvb_test_functions.h"

/*init,duplicate,reinit*/
void mvbScen1(void)
{
    int i;
    cmTestFuncMVB(1/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(2/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(3/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(4/*ssc*/,1/*crcOk*/,0);
    for (i=0;i<10;i++)
    {
        cmTestFuncMVB(5/*ssc*/,1/*crcOk*/,0);
    } 
    cmTestFuncMVB(6/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(7/*ssc*/,1/*crcOk*/,0);
}


void mvbScen2(void) /*initialisation with few DUPs*/
{
    int i;
    cmTestFuncMVB(1/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(1/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(1/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(2/*ssc*/,1/*crcOk*/,0);
    for (i=3;i<10;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    } 
}

void mvbScen3(void) /*initialisation with multi DUPs*/
{
    int i;
    for (i=0;i<10;i++)
    {
        cmTestFuncMVB(1/*ssc*/,1/*crcOk*/,0);
    } 
    for (i=2;i<10;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    } 
}

void mvbScen4(void) /*OOS*/
{
    int i;
    for (i=1;i<10;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    }     
    cmTestFuncMVB(10/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(6/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(11/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(12/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(13/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(14/*ssc*/,1/*crcOk*/,0);

}


/*crc-errors,init,duplicate,reinit*/
void mvbScen5(void)
{
    int i;
    cmTestFuncMVB(1/*ssc*/,0/*crcOk*/,0);
    cmTestFuncMVB(2/*ssc*/,0/*crcOk*/,0);
    cmTestFuncMVB(3/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(3/*ssc*/,1/*crcOk*/,0);
    for (i=4;i<10;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    } 
}

/*old VDPs (DUPs),init,running*/
void mvbScen6(void)
{
    int i;
    cmTestFuncMVB(7/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(7/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(7/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(7/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(7/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(7/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(7/*ssc*/,1/*crcOk*/,0);
    for (i=103;i<110;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    } 
}

/*clean run with inaufuration*/
void mvbScen7(void)
{
    printf("turn on phase - stuck PD\n");
    cmTestFuncMVB(7/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(7/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(7/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(7/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(387/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(388/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(389/*ssc*/,1/*crcOk*/,0);
    printf("Inaugurate after next validation\n");
    cmTestFuncMVB(390/*ssc*/,1/*crcOk*/,1);
    printf("Inaugurated - ssc flow with no gap\n");
    cmTestFuncMVB(391/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(392/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(393/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(394/*ssc*/,1/*crcOk*/,0);
    printf("Inaugurate after next validation\n");
    cmTestFuncMVB(395/*ssc*/,1/*crcOk*/,1);
    printf("Inaugurated - ssc flow with gap\n");
    cmTestFuncMVB(410/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(411/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(412/*ssc*/,1/*crcOk*/,0);
    printf("now falling into channel monitoring\n");
    cmTestFuncMVB(413/*ssc*/,0/*crcOk*/,0);
    cmTestFuncMVB(414/*ssc*/,0/*crcOk*/,0);
    printf("Inaugurate after next validation\n");
    cmTestFuncMVB(415/*ssc*/,1/*crcOk*/,1);
    printf("Inaugurated - ssc flow with no gap\n");
    cmTestFuncMVB(416/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(417/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(418/*ssc*/,1/*crcOk*/,0);
    printf("Inaugurate after next validation\n");
    cmTestFuncMVB(419/*ssc*/,1/*crcOk*/,1);
    printf("Inaugurated - ssc flow with sink time failure during initial\n");
    cmTestFuncMVB(419/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(419/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(419/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(419/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(419/*ssc*/,1/*crcOk*/,0);
    printf("resuming ssc flow\n");
    cmTestFuncMVB(425/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(426/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(427/*ssc*/,1/*crcOk*/,0);


}

void mvbCMRun(void)
{
    int i;
    cmTestFuncMVB(1/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(2/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(3/*ssc*/,1/*crcOk*/,0);
    for (i=4;i<10;i++)
    {
        cmTestFuncMVB(i/*ssc*/,0/*crcOk*/,0);
    }
#if 0
    cmTestFuncMVB(4/*ssc*/,0/*crcOk*/,0);
    cmTestFuncMVB(4/*ssc*/,0/*crcOk*/,0);
    cmTestFuncMVB(5/*ssc*/,0/*crcOk*/,0);
#endif
    for (i=10;i<10200;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    } 
}


/*2CRC within n_rxsafe*/
void mvbCMRun1(void)
{
    int i;
    cmTestFuncMVB(1/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(2/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(3/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(4/*ssc*/,0/*crcNOk*/,0);
    cmTestFuncMVB(5/*ssc*/,0/*crcNOk*/,0);
    cmTestFuncMVB(6/*ssc*/,0/*crcOk*/,0);

    for (i=7;i<10200;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    } 
}
/*multi CRC reaching over n_rxsafe*/
void mvbCMRun2(void)
{
    int i;
    cmTestFuncMVB(1/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(2/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(3/*ssc*/,1/*crcOk*/,0);
    for (i=4;i<10;i++)
    {
        cmTestFuncMVB(i/*ssc*/,0/*crcNOk*/,0);
    }
    for (i=10;i<10200;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    } 
} 
/*2CRC within n_rxsafe with DUP*/
void mvbCMRun3(void)
{
    int i;
    cmTestFuncMVB(1/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(2/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(3/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(4/*ssc*/,0/*crcNOk*/,0);
    cmTestFuncMVB(4/*ssc*/,0/*crcNOk*/,0);
    cmTestFuncMVB(5/*ssc*/,0/*crcOk*/,0);

    for (i=6;i<10200;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    } 
}  
/*multi CRC reaching over n_rxsafe with DUP*/
void mvbCMRun4(void)
{
    int i;
    cmTestFuncMVB(1/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(2/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(3/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(4/*ssc*/,0/*crcOk*/,0);
    cmTestFuncMVB(4/*ssc*/,0/*crcOk*/,0);
    for (i=5;i<10;i++)
    {
        cmTestFuncMVB(i/*ssc*/,0/*crcNOk*/,0);
    }
    for (i=10;i<10200;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    } 
} 
/*1CRC within n_rxsafe*/
void mvbCMRun5(void)
{
    int i;
    cmTestFuncMVB(1/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(2/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(3/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(4/*ssc*/,0/*crcNOk*/,0);
    for (i=5;i<10200;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    } 
} 
/*full stuck CRC*/
void mvbCMRun6(void)
{
    int i;
    cmTestFuncMVB(1/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(2/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(3/*ssc*/,1/*crcOk*/,0);
    for (i=5;i<20;i++)
    {
        cmTestFuncMVB(4/*ssc*/,0/*crcNOk*/,0);
    }                          
    for (i=21;i<10300;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    } 
} 
/*2CRC at the limits of cmthr intervall*/
void mvbCMRun7(void)
{
    int i;
    cmTestFuncMVB(1/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(2/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(3/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(4/*ssc*/,0/*crcNOk*/,0);
    for (i=5;i<10000;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    } 
    cmTestFuncMVB(10000/*ssc*/,0/*crcNOk*/,0);
    for (i=10001;i<20010;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    }   
}

/*2CRC at the limits of 2nd cmthr intervall*/
void mvbCMRun8(void)
{
    int i;
    for (i=1;i<10001;i++)
     {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    } 
    cmTestFuncMVB(10001/*ssc*/,0/*crcNOk*/,0);
    for (i=10002;i<20000;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    }   
    cmTestFuncMVB(20000/*ssc*/,0/*crcNOk*/,0);
    for (i=20000;i<30010;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    }   

}

/*CRC stuck over n_rxsafe and sequent CRC new SSC*/
void mvbCMRun9(void)
{
    int i;
    for (i=1;i<5;i++)
     {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    } 
    for (i=0;i<4;i++)
    {
        cmTestFuncMVB(5/*ssc*/,0/*crcOk*/,0);
    }   
    cmTestFuncMVB(6/*ssc*/,0/*crcOk*/,0);
    for (i=7;i<100;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    }   

}

/*CRC stuck over n_rxsafe and sequent CRC new SSC also stuck over n_rxsafe*/
void mvbCMRun10(void)
{
    int i;
    for (i=1;i<5;i++)
     {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    } 
    for (i=0;i<4;i++)
    {
        cmTestFuncMVB(5/*ssc*/,0/*crcOk*/,0);
    }
    for (i=0;i<4;i++)
    {
        cmTestFuncMVB(6/*ssc*/,0/*crcOk*/,0);
    }   
    for (i=7;i<100;i++)
     {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    } 
}

/*CRC initially stuck, then stuck 2 then regular init*/
void mvbCMRun11(void)
{
    int i;
    for (i=1;i<10;i++)
     {
        cmTestFuncMVB(77/*ssc*/,0/*crcOk*/,0);
    } 
    for (i=1;i<7;i++)
    {
        cmTestFuncMVB(55/*ssc*/,0/*crcOk*/,0);
    }
    for (i=99;i<120;i++)
    {
        cmTestFuncMVB(i/*ssc*/,1/*crcOk*/,0);
    }   
}

/*MVB sequence ok ok crc1 ok crc2 crc2 ok ok*/
void mvbCMRun12(void)
{
    cmTestFuncMVB(88/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(89/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(90/*ssc*/,0/*crcOk*/,0);
    cmTestFuncMVB(91/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(92/*ssc*/,0/*crcOk*/,0);
    cmTestFuncMVB(92/*ssc*/,0/*crcOk*/,0);
    cmTestFuncMVB(94/*ssc*/,1/*crcOk*/,0);
    cmTestFuncMVB(95/*ssc*/,1/*crcOk*/,0);
}

/*standard red switch over*/
void mvbRedRun1(void)
{
    int i;
    for (i=17;i<30;i++)
        redTestFuncMVB(i,1,0,1);
    redTestFuncMVB(i,1,1,1);
    for (i=10;i<30;i++)
        redTestFuncMVB(i,2,0,1);
}

/*T_guard violation during red switch over in very initial phase*/
void mvbRedRun2(void)
{
    int i;
    for (i=17;i<30;i++)
        redTestFuncMVB(i,1,0,1);
    redTestFuncMVB(10,2,0,1);
    redTestFuncMVB(30,1,0,1);
    for (i=11;i<50;i++)
        redTestFuncMVB(i,2,0,1);
}

/*CMTHR and sequent red switch over*/
void mvbRedRun3(void)
{
    int i;
    for (i=17;i<30;i++)
        redTestFuncMVB(i,1,0,1);
    redTestFuncMVB(30,1,0,0);/*crc error*/
    redTestFuncMVB(31,1,0,1);
    redTestFuncMVB(32,1,0,0);/*crc error*/
    for (i=33;i<40;i++)
        redTestFuncMVB(i,1,0,0);/*crc error*/
    for (i=11;i<20;i++)
        redTestFuncMVB(i,2,0,1);
}

/*T_guard violation during red switch over after initial phase*/
void mvbRedRun4(void)
{
    int i;
    for (i=17;i<30;i++)
        redTestFuncMVB(i,1,0,1);
    redTestFuncMVB(10,2,0,1);
    redTestFuncMVB(11,2,0,1);
    redTestFuncMVB(30,1,0,1);
    for (i=12;i<100;i++)
        redTestFuncMVB(i,2,0,1);
}


