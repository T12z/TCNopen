/**********************************************************************************************************************/
/**
 * @file            trdp_reserved.c
 *
 * @brief           Definition of reserved data sets
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Armin-H. Weiss, Bombardier
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 *      BL 2019-02-01: Ticket #234 Correcting Statistics ComIds
 *      BL 2018-09-05: Ticket #211 XML handling: Dataset Name should be stored in TRDP_DATASET_ELEMENT_T
 *      BL 2017-06-30: Compiler warnings, local prototypes added
 */

#ifndef TRDP_RESERVED_C
#define TRDP_RESERVED_C

/***********************************************************************************************************************
 * INCLUDES
 */

#include "trdp_reserved.h"


/***********************************************************************************************************************
 * LOCALS
 */
TRDP_DATASET_T  dsStatisticsRequest =   /* ComId request used for statistics */
{
    TRDP_STATISTICS_REQUEST_DSID,         /* dataset/com ID  */
    0,         /*  reserved                          */
    1,         /*   No of elements, var size  */
    {          /*  TRDP_DATASET_ELEMENT_T[]  */
        {
            TRDP_UINT32,   /* requested ComId for statistics*/
            1,
            NULL, NULL, 0, 0, NULL
        }
    }
};

TRDP_DATASET_T  dsMemStatistics =   /* mem statistics */
{
    TRDP_MEM_STATISTICS_DSID,         /* dataset/com ID  */
    0,         /*  reserved */
    8,         /*  No of elements, var size  */
    {          /*  TRDP_DATASET_ELEMENT_T[] */
        {
            TRDP_UINT32,   /**< total memory size */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< free memory size */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< minimal free memory size in statistics interval */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< allocated memory blocks */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< allocation errors */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< free errors */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< allocated memory blocks */
            (VOS_MEM_NBLOCKSIZES),
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< used memory blocks */
            (VOS_MEM_NBLOCKSIZES),
            NULL, NULL, 0, 0, NULL
        },

    }
};


TRDP_DATASET_T  dsPdStatistics =   /* pd statistics */
{
    TRDP_PD_STATISTICS_DSID,         /*    dataset/com ID  */
    0,          /* reserved */
    13,         /* No of elements, var size */
    {           /* TRDP_DATASET_ELEMENT_T[] */
        {
            TRDP_UINT32,   /**< default QoS for PD */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< default TTL for PD */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< default timeout in us for PD */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of subscribed ComId's */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of published ComId's */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of received PD packets */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of received PD packets with CRC err */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of received PD packets with protocol err */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of received PD packets with wrong topo count */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of received PD push packets without subscription */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of received PD pull packets without publisher */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of PD timeouts */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of sent PD  packets */
            1,
            NULL, NULL, 0, 0, NULL
        },

    }
};

TRDP_DATASET_T  dsMdStatistics =   /* md statistics */
{
    TRDP_MD_STATISTICS_DSID,         /*    dataset/com ID  */
    0,          /*  reserved  */
    13,         /*  No of elements, var size  */
    {           /*  TRDP_DATASET_ELEMENT_T[]  */
        {
            TRDP_UINT32,   /**< default QoS for MD */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< default TTL for MD */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< default reply timeout in us for PD */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< default confirm timeout in us for PD */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of listeners */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of received MD packets */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of received MD packets with CRC err */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of received MD packets with protocol err */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of received MD packets with wrong topo count */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of received MD packets without listener */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of MD reply timeouts */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of MD confirm timeouts */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of sent MD  packets */
            1,
            NULL, NULL, 0, 0, NULL
        },

    }
};

TRDP_DATASET_T  dsGlobalStatistics =   /* global statistics */
{
    TRDP_GLOBAL_STATISTICS_DSID,         /*    dataset/com ID  */
    0,          /*    reserved        */
    15,         /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT32,   /**< TRDP version  */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_TIMEDATE64,   /**< actual time stamp */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_TIMEDATE32,   /**< time in sec since last initialisation */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_TIMEDATE32,   /**< time in sec since last reset of statistics */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_CHAR8,   /**< host name */
            16,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_CHAR8,   /**< leader host name */
            16,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< own IP address */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< leader IP address */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< priority of TRDP process */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< cycle time of TRDP process in microseconds */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of joins */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< number of redundancy groups */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_MEM_STATISTICS_DSID,   /**< memory statistics */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_PD_STATISTICS_DSID,   /**< pd statistics */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_MD_STATISTICS_DSID,   /**< udp md statistics */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_MD_STATISTICS_DSID,   /**< tcp md statistics */
            1,
            NULL, NULL, 0, 0, NULL
        }

    }
};

