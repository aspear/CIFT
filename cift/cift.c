/*******************************************************************************
* CIFT : Compiler Instrumentation Function Tracer
*
* Copyright (c) 2011 Aaron Spear  aaron_spear@ontherock.com
*
* This is free software released under the terms of the MIT license:
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
********************************************************************************
* Please see notes in cift.h
********************************************************************************
*/

#include "cift.h"

#include <stddef.h> //for size_t
#include <malloc.h> //for malloc (duh)

#if (CIFT_HAVE_FILEIO)
#include <stdio.h>
#include <stdlib.h>
#endif

#if defined(__GNUC__)
    //#include <stdatomic.h> //GCC atomic builtins/intrinsics
    //macro to wrap access to GCC specific intrinsic function.  maybe we can use this for portability, though
    //I am unsure about cross platform availability.  See usage for requirements on the functionality
    #define ATOMIC_COMPARE_AND_SWAP( VAR_ADDR, OLD_VALUE, NEW_VALUE ) (__sync_bool_compare_and_swap( (VAR_ADDR), (OLD_VALUE), (NEW_VALUE) ))
    #define ATOMIC_FETCH_AND_ADD( VAR_ADDR, INCREMENT ) __sync_fetch_and_add( (VAR_ADDR),(INCREMENT))
    #define ATOMIC_INCREMENT( VAR_ADDR ) __sync_fetch_and_add( (VAR_ADDR),1)

#else
    #error sorry, this compiler is not currently supported.  You should add the needed macros!
#endif

/**
 * for high precision timestamps we expose cift_get_timestamp.  see cycle.h file for more information
 */
#include "cycle.h"

/** include the file that defines the cift_get_current_thread function which gets the
 * current thread id/context for the event log
 */
#include "cift_os.h"

