/*******************************************************************************
* CIFT : Compiler Instrumentation Function Tracer
*
* Copyright (c) 2011 Aaron Spear  aspear@vmware.com
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
* CIFT : Compiler Instrumentation Function Tracer IS:
* A simple static library that allows usage of gcc function entry/exit
* instrumentation as a means to generate a trace of an arbitrary application.
* This library provides hooks that gcc calls on function entry/exit, and in
* those hooks we record the function call data in a circular buffer.  This buffer
* can then be converted to a CTF standard file for analysis by a trace analyzer.
********************************************************************************
* To use this, you need to:
* 1) recompile application sources that you want to be instrumented with 
*    -finstrument-functions.  this will result in some unresolved externals that
*    this library provides.
* 2) link with this library.  Do NOT build this library with the switch above
* 3) run your app.  when it exits a trace will be dumped to both a text and
* binary file. the text file is self explanatory.  the binary needs to be
* converted into a CTF file via the cift-dump utility program
*
* Note that the library provides a good bit of flexibility in terms of
* configuration. You can:
* -set the sizes of buffers
* -configure the buffering mode (whether you fill and then stop, or log
*  circularly)
* -enable/disable trace collection
* -dump the log to files at any time
* TODO examples
********************************************************************************
*/

#ifndef __CIFT_H__
#define __CIFT_H__

//include C99 standard types, which this lib needs
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* CFT compile time configuration
*******************************************************************************/

//enable the optional feature to dump the log to files
#ifndef CIFT_HAVE_FILEIO
#define CIFT_HAVE_FILEIO 1
#endif

// by default if the FILEIO is enabled the log is dumped to a file
// when the application exits.  this is really just for ease of use
#ifndef CIFT_ATEXIT_DUMP_LOG
#define CIFT_ATEXIT_DUMP_LOG CIFT_HAVE_FILEIO
#endif

/** configuration of whether or not tracing is enabled immediately by default. Note that
 allowing this option to be set to true will incur a small additional penalty (variable
 check) on every function hook due to lazy initialization
 */
#ifndef CIFT_DEFAULT_ENABLE
#define CIFT_DEFAULT_ENABLE 1
#endif

//the maximum number of bytes to allocate for the buffer.  only used if
//CIFT_DEFAULT_ENABLE is defined to be 1.  Otherwise you must configure
//manually
#ifndef CIFT_DEFAULT_MAX_BYTES
#define CIFT_DEFAULT_MAX_BYTES (1024*1024*4)
#endif

#ifndef CIFT_DEFAULT_BUFFER_MODE
#define CIFT_DEFAULT_BUFFER_MODE CIFT_BUFFER_MODE_CIRCULAR
#endif

#if (CIFT_HAVE_FILEIO)
    #ifndef CIFT_DEFAULT_TRACE_FILE_NAME
        #define CIFT_DEFAULT_TRACE_FILE_NAME "function_trace.ctf"
    #endif
#endif

/*******************************************************************************
* CFT data types
*/

/**
* available modes for the buffering.  either the buffering is setup to circularly
* overwrite the oldest data (aka "flight recorder mode") or is setup to stop when
* the buffer is full
*/
typedef enum
{
    CIFT_BUFFER_MODE_CIRCULAR        = 0,
    CIFT_BUFFER_MODE_FILL_AND_STOP    = 1,
} CIFT_BUFFER_MODE;

typedef uint_fast32_t  CIFT_BOOL;      // type used for all boolean operations
typedef uintptr_t      CIFT_COUNT;     // type used for counts of events as well as the buffer size
typedef uint32_t       CIFT_CONTEXT;     // type used for thread id's
typedef uint64_t       CIFT_TIMESTAMP;
typedef uintptr_t      CIFT_ADDR;          // a PC value/function address

/**
 * enum for the event id's used by this library
 */
