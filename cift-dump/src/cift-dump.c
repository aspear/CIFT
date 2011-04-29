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
* the cift-dump utility is used for dumping the contents of binary cift buffers.
* the idea is that you can collect trace data in memory, write the raw buffers to
* a file, then at some point later on convert that file into a standard format
* (e.g. CTF)
********************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h> //offsetof
#include <endian.h> //gcc endian conversion utils

#include <sys/mman.h> //memory mapped files
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> //close, open

#include "cift.h"

//if true we are going to endian convert
int endian_swap = 0;
int addr_bytes  = 4;

inline uint32_t ExtractU32( volatile void* addr )
{
    if (endian_swap)
        return __bswap_32( *((uint32_t*)addr) );
    else
        return *((uint32_t*)addr);
}
inline uint64_t ExtractU64( volatile void* addr )
{
    if (endian_swap)
        return __bswap_64( *((uint64_t*)addr) );
    else
        return *((uint64_t*)addr);
}
inline uint64_t ExtractUINTPTR( volatile void* addr )
{
    if (addr_bytes == 4)
        return (uint64_t)ExtractU32(addr);
    else
        return ExtractU64(addr);
}

const char* usage = "cift-dump <filename>\n";


int cift_dump_plain_to_stdout( const char* filename );

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr,"%s", usage );
        return EXIT_FAILURE;
    }

    return cift_dump_plain_to_stdout( argv[1] );
}

