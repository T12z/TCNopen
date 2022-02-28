/*****************************************************************************
 *  COPYRIGHT     : This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *                  If a copy of the MPL was not distributed with this file, 
 *                  You can obtain one at http://mozilla.org/MPL/2.0/.
 *                  Copyright Alstom Transport SA or its subsidiaries and others, 2011-2022. All rights reserved. 
 *****************************************************************************
 *
 *  PRODUCT       : MITRAC Safety Layer
 *
 *  MODULE        : sdt_mutex.c
 *
 *  ABSTRACT      : SDT mutex functions
 *
 *  REVISION      : $Id: sdt_mutex.c 29486 2013-10-09 08:07:53Z mkoch $
 *
 ****************************************************************************/

/**
 * We need a different mutex handling for:
 *
 * - WIN32
 *   On Windows we use the functions CreateMutex(), WaitForSingleObject()
 *   and ReleaseMutex().
 *
 * - LINUX
 *   Here we use pthread mutexes, i.e. the functions pthread_mutex_lock()
 *   and pthread_mutex_unlock().
 *
 * - INTEGRITY
 *   No concurrency is used, all functions act as stubs returning SDT_OK
 *
 * - ARM_TI
 *   No concurrency is used, all functions act as stubs returning SDT_OK
 *
 * - VXWORKS (HMI410/411-SP) 
 *   Here we use VxWorks mutual-exclusive semaphores directly via the
 *   functions semMCreate(), semGive() and semTake().
 *
 * - CSS
 *   Here we use CSS' semaphore API with the functions os_sm_create(),
 *   os_s_give() and os_s_take() which are only thin wrappers around
 *   VxWorks semaphores.
 */


/*******************************************************************************
 *  INCLUDES
 */

#include "sdt_api.h"
#include "sdt_mutex.h"

#if defined WIN32

#if defined _MSC_VER
#include <windows.h>
#endif

#elif defined LINUX

#include <pthread.h>

#elif defined OS_INTEGRITY

#elif defined ARM_TI

#elif defined VXWORKS

#include <semLib.h>

#else /* CSS */

#include "rts.h"
#include "os_api.h"
#include "rts_dev.h"

#endif




/*******************************************************************************
 *  DEFINES
 */


/*******************************************************************************
 *  TYPEDEFS
 */
 

/*******************************************************************************
 *  LOCAL FUNCTION DECLARATIONS
 */

static sdt_result_t sdt_mutex_init(void);



/*******************************************************************************
 *  LOCAL VARIABLES
 */

#if defined WIN32

#if defined _MSC_VER
static HANDLE priv_sdt_mutex = NULL;
#endif

#elif defined LINUX

static pthread_mutex_t priv_sdt_mutex;

#elif defined OS_INTEGRITY
/* The INTEGRITY based CCU-S device does not use concurrent access, therefore */
/* no definitions stated here                                                 */

#elif defined ARM_TI
/* The ARM based GW-S safety computer does not use concurrent access, there-  */
/* fore no definitions stated here                                            */

#elif defined VXWORKS

static SEM_ID priv_sdt_mutex = NULL;

#else /* CSS */

static int32_t priv_sdt_mutex = 0;

#endif




/*******************************************************************************
 *  GLOBAL VARIABLES
 */




/*******************************************************************************
 *  LOCAL FUNCTION DEFINITIONS
 */

/**
 * @internal
 * Initializes the priv_sdt_mutex OS specific, if the mutex
 * exists no action is taken. On systems not using concurrency
 * the function shall act as a stub returning SDT_OK
 *
 * @retval SDT_ERR_SYS     system error in OS mutex handling
 * @retval SDT_OK          No error
 */
