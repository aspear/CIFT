package com.vmware.cift;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.FileChannel.MapMode;
//import NMSymbolExtractor;




/**
 * 
 */

/**
 * @author joe
 *
 */
public class CIFT_Dumper {

	public static final int EXIT_FAILURE = -1;
	public static final int EXIT_SUCESS = 0;
	
	private boolean endian_swap;
	private int addr_bytes;
	
	private NMSymbolExtractor symbolExtracter;
	/**
	 * @param args first arg is the path to the exe.  the second is the CIFT file
	 */
	public static void main(String[] args) {
		
		CIFT_Dumper dumper = new CIFT_Dumper();
		
		// TODO Auto-generated method stub
		System.out.println("CIFT dumper");
		if (args.length < 2){
			System.out.println("usage: cift-dump <path to executable> <path to CIFT dump file>");
			return;
		}
		//for (int i=0;i < args.length;++i){
		//	System.out.println("arg " + i + "=" + args[i] );
		//	
		//}
				
		//symbolExtracter.test();
		dumper.readCIFTFile(args[0],args[1]);
	}
	
	public void CIFT_Dumper() {
		addr_bytes = 4;
	}
		
	public static ByteBuffer byteBufferForFile(String fname){
	    FileChannel vectorChannel;
	    ByteBuffer vector;
	    try {
	        vectorChannel = new FileInputStream(fname).getChannel();
	    } catch (FileNotFoundException e1) {
	        e1.printStackTrace();
	        return null;
	    }
	    try {
	        vector = vectorChannel.map(MapMode.READ_ONLY,0,vectorChannel.size());
	    } catch (IOException e) {
	        e.printStackTrace();
	        return null;
	    }
	    return vector;
	}

	public void readCIFTFile( String executableFilePath, String ciftFilePath ) {
		symbolExtracter = new NMSymbolExtractor();
		if (!symbolExtracter.init(executableFilePath)){
			System.err.println("ERROR: unable to initialize symbol dumper");
			return;
		}		
	}
	
	public int ExtractU32( ByteBuffer buffer, int offset )
	{
	    if (endian_swap)
	    	return buffer.getInt(offset); //FIXME
	    else
	    	return buffer.getInt(offset);
	}
	public long ExtractU64( ByteBuffer buffer, int offset )
	{
	    if (endian_swap)
	    	return buffer.getLong(offset); //FIXME
	    else
	    	return buffer.getLong(offset);
	}
	public long ExtractUINTPTR( ByteBuffer buffer, int offset )
	{
		if (addr_bytes == 4)
		      return (long)ExtractU32(buffer,offset);
		else
		     return ExtractU64(buffer,offset);
	}
	
