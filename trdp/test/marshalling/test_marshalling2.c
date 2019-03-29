/**********************************************************************************************************************/
/**
 * @file            test_marshalling2.c
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
 */

#include <stdio.h>
#include <string.h>
#include "tau_marshall.h"

#define TEST_COMID  2002

/**********************************************************************************************************************/
/* Declaration of DATASET1 */
/**********************************************************************************************************************/
typedef struct
{
    BOOL8       boolean;
    CHAR8       character;
    UTF16       utf16;
    INT8        integer8;
    INT16       integer16;
    INT32       integer32;
    INT64       integer64;
    UINT8       uInteger8;
    UINT16      uInteger16;
    UINT32      uInteger32;
    UINT64      uInteger64;
    REAL32      real32;
    REAL64      real64;
    TIMEDATE32  timeDate32;
    TIMEDATE48  timeDate48;
    TIMEDATE64  timeDate64;
} DATASET1_T;

#define DATASET1_PACKED_SIZE  (sizeof(BOOL8) +           \
                               sizeof(CHAR8) +          \
                               sizeof(UTF16) +          \
                               sizeof(INT8) +           \
                               sizeof(INT16) +          \
                               sizeof(INT32) +          \
                               sizeof(INT64) +          \
                               sizeof(UINT8) +          \
                               sizeof(UINT16) +         \
                               sizeof(UINT32) +         \
                               sizeof(UINT64) +         \
                               sizeof(REAL32) +         \
                               sizeof(REAL64) +         \
                               sizeof(TIMEDATE32) +     \
                               sizeof(TIMEDATE48) - 2 + \
                               sizeof(TIMEDATE64))

/* Definition of DATASET1 */
TRDP_DATASET_T gDataSet2001 =
{
    2001,       /*    dataset/com ID  */
    0,          /*    reserved        */
    16,         /*    No of TRDP_DATASET_ELEMENT_T    */
    { /* data type,  no of elements, reserved pointer */
        { TRDP_BOOL8, 1, NULL, NULL, 0, 0, NULL},
        { TRDP_CHAR8, 1, NULL, NULL, 0, 0, NULL},
        { TRDP_UTF16, 1, NULL, NULL, 0, 0, NULL },
        { TRDP_INT8, 1, NULL, NULL, 0, 0, NULL },
        { TRDP_INT16, 1, NULL, NULL, 0, 0, NULL },
        { TRDP_INT32, 1, NULL, NULL, 0, 0, NULL },
        { TRDP_INT64, 1, NULL, NULL, 0, 0, NULL },
        { TRDP_UINT8, 1, NULL, NULL, 0, 0, NULL },
        { TRDP_UINT16, 1, NULL, NULL, 0, 0, NULL },
        { TRDP_UINT32, 1, NULL, NULL, 0, 0, NULL },
        { TRDP_UINT64, 1, NULL, NULL, 0, 0, NULL },
        { TRDP_REAL32, 1, NULL, NULL, 0, 0, NULL },
        { TRDP_REAL64, 1, NULL, NULL, 0, 0, NULL },
        { TRDP_TIMEDATE32, 1, NULL, NULL, 0, 0, NULL },
        { TRDP_TIMEDATE48, 1, NULL, NULL, 0, 0, NULL },
        { TRDP_TIMEDATE64, 1, NULL, NULL, 0, 0, NULL }
    }
};

/**********************************************************************************************************************/
/* Declaration of DATASET2 */
/**********************************************************************************************************************/
typedef struct
{
    DATASET1_T  dataset1[2];
    INT16       int16[64];
} DATASET2_T;

#define DATASET2_PACKED_SIZE  (2 * DATASET1_PACKED_SIZE + \
                               64 * sizeof(INT16))

/* Definition of DATASET2 */
TRDP_DATASET_T  gDataSet2002 =
{
    2002,       /*    dataset/com ID  */
    0,          /*    reserved        */
    2,         /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            2001,
            2,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_INT16,
            64,
            NULL, NULL, 0, 0, NULL
        }
    }
};

/*    Will be sorted by tau_initMarshall    */
TRDP_DATASET_T  *gDataSets[] =
{
    &gDataSet2001,
    &gDataSet2002
};


/**********************************************************************************************************************/
/*
     Demo/Test data
 */
/**********************************************************************************************************************/

