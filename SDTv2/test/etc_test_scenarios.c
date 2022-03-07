#define LINUX
#include "../api/sdt_api.h"
#include <stdio.h>
#include <stdint.h>
#define SDT_SID 0x12345678U

void initRun1(void)
{
    static sdt_handle_t hnd[10];
    sdt_result_t        result;
    int n;

    for (n = 1; n < 11; n++)
    {
        result = sdt_get_validator(SDT_IPT,  n, 0, 0, 2, &hnd[n-1]);
        if (result != SDT_OK)
        {
            printf("ERROR: got %d from sdt_get_validator, iteration %d\n",result, n);
        }
    }
    printf("Finished initRun1 - no Error shall be displayed\n");
}

void initRun2(void)
{
    static sdt_handle_t hnd[10];
    sdt_result_t        result;
    int n;

    for (n = 1; n < 10; n++)
    {
        result = sdt_get_validator(SDT_IPT,  n, 0, 0, 2, &hnd[n-1]);
        if (result != SDT_OK)
        {
            printf("ERROR: got %d from sdt_get_validator, iteration %d\n",result, n);
        }
    }
    result = sdt_get_validator(SDT_IPT,  9, 0, 0, 2, &hnd[9]);
    if (result != SDT_OK)
    {
        printf("got %d from sdt_get_validator - Duplicate SID detected successfully\n",result);
    }
    printf("Finished initRun2 - no Error shall be displayed\n");
}

void initRun3(void)
{
    static sdt_handle_t hnd[10];
    sdt_result_t        result;
    int n;

    for (n = 1; n < 9; n++)
    {
        result = sdt_get_validator(SDT_IPT,  n, 0, 0, 2, &hnd[n-1]);
        if (result != SDT_OK)
        {
            printf("ERROR: got %d from sdt_get_validator, iteration %d\n",result, n);
        }
    }
    result = sdt_get_validator(SDT_IPT,  9, 17, 1, 2, &hnd[8]);
    if (result != SDT_OK)
    {
        printf("ERROR: got %d from sdt_get_validator - red 1 \n",result);
    }
    result = sdt_get_validator(SDT_IPT,  10, 17, 1, 2, &hnd[9]);
    if (result != SDT_OK)
    {
        printf("got %d from sdt_get_validator - Duplicate redundant SID detected successfully\n",result);
    } 
    else
    {
        printf("ERROR: got %d from sdt_get_validator - error was expected! \n",result);
    }

    printf("Finished initRun3 - no Error shall be displayed\n");
}

