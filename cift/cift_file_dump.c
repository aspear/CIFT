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
* Please see notes in cift.h
********************************************************************************
*/

#include "cift.h"
#include <stdio.h>//fopen,fwrite
#include <stddef.h> //offsetof

#if (CIFT_HAVE_FILEIO)

void cift_file_dump_atexit_handler(void)
    __attribute__ ((no_instrument_function));

CIFT_COUNT cift_dump_to_text_file( const char* outputFile )
    __attribute__ ((no_instrument_function));

CIFT_COUNT cift_dump_to_text_file( const char* outputFile )
{
    CIFT_COUNT eventsInBuffers = 0;
    CIFT_COUNT startingIndex = 0;
    CIFT_COUNT index;
    CIFT_COUNT eventsPrinted=0;

    if (!cift_event_buffer)
    {
        printf("CIFT cift_event_buffer is NULL\n");
        return 0;
    }

    //do not allow modification of the structures while we are
    //iterating them
    cift_set_enable(0);

    FILE* fd = fopen(outputFile,"w");
    if (!fd)
    {
        printf("CIFT: unable to open output file %s\n",outputFile);
        return 0;
    }
    printf("CIFT: dumping events to file %s\n", outputFile);

    fprintf(fd,"******************************************************************************\n");
    fprintf(fd,"* magic                = %c %c %c %c\n",cift_event_buffer->magic[0],cift_event_buffer->magic[1],cift_event_buffer->magic[2],cift_event_buffer->magic[3]);
    fprintf(fd,"* endian              = %u\n",(unsigned)cift_event_buffer->endian);
    fprintf(fd,"* addr_bytes          = %u\n",(unsigned)cift_event_buffer->addr_bytes);
    fprintf(fd,"* cift_count_bytes    = %u\n",(unsigned)cift_event_buffer->cift_count_bytes);
    fprintf(fd,"* total_bytes         = %llu\n",(uint64_t)cift_event_buffer->total_bytes);
    fprintf(fd,"* init_timestamp      = %llu\n",(uint64_t)cift_event_buffer->init_timestamp);
    fprintf(fd,"* buffer_mode         = %u\n",cift_event_buffer->buffer_mode);
    fprintf(fd,"* max_event_count     = %llu\n",(uint64_t)cift_event_buffer->max_event_count);
    fprintf(fd,"* total_event_count   = %llu\n",(uint64_t)cift_event_buffer->total_event_count);
    fprintf(fd,"* dropped_event_count = %llu\n",(uint64_t)cift_event_buffer->dropped_event_count);
    fprintf(fd,"* next_event_index    = %llu\n",(uint64_t)cift_event_buffer->next_event_index);
    fprintf(fd,"* sizeof(CIFT_EVENT)=%u sizeof(CIFT_EVENT_BUFFER)=%u offsetof=%u\n",sizeof(CIFT_EVENT),sizeof(CIFT_EVENT_BUFFER),offsetof(CIFT_EVENT_BUFFER,event_buffer));
    fprintf(fd,"******************************************************************************\n");

    CIFT_EVENT* pEvent = 0;

    //calculate the total number of events that are in the buffers
    if (cift_event_buffer->buffer_mode == CIFT_BUFFER_MODE_CIRCULAR)
    {
        //figure out if we start from 0 or from next_event_index
        if (cift_event_buffer->total_event_count <= cift_event_buffer->max_event_count)
        {
            //we did not wrap, start at 0 and the count is the next index
            startingIndex = 0;
            eventsInBuffers = cift_event_buffer->next_event_index;
        }
        else
        {
            //we wrapped and are overwriting oldest data.  the oldest data is at
            //next_event_index, so start there
            startingIndex = cift_event_buffer->next_event_index;
            eventsInBuffers = cift_event_buffer->max_event_count;
        }
    }
    else
    {
        eventsInBuffers = cift_event_buffer->total_event_count;
        startingIndex = 0;
    }

    fprintf(fd,"* eventsInBuffers=%u startingIndex=%u\n",eventsInBuffers,startingIndex);
    fprintf(fd,"******************************************************************************\n");

    //write out a header for the data
    fprintf(fd,"timestamp,context,type,function,caller,total count\n");

    for (index=startingIndex;eventsInBuffers != 0;--eventsInBuffers)
    {
        CIFT_COUNT oldIndex = index;

        pEvent = &(cift_event_buffer->event_buffer[index]);

        //TODO implement a map that keeps track of call stack level for individual threads/contexts

        //TODO something smart with pEvent->event_type.  If it is 0, that means that the event was in the
        //middle of being written and is not complete and should be dropped

        eventsPrinted++;
        fprintf(fd,"%u,%llu,0x%08X,%u,0x%08llX,0x%08llX,%llu\n",
                        (unsigned)index,
                        //pEvent->timestamp - cift_event_buffer->init_timestamp,
                        (uint64_t)pEvent->timestamp,
                        (unsigned)pEvent->context,
                        (unsigned)pEvent->event_type,
                        (uint64_t)(uintptr_t)pEvent->func_called,
                        (uint64_t)(uintptr_t)pEvent->called_from,
                        (uint64_t)pEvent->event_count);

        index = (index + 1) % cift_event_buffer->max_event_count;//increment/wrap the index
    }
    fclose(fd);
    printf("* CIFT: %llu events dumped\n", (uint64_t)eventsPrinted);
    return eventsPrinted;
}

