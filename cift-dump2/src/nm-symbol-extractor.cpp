#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "address-range-lookup.h"

using namespace std;

typedef string String;

class FunctionInfo {
public:
	FunctionInfo(uint64_t _lowAddress,
			     uint64_t _highAddress,
			     const String& _name, const String& _file, int _line)
	:lowAddress(_lowAddress),highAddress(_highAddress),name(_name),file(_file),line(_line)
	 {}
	uint64_t lowAddress;
	uint64_t highAddress;
	String name;
	String file;
	int    line;
};

class NMSymbolExtractor {

private:
	RangeLookup<uint64_t,FunctionInfo> rangeLookup;

public:
	NMSymbolExtractor(){
		// TODO Auto-generated constructor stub
	}
	~NMSymbolExtractor()
	{
	}

	bool init( String executablePath ){

		rangeLookup.clear();

		//generateSymbolFile(executablePath);

		String symbolFilePath = executablePath + ".nmsymbols";

		return parseSymbolFile(symbolFilePath);
	}

	/*
	 * nm --demangle -a -l -S --numeric-sort ./parallel_speed > ./parallel_speed.nmsymbols
	 */
	/*bool generateSymbolFile( String executablePath ) {
		try{
			File executableFile = new File(executablePath);
			if (!executableFile.exists() || !executableFile.isFile()){
				fprintf(stderr,"Executable '" + executablePath + "'doesn't exit");
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
			//printf("process exited with " + exitValue);

			symbolFile = new File(nmName);
			if (symbolFile.exists())
			{
				return true;
			}
		}catch(IOException e){
			fprintf(stderr,"Exception " + e.toString());
		}
		//catch(InterruptedException e){
		//	return false;
		//}

		return false;
	}
	*/

	/*
	 * assume format is:
	 * 0x8048ad0	0x8048b1b	card_class::suit_to_string()	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../card.cpp	32
	 * lowaddr \t highaddr \t function \t file \t line \n
	 */
	bool parseSymbolFile(String symbolFilePath) {

		FILE* fd = fopen(symbolFilePath.c_str(),"rt");
		if (!fd)
		{
			return false;
		}

		char buffer[1024];

		char* line;
		while( (line=fgets(buffer,sizeof(buffer)-1,fd))!= 0)
		{
			uint64_t startAddress=0;
			uint64_t endAddress=0;
			String functionName;
			String fileName;
			int lineNumber=-1;
			char* pEnd=0;

			char* tok = strtok(line,"\t");

			startAddress = (uint64_t)strtoull(tok,&pEnd,0);

			tok = strtok(NULL,"\t");
			endAddress = (uint64_t)strtoull(tok,&pEnd,0);
			tok = strtok(NULL,"\t");
			if (tok){
				functionName = tok;

				tok = strtok(NULL,"\t");
				if (tok){
					fileName = tok;
				}
				tok = strtok(NULL,"\t");
				if (tok){
					lineNumber = strtol(tok,&pEnd,0);
				}
			}

			FunctionInfo info(startAddress,endAddress,functionName,fileName,lineNumber);

			rangeLookup.insertRange(startAddress,endAddress,info);
		}
	}

	String getStringForAddress( uint64_t address )
	{
		char buffer[1024];
		FunctionInfo* pInfo = rangeLookup.findRange(address);
		if (pInfo)
		{
			snprintf(buffer,sizeof(buffer)-1,"%s+%d",pInfo->name.c_str(),(int)(address - pInfo->lowAddress));
		}
		else
		{
			snprintf(buffer,sizeof(buffer)-1,"0x%llX",address);
		}
		buffer[sizeof(buffer)-1]=0;
		return String(buffer);
	}

	void testAddress( uint64_t address )
	{
		String rangeString = getStringForAddress(address);

		printf( "0x%08llX -> %s\n",address,rangeString.c_str());
	}

	void test() {

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
};
