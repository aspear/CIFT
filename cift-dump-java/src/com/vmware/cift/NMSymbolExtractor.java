package com.vmware.cift;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.Reader;
import java.lang.Runtime;

import sun.misc.Regexp;
import java.util.regex.*;


public class NMSymbolExtractor {

	private RangeLookup rangeLookup;
	
	public NMSymbolExtractor() {
		// TODO Auto-generated constructor stub
	}
	
	public boolean init( String executablePath ){
		
		rangeLookup = new RangeLookup();
		
		File executableFile = new File(executablePath);
		if (!executableFile.exists() || !executableFile.isFile()){
			System.err.println("Executable '" + executablePath + "'doesn't exit");
			return false;//TODO what to throw?
		}
		
		generateSymbolFile(executablePath);
		
		String symbolFilePath = executablePath + ".nmsymbols";
		
		return parseSymbolFile(symbolFilePath);
	}
	
	/*
	 * nm --demangle -a -l -S --numeric-sort ./parallel_speed > ./parallel_speed.nmsymbols
	 */
	public boolean generateSymbolFile( String executablePath ) {
		try{
			File executableFile = new File(executablePath);
			if (!executableFile.exists() || !executableFile.isFile()){
				System.err.println("Executable '" + executablePath + "'doesn't exit");
				return false;//TODO what to throw?
			}
			
			String nmName = executableFile + ".nmsymbols";
			
			//delete the symbol file if it exists already
			File symbolFile = new File(nmName);
			if (symbolFile.exists()) {
				if (symbolFile.lastModified() > executableFile.lastModified()){
					//you are done because the symbol file exists and is newer than the executable
					return true;
				}
				else {
					//it was modifed, get rid of it and regenerate.
					symbolFile.delete();
					symbolFile = null;
				}
			}			
			
			File outputFile = new File(nmName);
			
			File outputDir = outputFile.getParentFile();
			
			//spawn "nm <executablePath>"
			String cmdOnly = "/usr/bin/nm --demangle -a -l -S --numeric-sort " + executableFile.getPath() + " > " + nmName;
			
			String[] command = {"/bin/sh", "-c", cmdOnly};
			Process p = Runtime.getRuntime().exec(command);

			try{
				p.wait(5);
			}catch(InterruptedException e){
				
			}
			
			//int exitValue = p.exitValue();
			//System.out.println("process exited with " + exitValue);
			
			symbolFile = new File(nmName);
			if (symbolFile.exists())
			{
				return true;
			}
		}catch(IOException e){
			System.err.println("Exception " + e.toString());
		}
		//catch(InterruptedException e){
		//	return false;
		//}
				
		return false;
	}
	