#define MIN(x, y) (((x) < (y)) ? (x) : (y)) //don't be stupid and pass mutable expressions in this

CIFT_COUNT cift_dump_to_bin_file( const char* outputFile )
{
    size_t chunkBytes = 4*1024; //write in 4k chunks
    size_t bytesToWrite;
    size_t bytesWritten;
    size_t bytesLeftToWrite;
    size_t totalBytesWritten = 0;

    if (!cift_event_buffer)
    {
        printf("CIFT cift_event_buffer is NULL\n");
        return 0;
    }

    //do not allow modification of the structures while we are
    //iterating them
    cift_set_enable(0);

    FILE* fd = fopen(outputFile,"wb");
    if (!fd)
    {
        printf("CIFT: unable to open output file %s\n",outputFile);
        return 0;
    }
    printf("CIFT: dumping events to binary file %s\n", outputFile);

    printf("**************************************************\n");
    printf("* magic                = %c %c %c %c\n",cift_event_buffer->magic[0],cift_event_buffer->magic[1],cift_event_buffer->magic[2],cift_event_buffer->magic[3]);
    printf("* endian              = %u\n",(unsigned)cift_event_buffer->endian);
    printf("* addr_bytes          = %u\n",(unsigned)cift_event_buffer->addr_bytes);
    printf("* cift_count_bytes    = %u\n",(unsigned)cift_event_buffer->cift_count_bytes);
    printf("* total_bytes         = %llu\n",(uint64_t)cift_event_buffer->total_bytes);
    printf("* init_timestamp      = %016llu\n",(uint64_t)cift_event_buffer->init_timestamp);
    printf("* buffer_mode         = %u\n",cift_event_buffer->buffer_mode);
    printf("* max_event_count     = %llu\n",(uint64_t)cift_event_buffer->max_event_count);
    printf("* total_event_count   = %llu\n",(uint64_t)cift_event_buffer->total_event_count);
    printf("* dropped_event_count = %llu\n",(uint64_t)cift_event_buffer->dropped_event_count);
    printf("* next_event_index    = %llu\n",(uint64_t)cift_event_buffer->next_event_index);
    printf("**************************************************\n");

    uint8_t* pBuffer = (uint8_t*)cift_event_buffer;
    bytesLeftToWrite = cift_event_buffer->total_bytes;

    for (;bytesLeftToWrite != 0;)
    {
        bytesToWrite = MIN(chunkBytes,bytesLeftToWrite );

        bytesWritten = fwrite(pBuffer,1,bytesToWrite,fd);
        if (bytesWritten != bytesToWrite)
        {
            fprintf(stderr,"ERROR: write to file failed\n");
            break;
        }
        bytesLeftToWrite -= bytesWritten;
        totalBytesWritten += bytesWritten;
        pBuffer += bytesWritten;
    }
    fclose(fd);
    printf("CIFT: %u bytes written\n", totalBytesWritten);
    return cift_event_buffer->total_event_count;
}

void cift_file_dump_atexit_handler()
{
    cift_set_enable(0);

    printf("\nCIFT exit handler called.\n");

    cift_dump_to_text_file("cift_output.txt");
    cift_dump_to_bin_file( "cift_output.bin");
}

#endif