int cift_dump_plain_to_stdout( const char* filename )
{
    uint64_t eventsInBuffers = 0;
    uint64_t startingIndex = 0;
    uint64_t index;
    uint64_t eventsPrinted=0;
    uint8_t  cift_count_bytes;
    uint64_t total_bytes;
    uint64_t init_timestamp;
    unsigned buffer_mode;
    uint64_t max_event_count;
    uint64_t total_events_added;
    uint64_t total_events_dropped;
    uint64_t next_event_index;
    int    fd;
    size_t  totalFileLength;
    struct stat statBuffer;
    int status;

    CIFT_EVENT_BUFFER* cift_event_buffer;

    fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        fprintf(stderr,"cift-dump: unable to open file '%s'\n",filename);
        return EXIT_FAILURE;
    }

    //figure out the total size of the file so we can map it all
    status = fstat(fd, &statBuffer);
    totalFileLength = statBuffer.st_size;

    //sanity check the file length
    if ( totalFileLength < sizeof(CIFT_EVENT_BUFFER))
    {
       	close(fd);
       	fprintf(stderr,"cift-dump: file %s is smaller than sizeof(CIFT_EVENT_BUFFER).  There is no way it is a valid file.\n",
       			filename);
        return EXIT_FAILURE;
    }

    printf("cift-dump: opened %s, file size: %u\n",filename,totalFileLength);

    //memory map the file
    cift_event_buffer = (CIFT_EVENT_BUFFER*)mmap(0, totalFileLength, PROT_READ, MAP_SHARED, fd, 0);
    if (cift_event_buffer == MAP_FAILED)
    {
        close(fd);
        fprintf(stderr,"cift-dump: Error mmapping the file\n");
        return EXIT_FAILURE;
    }

    //do a sanity check on the file.  see if the magic value is there.
    if (       cift_event_buffer->magic[0] != 'C'
    		||(cift_event_buffer->magic[1] != 'I')
    		||(cift_event_buffer->magic[2] != 'F')
    		||(cift_event_buffer->magic[3] != 'T'))
    {
    	munmap(cift_event_buffer, totalFileLength);
    	close(fd);
    	 fprintf(stderr,"cift-dump: file %s is missing the CIFT magic value at the start of the file.  It does not appear to be a valid image\n",
    			filename);
        return EXIT_FAILURE;
    }

    //figure out endian value first. if the value is 4321, that means that the target is the
    //same endian as the host
    endian_swap         = (cift_event_buffer->endian != 4321);

    addr_bytes          = cift_event_buffer->addr_bytes;
    cift_count_bytes    = cift_event_buffer->cift_count_bytes;
    total_bytes         = (uint64_t)ExtractU64( &cift_event_buffer->total_bytes );
    init_timestamp      = (uint64_t)ExtractU64( &cift_event_buffer->init_timestamp);
    buffer_mode         = (unsigned)ExtractU32( &cift_event_buffer->buffer_mode);
    max_event_count     = (uint64_t)ExtractUINTPTR(&cift_event_buffer->max_event_count);
    total_events_added   = (uint64_t)ExtractUINTPTR(&cift_event_buffer->total_events_added);
    total_events_dropped = (uint64_t)ExtractUINTPTR(&cift_event_buffer->total_events_dropped);
    next_event_index    = (uint64_t)ExtractUINTPTR(&cift_event_buffer->next_event_index);

    printf("******************************************************************************\n");
    printf("* magic               = %c %c %c %c\n",cift_event_buffer->magic[0],cift_event_buffer->magic[1],cift_event_buffer->magic[2],cift_event_buffer->magic[3]);
    printf("* endian              = %u\n",    (unsigned)cift_event_buffer->endian);
    printf("* addr_bytes          = %u\n",        (unsigned)cift_event_buffer->addr_bytes);
    printf("* cift_count_bytes    = %u\n",        (unsigned)cift_count_bytes);
    printf("* total_bytes         = %llu\n",    (uint64_t)total_bytes );
    printf("* init_timestamp      = %llu\n",    (uint64_t)init_timestamp);
    printf("* buffer_mode         = %u\n",        (unsigned)buffer_mode);
    printf("* max_event_count     = %llu\n",    (uint64_t)max_event_count);
    printf("* total_events_added   = %llu\n",    (uint64_t)total_events_added);
    printf("* total_events_dropped = %llu\n",    (uint64_t)total_events_dropped);
    printf("* next_event_index    = %llu\n",    (uint64_t)next_event_index);
    printf("* sizeof(CIFT_EVENT)=%u sizeof(CIFT_EVENT_BUFFER)=%u offsetof=%u\n",sizeof(CIFT_EVENT),sizeof(CIFT_EVENT_BUFFER),offsetof(CIFT_EVENT_BUFFER,event_buffer));
    printf("******************************************************************************\n");

    CIFT_EVENT* pEvent = 0;

    //calculate the total number of events that are in the buffers
    if (buffer_mode == CIFT_BUFFER_MODE_CIRCULAR)
    {
        //figure out if we start from 0 or from next_event_index
        if (total_events_added <= max_event_count)
        {
            //we did not wrap, start at 0 and the count is the next index
            startingIndex = 0;
            eventsInBuffers = cift_event_buffer->next_event_index;
        }
        else
        {
            //we wrapped and are overwriting oldest data.  the oldest data is at
            //next_event_index, so start there
            startingIndex = next_event_index;
            eventsInBuffers = max_event_count;
        }
    }
    else
    {
        eventsInBuffers = total_events_added;
        startingIndex = 0;
    }
    printf("* eventsInBuffers=%llu startingIndex=%llu\n",eventsInBuffers,startingIndex);
    printf("******************************************************************************\n");

    //write out a header for the data
    printf("timestamp,context,type,function,caller,total count\n");

    for (index=startingIndex;eventsInBuffers != 0;--eventsInBuffers)
    {
        pEvent = &(cift_event_buffer->event_buffer[index]);

        //TODO implement a map that keeps track of call stack level for individual threads/contexts

        //TODO something smart with pEvent->event_id.  If it is 0, that means that the event was in the
        //middle of being written and is not complete and should be dropped

        eventsPrinted++;
        printf("%llu,%llu,0x%08X,%u,0x%08llX,0x%08llX,%llu\n",
                (uint64_t)index,
                (uint64_t)ExtractU64(&pEvent->timestamp),
                (unsigned)ExtractU32(&pEvent->context),
                (unsigned)ExtractU32(&pEvent->event_id),
                (uint64_t)ExtractUINTPTR( &pEvent->func_called ),
                (uint64_t)ExtractUINTPTR( &pEvent->called_from ),
                (uint64_t)ExtractUINTPTR( &pEvent->event_count )
             );

        index = (index + 1) % max_event_count;//increment/wrap the index
    }
    printf("******************************************************************************\n");
    printf("* cift-dump: %llu events dumped\n", (uint64_t)eventsPrinted);

    if (munmap(cift_event_buffer, totalFileLength) == -1)
    {
        fprintf(stderr,"Error un-mmapping the file\n");
    }

    close(fd);

    return EXIT_SUCCESS;
}
