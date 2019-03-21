/**********************************************************************************************************************/
/**
 * @file            test_marshalling.c
 *
 * @brief           Test application for TRDP marshalling
 *
 * @details         Send PD Pull request for statistics and display them by unmarshalling them
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 *      BL 2018-09-05: Ticket #211 XML handling: Dataset Name should be stored in TRDP_DATASET_ELEMENT_T
 *      BL 2018-04-27: Testing ticket #197
 *      BL 2017-06-30: Compiler warnings, local prototypes added
 */
#include <stdio.h>
#include <string.h>
#include "tau_marshall.h"

/*    Test data sets    */
TRDP_DATASET_T  gDataSet1990 =
{
    1990,       /*    dataset/com ID  */
    0,          /*    reserved        */
    2,          /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT8,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_CHAR8,
            16,
            NULL, NULL, 0, 0, NULL
        }
    }
};

TRDP_DATASET_T  gDataSet1991 =
{
    1991,       /*    dataset/com ID  */
    0,          /*    reserved        */
    2,          /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT8,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            1990,
            1,
            NULL, NULL, 0, 0, NULL
        }
    }
};

TRDP_DATASET_T  gDataSet1992 =
{
    1992,       /*    dataset/com ID  */
    0,          /*    reserved        */
    2,          /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT8,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            1991,
            1,
            NULL, NULL, 0, 0, NULL
        }
    }
};

TRDP_DATASET_T  gDataSet1993 =
{
    1993,       /*    dataset/com ID  */
    0,          /*    reserved        */
    2,          /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT8,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            1992,
            1,
            NULL, NULL, 0, 0, NULL
        }
    }
};