TRDP_DATASET_T  dsSubsStatistics =   /* subs statistics */
{
    TRDP_SUBS_STATISTICS_DSID,         /*    dataset/com ID  */
    0,          /*    reserved        */
    8,         /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT32,   /**< Subscribed ComId */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< Joined IP address */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< Filter IP address, i.e IP address of the sender for this subscription, 0.0.0.0 in case
                             all senders. */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< Reference for call back function if used */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< Time-out value in us. 0 = No time-out supervision */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< Receive status information TRDP_NO_ERR, TRDP_TIMEOUT_ERR */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< Behaviour at time-out. Set data to zero / keep last value */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< Number of packets received for this subscription. */
            1,
            NULL, NULL, 0, 0, NULL
        }
    }
};

TRDP_DATASET_T  dsSubsStatisticsArray =      /* pub statistics */
{
    TRDP_SUBS_STATISTICS_ARRAY_DSID,         /*    dataset/com ID  */
    0,          /*    reserved        */
    2,          /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT32,    /**< size  */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_SUBS_STATISTICS_DSID,       /*    dataset/com ID  */
            0,
            NULL, NULL, 0, 0, NULL
        }
    }
};

TRDP_DATASET_T  dsPubStatistics =   /* pub statistics */
{
    TRDP_PUB_STATISTICS_DSID,         /*    dataset/com ID  */
    0,          /*    reserved        */
    7,         /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT32,   /**< Published ComId  */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< IP address of destination for this publishing. */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< Publishing cycle in us */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< Redundancy group id */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< Redundant state.Leader or Follower */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< Number of packet updates */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< Number of packets sent out */
            1,
            NULL, NULL, 0, 0, NULL
        }

    }
};

TRDP_DATASET_T  dsPubStatisticsArray =      /* pub statistics */
{
    TRDP_PUB_STATISTICS_ARRAY_DSID,         /*    dataset/com ID  */
    0,          /*    reserved        */
    2,          /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT32,    /**< size  */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_PUB_STATISTICS_DSID,       /*    dataset/com ID  */
            0,
            NULL, NULL, 0, 0, NULL
        }
    }
};

TRDP_DATASET_T  dsListStatistics =   /* md listener statistics */
{
    TRDP_LIST_STATISTIC_DSID,         /*    dataset/com ID  */
    0,          /*    reserved        */
    7,         /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT32,   /**< ComId to listen to */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< URI user part to listen to */
            32,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< Joined IP address */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< Call back function reference if used */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< Queue reference if used */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< User reference if used */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,   /**< Number of received packets */
            1,
            NULL, NULL, 0, 0, NULL
        }
    }
};


TRDP_DATASET_T  dsListStatisticsArray =     /* md listener statistics */
{
    TRDP_LIST_STATISTIC_ARRAY_DSID,         /*    dataset/com ID  */
    0,          /*    reserved        */
    2,          /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT32,    /**< size  */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_LIST_STATISTIC_DSID,       /*    dataset/com ID  */
            0,
            NULL, NULL, 0, 0, NULL
        }
    }
};

TRDP_DATASET_T  dsRedStatistics =     /* md listener statistics */
{
    TRDP_RED_STATISTICS_DSID,         /*    dataset/com ID  */
    0,          /*    reserved        */
    2,          /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT32,      /**< id  */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,      /*  redundancy state  */
            0,
            NULL, NULL, 0, 0, NULL
        }
    }
};


TRDP_DATASET_T  dsRedStatisticsArray =     /* md listener statistics */
{
    TRDP_RED_STATISTICS_ARRAY_DSID,         /*    dataset/com ID  */
    0,          /*    reserved        */
    2,          /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT32,    /**< size  */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_RED_STATISTICS_DSID,       /*    redundancy statistics  */
            0,
            NULL, NULL, 0, 0, NULL
        }
    }
};

TRDP_DATASET_T  dsJoinStatisticsArray =     /* md join statistics */
{
    TRDP_JOIN_STATISTICS_ARRAY_DSID,         /*    dataset/com ID  */
    0,          /*    reserved        */
    2,          /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT32,    /**< size  */
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,       /*    joined IP addr  */
            0,
            NULL, NULL, 0, 0, NULL
        }
    }
};