enum CIFT_EVENT_TYPE_VALUES
{
    CIFT_EVENT_TYPE_INVALID    = 0,
    CIFT_EVENT_TYPE_FUNC_ENTER = 1,
    CIFT_EVENT_TYPE_FUNC_EXIT  = 2
};
typedef uint32_t CIFT_EVENT_TYPE;  //explicit sizing of the enum

/** the fixed size event that we put in the trace buffers. 28 bytes on
 * a 32 bit machine, 32 bytes on a 64 bit machine */
typedef struct
{
    CIFT_EVENT_TYPE event_type;  // the type of the event, see CIFT_EVENT_TYPE
    CIFT_TIMESTAMP  timestamp;   // high precision timestamp
    CIFT_CONTEXT    context;     // the thread of the event
    CIFT_ADDR       func_called; // the function that was called
    CIFT_ADDR       called_from; // the location that the call was made
    CIFT_COUNT      event_count; // the event count at the time of recording this event (for debugging)
} CIFT_EVENT;

/* the main control buffer.  note that it is setup such that the fields that reflect the current state
 * of the buffer are the first members in the structure.  doing this facilitates an easy way to dump
 * the entire buffer to a file and then post mortem convert it into a CTF file for example
 */
typedef struct
{
    char                magic[4];            //'C','I','F','T'
    uint16_t            endian;              // the value1 is written into it (by implication you can figure out if big or little)
    uint8_t             addr_bytes;          // target architecture number of bytes in a pointer
    uint8_t             cift_count_bytes;    // target architecture bytes in a CIFT_COUNT type instance
    uint8_t             header_bytes;
    uint64_t            total_bytes;         // total number of bytes in this buffer including this header
    uint64_t            init_timestamp;      // the timestamp at the init of the buffer
    CIFT_BUFFER_MODE    buffer_mode;         // CIFT_BUFFER_MODE mode of the buffer
    CIFT_COUNT          max_event_count;     // the total number of events that the buffer can hold
    volatile CIFT_COUNT next_event_index;    // the index of next available event
    volatile CIFT_COUNT total_event_count;   // the number of events that have been added (statistic...)
    volatile CIFT_COUNT dropped_event_count; // the number of events that have been dropped (statistic...)
    CIFT_EVENT           event_buffer[0];    // the main event buffer circular buffer
} CIFT_EVENT_BUFFER;

//single global variable holding the buffer for the function events
extern CIFT_EVENT_BUFFER* cift_event_buffer;

/*******************************************************************************
* * CFT control API's
*/

/** 
* configure the available size for the function trace buffer as well as the 
* tracing mode. Note that this must be done while tracing is disabled.
* @param totalBytes the number of bytes to allocate for buffering events and configuration
* data structures (they are all allocated together).  this is the amount of memory that will
* be allocated via malloc
* @param mode the configuration for the buffer mode
* @return non zero if sucessful. 0 if failure.
*/
CIFT_BOOL cift_configure( CIFT_COUNT totalBytes, CIFT_BUFFER_MODE mode );

/**
 * enable/disable tracing
 * @param enabled non zero to enable, 0 to disable
 * @return previous enable state
 */
CIFT_BOOL cift_set_enable(CIFT_BOOL enabled);

/**
 * @return the current enable state of the tracing
 */
CIFT_BOOL cift_get_enable(void);

#if (CIFT_HAVE_FILEIO)

    /**
     * dump the entire buffer out to a binary file.  the implication
     * is that later on you will use another tool (e.g. babeltrace) to
     * actually convert this data to CTF
     */
     CIFT_COUNT cift_dump_to_bin_file( const char* outputFile )
        __attribute__ ((no_instrument_function));

    /**
     * dump the buffer out to a readable/parsable text file
     */
     CIFT_COUNT cift_dump_to_text_file( const char* outputFile )
        __attribute__ ((no_instrument_function));

#else


//it is assumed that if this is the case you are going to use some other mechanism
//to move the trace data buffers to a host for processing, e.g. using a debugger,
//writing some sort of daemon of your own, etc

#endif

#ifdef __cplusplus
} // end extern C
#endif



#endif //__CIFT_H__