	/*
	 * 08048ad0 0000004c T card_class::suit_to_string()	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../card.cpp:32
	 * "[0-9A-Fa-f]+ [0-9A-Fa-f]+ [T] [A-Za-z0-9_\:]+ [ \ta-zA-Z0-9/-_\.]+\:[0-9]+" 
	 */
	public boolean parseSymbolFile(String symbolFilePath) {
		
		try{
					
			File symbolFile = new File(symbolFilePath);
			if (!symbolFile.exists() || !symbolFile.isFile()){
				System.err.println("Symbol file '" + symbolFilePath + "'doesn't exit");
				return false;//TODO what to throw?
			}
			
			File outputFile = new File(symbolFilePath + ".out");
			FileWriter writer = new FileWriter(outputFile);
			
			Pattern nmFuncRegexPattern = Pattern.compile("^([0-9A-Fa-f]+)[ \t]+([0-9A-Fa-f]+)[ \t]+T[ \t]+([^\\)]+)\\)[ \t]+([a-zA-Z0-9/-_\\.]+):([0-9]+)");
			
			Matcher lineMatcher = nmFuncRegexPattern.matcher("");
						
			Reader reader = new FileReader(symbolFile);
			BufferedReader bufferedReader = new BufferedReader(reader, 4*1024 ); // (symboReader reader = symbolFile.lFile);
			
			String line;
			try
			{
				int lineCount = 0;
				int linesInFileCount=0;
			
				while((line = bufferedReader.readLine()) != null)
				{
					int startAddress=0;
					int unitCount=0;
					String functionName;
					String fileName;
					int lineNumber=0;
					
					linesInFileCount++;
					
					//parse the line and extract the values.  match with regex that extracts
					//the values
					//TODO figure out how to reuse this Matcher instance
					//Matcher lineMatcher = nmFuncRegexPattern.matcher(line);
					lineMatcher.reset(line);
					
					if (!lineMatcher.find())
					{
						continue;
					}
					
					try
					{
						int groupCount = lineMatcher.groupCount();
						if (groupCount > 0)
						{
							//extract the value as a hex address
							startAddress = Integer.parseInt(lineMatcher.group(1),16);
						}
						if (lineMatcher.groupCount() > 1)
						{
							unitCount = Integer.parseInt(lineMatcher.group(2),16);
						}
						if (lineMatcher.groupCount() > 2)
						{
							functionName = lineMatcher.group(3);
						}
						else
						{
							functionName = "?";
						}
						if (lineMatcher.groupCount() > 3)
						{
							fileName = lineMatcher.group(4);
						}
						else
						{
							fileName = "?";
						}
						if (lineMatcher.groupCount() > 4)
						{
							lineNumber = Integer.parseInt(lineMatcher.group(5),10);
						}
						else
						{
							lineNumber = -1;
						}
						
						int endAddress = startAddress+unitCount-1;
						
						rangeLookup.addRange(startAddress,endAddress,functionName);
						
						writer.append(
								"0x" + Integer.toHexString(startAddress) 
								+ "\t"
								+ "0x" + Integer.toHexString(endAddress)
								+ "\t"
								+ functionName + ")"
								+ "\t"
								+ fileName
								+ "\t" + Integer.toString(lineNumber)
								+ "\n");
					}catch(NumberFormatException e){
						System.err.println("NumberFormatException " + e.toString() + "line="
								+ Integer.toString(linesInFileCount));
					}
								
					lineCount++;				
				}
			}
			catch(IOException e){
				e.printStackTrace();
			}
			writer.flush();
			return true;
		}
		catch(IOException e)
		{
			System.err.println("Exception " + e.toString());
			e.printStackTrace();
			return false;
		}
	}
	
	public String getStringForAddress( int address )
	{
		Range range = rangeLookup.lookupRange(address);
		if (range != null){
			
			int diff = address - range.lower;
			return range.functionName + "+" +  Integer.toString(diff);
		}
		else
		{
			return "0x"+Integer.toString(address,16);
		}
	}
	
	public void testAddress( int address )
	{
		Range range = rangeLookup.lookupRange(address);
		if (range != null){
			
			int diff = address - range.lower;
			
			System.out.println( Integer.toString(address,16) + " -> [" 
					+ Integer.toString(range.lower,16)
					+ "-" +  Integer.toString(range.upper,16)
					+ "] " + range.functionName + " + " +  Integer.toString(diff)  );
		}
		else
		{
			System.out.println( Integer.toString(address,16) + " -> null" );
		}
	}
	
	public void test() {
		
		//10) 804a770-804a7bc  function='stack_of_cards_class::size()'  file='/home/joe/trace/CIFT/demo/parallel_speed/Debug/../stack_of_cards.cpp':236
		//11) 804a7c0-804a815  function='stack_of_cards_class::last_card()'  file='/home/joe/trace/CIFT/demo/parallel_speed/Debug/../stack_of_cards.cpp':68
		//12) 804a820-804a869  function='stack_of_cards_class::next_card()'  file='/home/joe/trace/CIFT/demo/parallel_speed/Debug/../stack_of_cards.cpp':59
		
		testAddress(0x804a7c0-1);
		testAddress(0x804a7c0);
		testAddress(0x804a7c0+1);
		testAddress(0x804a815-1);
		testAddress(0x804a815);
		testAddress(0x804a815+1);
		
		//testAddress(0x080490f1);
		
		
		//08049080 00000068 T deck_class::swap_cards(card_class&, card_class&)	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../deck.cpp:36
		//080490f0 000001c4 T deck_class::initialize_suit(suit, int&, int)	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../deck.cpp:6
		//080492c0 000000ae T deck_class::deck_class()	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../deck.cpp:22
		//08049370 000000ae T deck_class::deck_class()	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../deck.cpp:22
		
	}
}