/*    Test data sets    */
TRDP_DATASET_T          dsNest1Test =
{
    TRDP_NEST1_TEST_DSID,       /*    dataset/com ID  */
    0,          /*    reserved        */
    0,         /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT32,
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

TRDP_DATASET_T          dsNest2Test =
{
    TRDP_NEST2_TEST_DSID,       /*    dataset/com ID  */
    0,          /*    reserved        */
    2,          /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT8,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_NEST1_TEST_DSID,
            1,
            NULL, NULL, 0, 0, NULL
        }
    }
};

TRDP_DATASET_T          dsNest3Test =
{
    TRDP_NEST3_TEST_DSID,       /*    dataset/com ID  */
    0,          /*    reserved        */
    2,          /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT8,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_NEST2_TEST_DSID,
            1,
            NULL, NULL, 0, 0, NULL
        }
    }
};

TRDP_DATASET_T          dsNest4Test =
{
    TRDP_NEST4_TEST_DSID,       /*    dataset/com ID  */
    0,          /*    reserved        */
    2,          /*    No of elements, var size    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_UINT8,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_NEST3_TEST_DSID,
            1,
            NULL, NULL, 0, 0, NULL
        }
    }
};

TRDP_DATASET_T          dsTest =
{
    TRDP_TEST_DSID,        /*    dataset/com ID  */
    0,          /*    reserved        */
    65,         /*    No of elements    */
    {           /*    TRDP_DATASET_ELEMENT_T[]    */
        {
            TRDP_BOOL8,     /*    data type        */
            1,              /*    no of elements    */
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_CHAR8,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UTF16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_INT8,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_INT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_INT32,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_INT64,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT8,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT64,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_REAL32,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_REAL64,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_TIMEDATE32,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_TIMEDATE48,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_TIMEDATE64,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_BOOL8,     /*    data type        */
            4,              /*    no of elements    */
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_CHAR8,
            16,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UTF16,
            16,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_INT8,
            4,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_INT16,
            4,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_INT32,
            4,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_INT64,
            4,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT8,
            4,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            4,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT32,
            4,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT64,
            4,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_REAL32,
            4,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_REAL64,
            4,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_TIMEDATE32,
            4,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_TIMEDATE48,
            4,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_TIMEDATE64,
            4,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_UINT16,
            1,
            NULL, NULL, 0, 0, NULL
        },
        {
            TRDP_BOOL8,     /*    data type        */
            0,              /*    no of elements    */
            NULL, NULL, 0, 0, NULL
        },
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
            TRDP_NEST4_TEST_DSID,
            1,
            NULL, NULL, 0, 0, NULL
        }
    }
};

TRDP_COMID_DSID_MAP_T   gComIdMap[] =
{
    {TRDP_TEST_DSID, TRDP_TEST_DSID},
    {TRDP_GLOBAL_STATS_REPLY_COMID, TRDP_GLOBAL_STATISTICS_DSID},
    {TRDP_SUBS_STATS_REPLY_COMID, TRDP_SUBS_STATISTICS_DSID},
    {TRDP_PUB_STATS_REPLY_COMID, TRDP_PUB_STATISTICS_DSID},
    {TRDP_RED_STATS_REPLY_COMID, TRDP_RED_STATISTICS_DSID},
    {TRDP_UDP_LIST_STATS_REPLY_COMID, TRDP_LIST_STATISTIC_DSID},
    {TRDP_TCP_LIST_STATS_REPLY_COMID, TRDP_LIST_STATISTIC_DSID}
};

/*    Will be sorted by tau_initMarshall    */
TRDP_DATASET_T          *gDataSets[] =
{
    &dsStatisticsRequest,
    &dsMemStatistics,
    &dsGlobalStatistics,
    &dsSubsStatistics,
    &dsSubsStatisticsArray,
    &dsPubStatistics,
    &dsPubStatisticsArray,
    &dsListStatistics,
    &dsListStatisticsArray,
    &dsRedStatistics,
    &dsRedStatisticsArray,
    &dsJoinStatisticsArray,
    &dsNest1Test,
    &dsNest2Test,
    &dsNest3Test,
    &dsNest4Test,
    &dsTest
};

const UINT32            cNoOfDatasets = sizeof(gDataSets) / sizeof(TRDP_DATASET_T *);

#endif /* TRDP_RESERVED_C */