DATASET2_T gMyDataSet2 =
{
    {{ /* Dataset1[0] */
         1,
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
         0.12345,
         0.12345678,
         0x12345678,
         {0x12345678, 0x9ABC},           /* 14    */
         {0x12345678, 0x9ABCDEF0},
     },
     { /* Dataset1[1] */
         1,
         'B',
         0x0030,
         0x12,
         0x1234,
         0x12345678,
         0x123456789ABCDEF0,
         0x12,
         0x1234,
         0x12345678,
         0x123456789ABCDEF0,
         0.12345,
         0.12345678,
         0x12345678,
         {0x12345678, 0x9ABC},           /* 14    */
         {0x12345678, 0x9ABCDEF0},
     }},
    { /* INT16[64] */
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        0, 1, 2, -1
    }
};


TRDP_COMID_DSID_MAP_T gComIdMap[] =
{
    {2000, 2000},
    {2001, 2001},
    {2002, 2002}
};

UINT8       gDstDataBuffer[1500];

DATASET2_T  gMyDataSet2Copy;

/***********************************************************************************************************************
    Test marshalling and unmarshalling of test dataset
***********************************************************************************************************************/
int main ()
{
    /*INT32       index;*/
    UINT32      *refCon;
    TRDP_ERR_T  err;
    UINT32      bufSize = 0;

    err = tau_initMarshall((void *)&refCon, sizeof(gComIdMap) / sizeof(TRDP_COMID_DSID_MAP_T), gComIdMap,
                           sizeof(gDataSets) / sizeof(TRDP_DATASET_T *), gDataSets);

    if (err != TRDP_NO_ERR)
    {
        printf("tau_initMarshall returns error %d\n", err);
        return 1;
    }

    /*    Compute size of marshalled data */
    err = tau_calcDatasetSizeByComId(refCon, TEST_COMID, (UINT8 *) &gMyDataSet2, sizeof(gMyDataSet2), &bufSize, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("tau_calcDatasetSizeByComId returns error %d\n", err);
        return 1;
    }

    printf("Precomputed size of marshalled dataset for ComId %d is %u...\n", TEST_COMID, bufSize);

    if (bufSize == DATASET2_PACKED_SIZE)
    {
        printf("...seems OK!\n");
    }
    else
    {
        printf("...### Precomputed size is different (expected %lu)!\n", DATASET2_PACKED_SIZE);

        printf("DATASET1_PACKED_SIZE = %lu\n", DATASET1_PACKED_SIZE);
        printf("BOOL8 = %lu\n", sizeof(BOOL8));
        printf("TIMEDATE32 = %lu\n", sizeof(TIMEDATE32));
        printf("TIMEDATE48 = %lu\n", sizeof(TIMEDATE48));
        printf("TIMEDATE64 = %lu\n", sizeof(TIMEDATE64));
        printf("64 * sizeof(INT16) = %lu\n", 64 * sizeof(INT16));
    }

    bufSize = sizeof(gDstDataBuffer);
    memset(gDstDataBuffer, 0, bufSize);

    err = tau_marshall(refCon, TEST_COMID, (UINT8 *) &gMyDataSet2, sizeof(gMyDataSet2), gDstDataBuffer, &bufSize, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("### tau_marshall returns error %d\n", err);
        return 1;
    }

    printf("Marshalled size of dataset for ComId %d is %u\n", TEST_COMID, bufSize);

    if (bufSize == DATASET2_PACKED_SIZE)
    {
        printf("...seems OK!\n");
    }
    else
    {
        printf("...### Marshalled size is different!\n");
    }

    UINT32 bufSize2 = sizeof(gMyDataSet2Copy);
    memset(&gMyDataSet2Copy, 0, bufSize2);

    err = tau_unmarshall(refCon, TEST_COMID, gDstDataBuffer, bufSize, (UINT8 *) &gMyDataSet2Copy, &bufSize2, NULL);

    if (err != TRDP_NO_ERR)
    {
        printf("### tau_unmarshall returns error %d\n", err);
        return 1;
    }

    if (memcmp(&gMyDataSet2, &gMyDataSet2Copy, sizeof(gMyDataSet2)) != 0)
    {
        printf("### Something's wrong in the state of Marshalling!\n");
        return 1;
    }
    else
    {
        printf("Marshalling and Unmarshalling data matched!\n");
    }

    return 0;
}