#ifdef __cplusplus
extern "C" {
#endif

/* prototypes for the functions that are called internally by the
* compiler when functions are compiled with -finstrument-functions.
* any other code compiled with this switch will result on a dependency
* on these functions
*/
void __cyg_profile_func_enter (void *this_fn, void *call_site)
    __attribute__ ((no_instrument_function));
void __cyg_profile_func_exit (void *this_fn, void *call_site)
    __attribute__ ((no_instrument_function));

static void cift_default_init(void)
    __attribute__ ((no_instrument_function));

#if ((CIFT_HAVE_FILEIO)&&(CIFT_ATEXIT_DUMP_LOG))
    void cift_file_dump_atexit_handler(void)
        __attribute__ ((no_instrument_function));
#endif

/*****************************************************************************/

//single global variable for this instance
CIFT_EVENT_BUFFER* cift_event_buffer = 0;

//variable that controls whether or not tracing is enabled
static volatile CIFT_BOOL cift_enabled = CIFT_DEFAULT_ENABLE;

/*****************************************************************************/

/** utility function that uses a lockless mechanism to get the next event to populate.  note that this mechanism
 *  works well on a single core machine, but can result in an impact on determinism in a very busy multi-core
 *  machine. */

static inline CIFT_EVENT* cift_get_next_event(void)
    __attribute__ ((no_instrument_function));

static inline CIFT_EVENT* cift_get_next_event(void)
{
    CIFT_EVENT* pEvent = 0;
    CIFT_COUNT next_index;
    CIFT_COUNT new_next_index;

    //loop until we are mutually exclusive on the swap of the index
    do
    {
        //copy current index value from volatile variable
        next_index = cift_event_buffer->next_event_index;

        //TODO see if I can collapse this logic to only have one conditional check

        if (next_index >= cift_event_buffer->max_event_count)
        {
            //if this is ever the case, it means we are in a non-circular mode
            //and are out of events.  return NULL
            return 0;
        }

        //increment the index to the next value, check if we go over the end of the buffer
        if ((new_next_index = (next_index + 1)) >= cift_event_buffer->max_event_count)
        {
            //at the end of the buffer, so we either wrap or are out of space
            if (cift_event_buffer->buffer_mode == CIFT_BUFFER_MODE_CIRCULAR)
                new_next_index = 0; //wrap the index
        }

        //the event we will return will be the one at the next_index value
        pEvent = &(cift_event_buffer->event_buffer[next_index]);

        //for the condition check we use GCC intrinsic that returns true if the value in memory is
        //still the same as the old value.  in the case that it is, it sets the value to the new value and
        //returns true.  So, if it returns false, then we need to do the loop again.

    }while( !ATOMIC_COMPARE_AND_SWAP( &(cift_event_buffer->next_event_index), next_index,  new_next_index) );

    //mark the event as invalid so that it will be dropped if read
    if (pEvent)
        pEvent->event_id = CIFT_EVENT_TYPE_INVALID;

    return pEvent;
}

static inline void cift_log_function_event( uint32_t event_id, void *func_called, void* called_from)
    __attribute__ ((no_instrument_function));

static inline void cift_log_function_event( uint32_t event_id, void *func_called, void* called_from)
{
    if (!cift_event_buffer)
    {
        //check if lazy init is enabled.
        #if (CIFT_DEFAULT_ENABLE)
            cift_default_init();

            //check again.  the code will crash if this value is NULL due to out of memory or something
            if (!cift_event_buffer)
                return;
        #else
            //the package has not be initialized and we are setup not to enable
            //by default, so all we can do is drop event and return
            return;
        #endif
    }

    // get timestamp before obtaining event instance (there is some degree of non-determinism in
    // cift_get_next_event as it uses a lockless busy-wait mechanism)
    CIFT_TIMESTAMP timestamp = cift_get_timestamp();

    CIFT_EVENT*  pEvent = cift_get_next_event();

    if (pEvent)
    {
        pEvent->timestamp     = timestamp;
        pEvent->context     = cift_get_current_thread();
        pEvent->called_from = (CIFT_ADDR)called_from;
        pEvent->func_called = (CIFT_ADDR)func_called;

        // increment statistic on total events collected.  use gcc atomic fetch/increment to avoid locking
        pEvent->event_count = ATOMIC_FETCH_AND_ADD( &(cift_event_buffer->total_events_added),1);

        pEvent->event_id     = event_id; //do this last to mark the event as complete
    }
    else
    {
        //we cannot log this event because of configuration or something, drop it
        ATOMIC_INCREMENT( &(cift_event_buffer->total_events_dropped) );
    }
}

void __cyg_profile_func_enter(void *func_called, void* called_from)
{
    if(cift_enabled)
    {
        cift_log_function_event(CIFT_EVENT_TYPE_FUNC_ENTER, func_called, called_from );
    }
}

void __cyg_profile_func_exit(void* func_called, void* called_from)
{
    if(cift_enabled)
    {
        cift_log_function_event(CIFT_EVENT_TYPE_FUNC_EXIT, func_called, called_from );
    }
}

CIFT_BOOL cift_set_enable(CIFT_BOOL enabled)
{
    //TODO: add additional code that spins on the lockless lock to make sure that no one is in the middle
    //of adding an event.  Perhaps we spin here until they are done?
    CIFT_BOOL old_enabled = cift_enabled;
    cift_enabled = enabled;
    return old_enabled;
}

CIFT_BOOL cift_get_enable(void)
{
    return cift_enabled;
}

CIFT_BOOL cift_configure( CIFT_COUNT totalBytes, CIFT_BUFFER_MODE bufferMode )
{
    CIFT_COUNT maxEventCount;

    // we are passed in the max number of bytes that we can allocate to the
    // buffer system.  this byte count includes the events as well as the
    // header info, so we have to calculate the max number of events we
    // can put in the buffer
    size_t eventBytes = (totalBytes - sizeof(CIFT_EVENT_BUFFER));

    //divide to get the number of events.  this will drop any number of bytes
    //at the end that are not aligned to event size as well
    maxEventCount = eventBytes / sizeof(CIFT_EVENT);

    if (cift_event_buffer)
    {
        cift_event_buffer = (CIFT_EVENT_BUFFER*)realloc(cift_event_buffer,totalBytes);
    }
    else
    {
        cift_event_buffer = (CIFT_EVENT_BUFFER*)malloc(totalBytes);
    }

    //sanity check on the memory allocation
    if (cift_event_buffer == 0)
        return 0;

    printf("CIFT initializing buffer for %u mode %u bytes, %u max events\n",(unsigned int)bufferMode, (unsigned int)totalBytes, (unsigned int)maxEventCount);

    cift_event_buffer->magic[0]             = 'C';
    cift_event_buffer->magic[1]             = 'I';
    cift_event_buffer->magic[2]             = 'F';
    cift_event_buffer->magic[3]             = 'T';
    cift_event_buffer->endian               = 0x4321;
    cift_event_buffer->addr_bytes           = (uint8_t)sizeof(void*);
    cift_event_buffer->cift_count_bytes     = (uint8_t)sizeof(CIFT_COUNT);
    cift_event_buffer->total_bytes          = totalBytes;
    cift_event_buffer->init_timestamp       = cift_get_timestamp();
    cift_event_buffer->buffer_mode          = bufferMode;
    cift_event_buffer->max_event_count      = maxEventCount;
    cift_event_buffer->next_event_index     = 0;
    cift_event_buffer->total_events_dropped = 0;
    cift_event_buffer->total_events_added   = 0;

    #ifdef _DEBUG
        memset( cift_event_buffer->event_buffer,0x00, eventBytes );
    #endif

    // if configured to do so, tell the C library to call our exit handler
    // function to flush the data to disk when the application exits
    #if ((CIFT_HAVE_FILEIO)&&(CIFT_ATEXIT_DUMP_LOG))
    atexit(cift_file_dump_atexit_handler);
    #endif

    return 1;
}

static void cift_default_init(void)
{
    cift_configure( CIFT_DEFAULT_MAX_BYTES, CIFT_DEFAULT_BUFFER_MODE );
}

#ifdef __cplusplus
} // end extern C
#endif

/**************************** END OF FILE ************************************/