	public int cift_dump_plain_to_stdout( String filename )
		{
		    long eventsInBuffers = 0;
		    long startingIndex = 0;
		    long index;
		    long eventsPrinted=0;
		    byte  cift_count_bytes;
		    long total_bytes;
		    long init_timestamp;
		    int buffer_mode;
		    long max_event_count;
		    long total_events_added;
		    long total_events_dropped;
		    long next_event_index;
		    int    fd;
		    long  totalFileLength=0;;
		    
		    ByteBuffer cift_event_buffer = byteBufferForFile(filename);

		    if (cift_event_buffer ==  null)
		    {
		    	
		        System.err.printf("cift-dump: unable to open file '%s'\n",filename);
		        return EXIT_FAILURE;
		    }

		    //sanity check the file length
		    if ( totalFileLength < 12)//sizeof(CIFT_EVENT_BUFFER))
		    {
		       	System.err.printf("cift-dump: file %s is smaller than sizeof(CIFT_EVENT_BUFFER).  There is no way it is a valid file.\n",
		       			filename);
		        return EXIT_FAILURE;
		    }

		    System.out.printf("cift-dump: opened %s, file size: %u\n",filename,totalFileLength);

		    //do a sanity check on the file.  see if the magic value is there.
		    if (       cift_event_buffer.get(0) != 'C'
		    		||(cift_event_buffer.get(1) != 'I')
		    		||(cift_event_buffer.get(2) != 'F')
		    		||(cift_event_buffer.get(3) != 'T'))
		    {
		    	System.err.printf("cift-dump: file %s is missing the CIFT magic value at the start of the file.  It does not appear to be a valid image\n",
		    			filename);
		        return EXIT_FAILURE;
		    }

		    /*
		     * //figure out endian value first. if the value is 4321, that means that the target is the
		     
		    //same endian as the host
		    endian_swap         = (cift_event_buffer->endian != 4321);

		    addr_bytes          = cift_event_buffer->addr_bytes;
		    cift_count_bytes    = cift_event_buffer->cift_count_bytes;
		    total_bytes         = (long)ExtractU64( &cift_event_buffer->total_bytes );
		    init_timestamp      = (long)ExtractU64( &cift_event_buffer->init_timestamp);
		    buffer_mode         = (int)ExtractU32( &cift_event_buffer->buffer_mode);
		    max_event_count     = (long)ExtractUINTPTR(&cift_event_buffer->max_event_count);
		    total_events_added   = (long)ExtractUINTPTR(&cift_event_buffer->total_events_added);
		    total_events_dropped = (long)ExtractUINTPTR(&cift_event_buffer->total_events_dropped);
		    next_event_index    = (long)ExtractUINTPTR(&cift_event_buffer->next_event_index);

		    System.out.printf("******************************************************************************\n");
		    System.out.printf("* magic               = %c %c %c %c\n",cift_event_buffer->magic[0],cift_event_buffer->magic[1],cift_event_buffer->magic[2],cift_event_buffer->magic[3]);
		    System.out.printf("* endian              = %u\n",    (int)cift_event_buffer->endian);
		    System.out.printf("* addr_bytes          = %u\n",        (int)cift_event_buffer->addr_bytes);
		    System.out.printf("* cift_count_bytes    = %u\n",        (int)cift_count_bytes);
		    System.out.printf("* total_bytes         = %llu\n",    (long)total_bytes );
		    System.out.printf("* init_timestamp      = %llu\n",    (long)init_timestamp);
		    System.out.printf("* buffer_mode         = %u\n",        (int)buffer_mode);
		    System.out.printf("* max_event_count     = %llu\n",    (long)max_event_count);
		    System.out.printf("* total_events_added   = %llu\n",    (long)total_events_added);
		    System.out.printf("* total_events_dropped = %llu\n",    (long)total_events_dropped);
		    System.out.printf("* next_event_index    = %llu\n",    (long)next_event_index);
		   // System.out.printf("* sizeof(CIFT_EVENT)=%u sizeof(CIFT_EVENT_BUFFER)=%u offsetof=%u\n",sizeof(CIFT_EVENT),sizeof(CIFT_EVENT_BUFFER),offsetof(CIFT_EVENT_BUFFER,event_buffer));
		    System.out.printf("******************************************************************************\n");

		   // CIFT_EVENT* pEvent = 0;

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
		    System.out.printf("* eventsInBuffers=%llu startingIndex=%llu\n",eventsInBuffers,startingIndex);
		    System.out.printf("******************************************************************************\n");

		    //write out a header for the data
		    System.out.printf("timestamp,context,type,function,caller,total count\n");

		    for (index=startingIndex;eventsInBuffers != 0;--eventsInBuffers)
		    {
		        pEvent = &(cift_event_buffer->event_buffer[index]);

		        //TODO implement a map that keeps track of call stack level for individual threads/contexts

		        //TODO something smart with pEvent->event_id.  If it is 0, that means that the event was in the
		        //middle of being written and is not complete and should be dropped

		        eventsPrinted++;
		        System.out.printf("%llu,%llu,0x%08X,%u,0x%08llX,0x%08llX,%llu\n",
		                (long)index,
		                (long)ExtractU64(&pEvent->timestamp),
		                (int)ExtractU32(&pEvent->context),
		                (int)ExtractU32(&pEvent->event_id),
		                (long)ExtractUINTPTR( &pEvent->func_called ),
		                (long)ExtractUINTPTR( &pEvent->called_from ),
		                (long)ExtractUINTPTR( &pEvent->event_count )
		             );

		        index = (index + 1) % max_event_count;//increment/wrap the index
		    }
		    System.out.printf("******************************************************************************\n");
		    System.out.printf("* cift-dump: %llu events dumped\n", (long)eventsPrinted);

		    if (munmap(cift_event_buffer, totalFileLength) == -1)
		    {
		        System.err.printf("Error un-mmapping the file\n");
		    }

		    close(fd);
	}
	*/
		    return EXIT_FAILURE;
		}
}
