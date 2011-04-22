/*******************************************************************************
* CIFT : Compiler Instrumentation Function Tracer
*
* Copyright (c) 2011 Aaron Spear  aaron@ontherock.com
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

#ifndef __CIFT_OS_TIMER_H__
#define __CIFT_OS_TIMER_H__

#include "cift.h"

#include <pthread.h>

//very strange, but this noinstrument syntax requires declaration to be seperate from
//implementation for some reason
static inline CIFT_CONTEXT cift_get_current_thread(void)
    __attribute__ ((no_instrument_function));

/** prototype of the function that is used to get the current thread
 * for the current OS.  it is by definition OS specific, and so is provided
 * by another C file that you must link in */
static inline CIFT_CONTEXT cift_get_current_thread(void)
{
    return (CIFT_CONTEXT)pthread_self();
}

#endif //__CIFT_OS_TIMER_H__

