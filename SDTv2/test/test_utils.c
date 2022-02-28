#include <math.h>
#include <stdio.h>


void invalidate_crc(unsigned char* msg, unsigned short len)
{
    msg[len-1] = msg[len-1] ^ 0xFF;
}

int latency_ssc_generator(int start, int cycle, float ratio)
{
    /*calculator for simulated SSC slower than receiver*/
    float ssc=(float)0;
    float intpart=0;
    ssc = (float)start + ((float)cycle * ratio);
    if( modff(ssc, &intpart) >= 0.5)
    {
        /*round up*/
        printf("ssc: %f intpart: %f\n", ssc,intpart);
        return (int)(intpart)/*+1*/;
    }
    else
    {
        /*no rounding*/
        printf("ssc: %f intpart: %f\n", ssc,intpart);
        return (int)(intpart);
    }
}