TRDP_DATASET_T  gDataSet1000 =
{
    1000,       /*    dataset/com ID  */
    0,          /*    reserved        */
    65,         /*    No of elements    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_BOOL8,         /*    data type         */
            1,                  /*    no of elements    */
            NULL, NULL, 0, 0, NULL
        },                      /* size = 1 */
        {
            TRDP_CHAR8,
            1,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 2 */
        {
            TRDP_UTF16,
            1,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 4 */
        {
            TRDP_INT8,
            1,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 5 */
        {
            TRDP_INT16,
            1,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 7 */
        {
            TRDP_INT32,
            1,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 11 */
        {
            TRDP_INT64,
            1,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 19 */
        {
            TRDP_UINT8,
            1,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 20 */
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 22 */
        {
            TRDP_UINT32,
            1,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 26 */
        {
            TRDP_UINT64,    /* 10    */
            1,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 34 */
        {
            TRDP_REAL32,
            1,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 38 */
        {
            TRDP_REAL64,
            1,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 46 */
        {
            TRDP_TIMEDATE32,
            1,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 50 */
        {
            TRDP_TIMEDATE48,
            1,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 56 */
        {
            TRDP_TIMEDATE64,        /* 15    */
            1,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 64 */
        {    /* 16    */
            TRDP_BOOL8,         /*    data type        */
            4,                  /*    no of elements    */
            NULL, NULL, 0, 0, NULL
        },                      /* size = 68 */
        {
            TRDP_CHAR8,
            16,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 84 */
        {
            TRDP_UTF16,
            16,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 116 */
        {
            TRDP_INT8,
            4,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 120 */
        {
            TRDP_INT16,    /* 20    */
            4,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 128 */
        {
            TRDP_INT32,
            4,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 144 */
        {
            TRDP_INT64,
            4,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 176 */
        {
            TRDP_UINT8,
            4,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 180 */
        {
            TRDP_UINT16,
            4,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 188 */
        {
            TRDP_UINT32,        /* 25    */
            4,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 204 */
        {
            TRDP_UINT64,
            4,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 236 */
        {
            TRDP_REAL32,
            4,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 252 */
        {
            TRDP_REAL64,
            4,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 284 */
        {
            TRDP_TIMEDATE32,
            4,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 300 */
        {
            TRDP_TIMEDATE48,    /* 30    */
            4,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 324 */
        {
            TRDP_TIMEDATE64,
            4,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 356 */
        {
            TRDP_UINT16,    /* 32    */
            1,
            NULL, NULL, 0, 0, NULL
        },                      /* size = 358 */
        {
            TRDP_BOOL8,   /*    data type        */
            0,               /*    no of elements    */
            NULL, NULL, 0, 0, NULL
        },                      /* size = 362 for current test! */
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_CHAR8,
            0,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UTF16,
            0,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_INT8,
            0,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_INT16,
            0,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_INT32,
            0,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_INT64,
            0,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT8,
            0,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            0,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,
            0,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT64,
            0,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_REAL32,
            0,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_REAL64,
            0,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_TIMEDATE32,
            0,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_TIMEDATE48,
            0,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_TIMEDATE64,
            0,
            NULL, NULL, 0, 0, NULL
        },
        {
            1993,
            1,
            NULL, NULL, 0, 0, NULL
        }
    }
};

TRDP_DATASET_T  gDataSet1001 =
{
    1001,       /*    dataset/com ID  */
    0,          /*    reserved        */
    3,          /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_TIMEDATE64,    /*    Array    */
            4,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,        /*    Size of variable dataset    */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT8,
            0,
            NULL, NULL, 0, 0, NULL
        }
    }
};

TRDP_DATASET_T  gDataSet2002 =
{
    2002,       /*    dataset/com ID  */
    0,          /*    reserved        */
    3,          /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_CHAR8,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_INT32,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_INT32,
            1,
            NULL, NULL, 0, 0, NULL
        }
    }
};

TRDP_DATASET_T  gDataSet2003 =
{
    2003,       /*    dataset/com ID  */
    0,          /*    reserved        */
    3,          /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT32,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_CHAR8, //TRDP_INT32,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            2002,
            1,
            NULL, NULL, 0, 0, NULL
        }
    }
};

/*    Will be sorted by tau_initMarshall    */
TRDP_DATASET_T  *gDataSets[] =
{
    &gDataSet1001,
    &gDataSet1000,
    &gDataSet1990,
    &gDataSet1991,
    &gDataSet1992,
    &gDataSet1993,
    &gDataSet2002,
    &gDataSet2003
};

struct myDataSet1990
{
    UINT8   level;
    char    string[16];
};

struct myDataSet1991
{
    UINT8                   level;
    struct myDataSet1990    ds;
};

struct myDataSet1992
{
    UINT8                   level;
    struct myDataSet1991    ds;
};

struct myDataSet1993
{
    UINT8                   level;
    struct myDataSet1992    ds;
};

struct myDataSet1000
{
    UINT8                   bool8_1;
    char                    char8_1;
    INT16                   utf16_1;
    INT8                    int8_1;
    INT16                   int16_1;
    INT32                   int32_1;
    INT64                   int64_1;
    UINT8                   uint8_1;
    UINT16                  uint16_1;
    UINT32                  uint32_1;
    UINT64                  uint64_1;
    float                   float32_1;
    double                  float64_1;
    TIMEDATE32              timedate32_1;
    TIMEDATE48              timedate48_1;
    TIMEDATE64              timedate64_1;
    UINT8                   bool8_4[4];
    char                    char8_16[16];
    INT16                   utf16_4[16];
    INT8                    int8_4[4];
    INT16                   int16_4[4];
    INT32                   int32_4[4];
    INT64                   int64_4[4];
    UINT8                   uint8_4[4];
    UINT16                  uint16_4[4];
    UINT32                  uint32_4[4];
    UINT64                  uint64_4[4];
    float                   float32_4[4];
    double                  float64_4[4];
    TIMEDATE32              timedate32_4[4];
    TIMEDATE48              timedate48_4[4];
    TIMEDATE64              timedate64_4[4];
    UINT16                  size_bool8;
    UINT8                   bool8_0[4];
    UINT16                  size_char8;
    char                    char8_0[16];
    UINT16                  size_utf16;
    INT16                   utf16_0[16];
    UINT16                  size_int8;
    INT8                    int8_0[4];
    UINT16                  size_int16;
    INT16                   int16_0[4];
    UINT16                  size_int32;
    INT32                   int32_0[4];
    UINT16                  size_int64;
    INT64                   int64_0[4];
    UINT16                  size_uint8;
    UINT8                   uint8_0[4];
    UINT16                  size_uint16;
    UINT16                  uint16_0[4];
    UINT16                  size_uint32;
    UINT32                  uint32_0[4];
    UINT16                  size_uint64;
    UINT64                  uint64_0[4];
    UINT16                  size_float32;
    float                   float32_0[4];
    UINT16                  size_float64;
    double                  float64_0[4];
    UINT16                  size_timedate32;
    TIMEDATE32              timedate32_0[4];
    UINT16                  size_timedate48;
    TIMEDATE48              timedate48_0[4];
    UINT16                  size_timedate64;
    TIMEDATE64              timedate64_0[4];
    struct myDataSet1993    ds;
} gMyDataSet1000 =
{
    1,                               /* BOOL8 */
    'A',
    0x0030,
    0x12,
    0x1234,
    0x12345678,
    0x123456789ABCDEF0,
    0x12,
    0x1234,
    0x12345678,
    0x123456789ABCDEF0,
    0.12345f,
    0.12345678,
    0x12345678,
    {0x12345678, 0x9ABC},            /* 14    */
    {(INT32)0x12345678, (INT32)0x9ABCDEF0},
    {1, 0, 1, 0},                   /* BOOL8 array fixed size */
    "Hello old World",
    {0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0040, 0x0041, 0x0042, 0x0043,
     0x0044, 0},
    {0x12, 0x34, 0x55, 0x78},
    {(INT16)0x1234, (INT16)0x5678, (INT16)0x9ABC, (INT16)0xDEF0},    /* index == 20    */
    {0x12345671, 0x12345672, 0x12345673, 0x12345674},
    {0x123456789ABCDEF1, 0x123456789ABCDEF2, 0x123456789ABCDEF3, 0x123456789ABCDEF4},
    {0x01, 0x23, 0x45, 0x67},
    {0x1234, 0x5678, 0x9ABC, 0xDEF0},
    {0x12345671, 0x12345672, 0x12345673, 0x12345674},
    {0x123456789ABCDEF1, 0x123456789ABCDEF2, 0x123456789ABCDEF3, 0x123456789ABCDEF4},
    {0.12341f, 0.12342f, 0.12343f, 0.12344f},
    {0.12345671, 0.12345672, 0.12345673, 0.12345674},
    {0x12345671, 0x12345672, 0x12345673, 0x12345674},
    {{0x12345671, 0x89A1}, {0x12345672, 0x89A2}, {0x12345673, 0x89A3}, {0x12345674, 0x89A4}},
    {{(INT32)0x12345671, (INT32)0x89ABCDE1}, {(INT32)0x12345672, (INT32)0x89ABCDE2}, {(INT32)0x12345673, (INT32)0x89ABCDE3}, {(INT32)0x12345674, (INT32)0x89ABCDE4}},
    4,                                /* 32    */
    {1, 0, 1, 0},                    /* BOOL8 array var size */
    16,
    "Hello old World",
    16,
    {0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0040, 0x0041, 0x0042, 0x0043,
     0x0044, 0},
    4,
    {0x12, 0x34, 0x55, 0x78},
    4,
    {0x1234, 0x5678, (INT16)0x9ABC, (INT16)0xDEF0},
    4,
    {0x12345671, 0x12345672, 0x12345673, 0x12345674},    /* 43    */
    4,
    {0x123456789ABCDEF1, 0x123456789ABCDEF2, 0x123456789ABCDEF3, 0x123456789ABCDEF4},
    4,
    {0x12, 0x34, 0x55, 0x78},
    4,
    {0x1234, 0x5678, 0x9ABC, 0xDEF0},
    4,
    {0x12345671, 0x12345672, 0x12345673, 0x12345674},
    4,
    {0x123456789ABCDEF1, 0x123456789ABCDEF2, 0x123456789ABCDEF3, 0x123456789ABCDEF4},
    4,
    {0.12341f, 0.12342f, 0.12343f, 0.12344f},
    4,
    {0.12345671, 0.12345672, 0.12345673, 0.12345674},
    4,
    {0x12345671, 0x12345672, 0x12345673, 0x12345674},
    4,
    {{0x12345671, 0x89A1}, {0x12345672, 0x89A2}, {0x12345673, 0x89A3}, {0x12345674, 0x89A4}},
    4,
    {{(INT32)0x12345671, (INT32)0x89ABCDE1}, {(INT32)0x12345672, (INT32)0x89ABCDE2}, {(INT32)0x12345673, (INT32)0x89ABCDE3}, {(INT32)0x12345674, (INT32)0x89ABCDE4}},
    {1, {2, {3, {4, "Nested Datasets"}}}}
};

struct myDataSet1001
{
    TIMEDATE64  timedate64_4[4];
    UINT16      size;
    UINT8       array[4];
} gMyDataSet1001 =
{
    {{(INT32)0x12345671, (INT32)0x89ABCDE1}, {(INT32)0x12345672, (INT32)0x89ABCDE2}, {(INT32)0x12345673, (INT32)0x89ABCDE3}, {(INT32)0x12345674, (INT32)0x89ABCDE4}},
    4,
    {1, 0, 1, 0}                       /* UINT8 array var size */
};

/* Test ticket #197 */
struct DS2 {
    CHAR8 a1;
    INT32 b1;
    INT32 c1;
} gMyDataSet2002  =
{
    'a', 0x12345678, 0x23456789
};

struct myDataSet2003 {
    UINT32 a;
    //INT32 b;
    CHAR8 b;
    struct DS2 c;
} gMyDataSet2003  =
{
    0x12345678, -1, {0,0,0}
};

TRDP_COMID_DSID_MAP_T   gComIdMap[] =
{
    {1000, 1000},
    {1001, 1001},
    {2003, 2003}
};

UINT8   gDstDataBuffer[1500];
UINT32  *gpRefCon = NULL;

struct myDataSet1000    gMyDataSet1000Copy;
struct myDataSet1001    gMyDataSet1001Copy;
struct myDataSet2003    gMyDataSet2003Copy;

/***********************************************************************************************************************
    Test marshalling and unmarshalling of test dataset
***********************************************************************************************************************/
static int test1()
{
    TRDP_ERR_T  err;
    UINT32      compSize    = 0;
    UINT32      bufSize     = 0;
    UINT32      bufSize2;


    /*    Compute size of marshalled data */
    err = tau_calcDatasetSizeByComId(gpRefCon, 1000, (UINT8 *) &gMyDataSet1000, sizeof(gMyDataSet1000), &compSize, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("tau_calcDatasetSizeByComId returns error %d\n", err);
        return 1;
    }

    printf("Precomputed size of marshalled dataset for ComId %d is %u...\n", 1000, compSize);

    if (compSize <= sizeof(gMyDataSet1000))
    {
        printf("...seems OK!\n");
    }
    else
    {
        printf("...### Precomputed size is wrong (> %lu which is sizeof(ds) )!\n", (unsigned long) sizeof(gMyDataSet1000));
    }

    bufSize = compSize;
    memset(gDstDataBuffer, 0, bufSize);

    err = tau_marshall(gpRefCon, 1000, (UINT8 *) &gMyDataSet1000, sizeof(gMyDataSet1000), gDstDataBuffer, &bufSize, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("tau_marshall returns error %d\n", err);
        return 1;
    }

    printf("Marshalled size of dataset for ComId %d is %u\n", 1000, bufSize);

    if (compSize <= sizeof(gMyDataSet1000))
    {
        printf("...seems OK!\n");
    }
    else
    {
        printf("...### Marshalled size is different!\n");
    }

    bufSize2 = sizeof(gMyDataSet1000Copy);
    memset(&gMyDataSet1000Copy, 0, bufSize2);

    err = tau_unmarshall(gpRefCon, 1000, gDstDataBuffer, bufSize, (UINT8 *) &gMyDataSet1000Copy, &bufSize2, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("tau_unmarshall returns error %d\n", err);
        return 1;
    }

    if (memcmp(&gMyDataSet1000, &gMyDataSet1000Copy, sizeof(gMyDataSet1000)) != 0)
    {
        printf("Something's wrong in the state of Marshalling!\n");
        return 1;

    }
    else
    {
        printf("Marshalling and Unmarshalling data matched!\n");
    }

    return 0;
}

static int test2()
{
    TRDP_ERR_T  err;
    UINT32      compSize    = 0;
    UINT32      bufSize     = 0;
    UINT32      bufSize2;

    gMyDataSet2003.c = gMyDataSet2002;

    /*  size of dataset in memory */
    printf("sizeof(gMyDataSet2003): %lu\n", (unsigned long) sizeof(gMyDataSet2003));

    bufSize = sizeof(gDstDataBuffer);
    memset(gDstDataBuffer, 0, sizeof(gDstDataBuffer));

    err = tau_marshall(gpRefCon, 2003, (UINT8 *) &gMyDataSet2003, sizeof(gMyDataSet2003), gDstDataBuffer, &bufSize, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("tau_marshall returns error %d\n", err);
        return 1;
    }

    printf("Marshalled size of dataset for ComId %d is %u\n", 2003, bufSize);

    /*    Compute size of unmarshalled data */
    err = tau_calcDatasetSizeByComId(gpRefCon, 2003, (UINT8 *) &gMyDataSet2003, sizeof(gMyDataSet2003), &compSize, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("tau_calcDatasetSizeByComId returns error %d\n", err);
        return 1;
    }

    printf("Precomputed size of unmarshalled dataset for ComId %d is %u...\n", 2003, (unsigned int) compSize);

    if (compSize == sizeof(gMyDataSet2003))
    {
        printf("...seems OK!\n");
    }
    else
    {
        printf("...### Precomputed size is wrong (%u != %lu which is sizeof(ds) )!\n", (unsigned int)compSize, (unsigned long)sizeof(gMyDataSet2003));
    }

    bufSize2 = sizeof(gMyDataSet2003Copy);
    memset(&gMyDataSet2003Copy, 0, bufSize2);

    err = tau_unmarshall(gpRefCon, 2003, gDstDataBuffer, bufSize, (UINT8 *) &gMyDataSet2003Copy, &bufSize2, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("tau_unmarshall returns error %d\n", err);
        return 1;
    }

    if (bufSize2 != sizeof(gMyDataSet2003))
    {
        printf("...### Computed size is wrong (%u != %lu which is sizeof(ds) )!\n", (unsigned int)compSize, (unsigned long) sizeof(gMyDataSet2003));
        return 1;

    }

    if (memcmp(&gMyDataSet2003, &gMyDataSet2003Copy, sizeof(gMyDataSet2003)) != 0)
    {
        printf("Something's wrong in the state of Marshalling!\n");
        return 1;

    }
    else
    {
        printf("Marshalling and Unmarshalling data matched!\n");
    }

    return 0;
}

/******/
int main ()
{
    TRDP_ERR_T  err;

    err = tau_initMarshall((void *)&gpRefCon, sizeof(gComIdMap)/sizeof(TRDP_COMID_DSID_MAP_T), gComIdMap, 8, gDataSets);

    //test1();
    return test2();
}