static sdt_result_t sdt_mutex_init(void)
{
    static int32_t initialized = 0;
    sdt_result_t result = SDT_OK;

    if (initialized == 0)
    {
#if defined WIN32
#if defined _MSC_VER
        priv_sdt_mutex = CreateMutex(NULL, FALSE, NULL);
        if (priv_sdt_mutex == NULL)
        {
            result = SDT_ERR_SYS;
        }
#endif
#elif defined LINUX
        if (pthread_mutex_init(&priv_sdt_mutex, NULL) != 0)
        {
            result = SDT_ERR_SYS;
        }
#elif defined OS_INTEGRITY
        /* Due to performance probs, no use of semas here */

#elif defined ARM_TI
        /* same Handling as in Integrity */

#elif defined VXWORKS
        /*lint -save -e960 MISRA 2004 Rule 12.7
               Bitwise operator applied to signed underlying type is
               prescribed by the VxWorks API. The sign bit is not involved. */
        priv_sdt_mutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
        /*lint -restore */
        if (priv_sdt_mutex == NULL)
        {
            result = SDT_ERR_SYS;
        }
#else /* CSS */
        /*lint -save -e960 MISRA 2004 Rule 12.7
               Bitwise operator applied to signed underlying type is
               prescribed by the CSS API. The sign bit is not involved. */
        int16_t css_result = os_sm_create(OS_SEM_Q_PRIORITY |
                                          OS_SEM_INVERSION_SAFE,
                                          &priv_sdt_mutex);
        /*lint -restore */
        if (result != OK)
        {
            result = SDT_ERR_SYS;
        }
#endif
        initialized = 1;
    }

    return result;
}









/*******************************************************************************
 *  GLOBAL FUNCTION DEFINITIONS
 */

/**
 * Locks the priv_sdt_mutex. On systems not using concurrency
 * the function shall act as a stub returning SDT_OK
 *
 * @retval SDT_ERR_SYS     system error in OS mutex handling
 * @retval SDT_OK          No error
 */
sdt_result_t sdt_mutex_lock(void)
{
    sdt_result_t result = sdt_mutex_init();
    if (result == SDT_OK)
    {
#if defined WIN32
#if defined _MSC_VER
        if (WaitForSingleObject(priv_sdt_mutex, INFINITE) != WAIT_OBJECT_0)
        {
            result = SDT_ERR_SYS;
        }
#endif
#elif defined LINUX
        if (pthread_mutex_lock(&priv_sdt_mutex) != 0)
        {
            result = SDT_ERR_SYS;
        }
#elif defined OS_INTEGRITY
        /* Due to performance probs, no use of semas here */

#elif defined ARM_TI
/* same Handling as in Integrity */

#elif defined VXWORKS
        if (semTake(priv_sdt_mutex, WAIT_FOREVER) != OK)
        {
            result = SDT_ERR_SYS;
        }
#else /* CSS */
        if (os_s_take(priv_sdt_mutex, OS_WAIT_FOREVER) != OK)
        {
            result = SDT_ERR_SYS;
        }
#endif
    }

    return result;
}


/**
 * Unlocks the priv_sdt_mutex. On systems not using concurrency
 * the function shall act as a stub returning SDT_OK
 *
 * @retval SDT_ERR_SYS     system error in OS mutex handling
 * @retval SDT_OK          No error
 */   
sdt_result_t sdt_mutex_unlock(void)
{
    sdt_result_t result = sdt_mutex_init();
    if (result == SDT_OK)
    {
#if defined WIN32
#if defined _MSC_VER
        if (ReleaseMutex(priv_sdt_mutex) == 0)
        {
            result = SDT_ERR_SYS;
        }
#endif
#elif defined LINUX
        if (pthread_mutex_unlock(&priv_sdt_mutex) != 0)
        {
            result = SDT_ERR_SYS;
        }
#elif defined OS_INTEGRITY
        /* Due to performance probs, no use of semas here */

#elif defined ARM_TI
/* same Handling as in Integrity */

#elif defined VXWORKS
        if (semGive(priv_sdt_mutex) != OK)
        {
            result = SDT_ERR_SYS;
        }
#else /* CSS */
        if (os_s_give(priv_sdt_mutex) != OK)
        {
            result = SDT_ERR_SYS;
        }
#endif
    }

    return result;
}

