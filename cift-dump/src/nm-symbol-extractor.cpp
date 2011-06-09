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
*/
#include "nm-symbol-extractor.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "re2/regexp.h"

using namespace std;

SymbolLookup::SymbolLookup()
{
}
SymbolLookup::~SymbolLookup()
{
}

bool SymbolLookup::init(){

	rangeLookup.clear();
	return true;
}

bool SymbolLookup::loadSymbolsForExecutable( const String& executablePath, uint64_t addressBias/*=0*/ )
{
	if (!generateNMSymbolFile(executablePath))
		return false;

	String nmSymbolFilePath = executablePath + ".nmsymbols";
	return parseNMSymbolFile(nmSymbolFilePath,addressBias);

	//String symbolFilePath = executablePath + ".symbolsmap";
	//return parseSymbolFile(symbolFilePath);
}

/*
 * nm --demangle -a -l -S --numeric-sort ./parallel_speed > ./parallel_speed.nmsymbols
 */
bool SymbolLookup::generateNMSymbolFile( const String& executablePath ) {
	FILE* fd = fopen(executablePath.c_str(),"r");
	if (!fd)
	{
		fprintf(stderr,"ERROR: passing in executable doesn't exist\n");
		return false;
	}
	fclose(fd);

	String nmName = executablePath + ".nmsymbols";

	//delete the symbol file if it exists already
	int retVal = remove(nmName.c_str());

	//spawn nm to dump the symbols for the executable to a text file
	String cmdOnly = "/usr/bin/nm --demangle -a -l -S --numeric-sort " + executablePath + " > " + nmName;

	//not sure if this is going to work on windows...
	retVal = system(cmdOnly.c_str());

	return retVal == 0;
}

static const char* skipWhiteSpace(const char* pName)
{
	while(pName && isspace(*pName))
		pName++;
	return pName;
}

/*
 * @param pStart the string to scan an identifier from
 * @param name the string to append the name too
 * @return the end position
 */
char* extractFunctionOrLabelName(const char* pName, String& name)
{
	unsigned parenCount=0;
	for(;pName != 0 && *pName;++pName)
	{
		char c = *pName;
		if (c == '(')
		{
			parenCount++;
		}
		else if (c == ')')
		{
			parenCount--;
		}
		//if the paren count is 0 and this is a space we are done
		if ((parenCount == 0) && isspace(c))
		{
			break;
		}
		else
		{
			name += c;
		}
	}
	return (char*)skipWhiteSpace(pName);
}

/*

08048750 t .plt
08048890 t .text
08048890 T _start
080488c0 t __do_global_dtors_aux
08048920 t frame_dummy
08048944 00000091 T function4(int)	/home/joe/trace/CIFT/demo/callStackTest/Debug/../src/callStackTest.cpp:18
08048ba0 000000b8 T main	/home/joe/trace/CIFT/demo/callStackTest/Debug/../src/callStackTest.cpp:52
08048c58 00000092 t __static_initialization_and_destruction_0(int, int)	/home/joe/trace/CIFT/demo/callStackTest/Debug/../src/callStackTest.cpp:57
08048cea 00000042 t global constructors keyed to someVariable
08048d2c 00000016 t cift_get_timestamp	/home/joe/trace/CIFT/cift/Debug/../cycle.h:211
08049860 00000005 T __libc_csu_fini
08049870 0000005a T __libc_csu_init
080498ca T __i686.get_pc_thunk.bx
080498d0 0000003b T atexit
08049910 t __do_global_ctors_aux
0804993c t .fini
0804993c T _fini
08049958 r .rodata
08049958 00000004 R _fp_hw
0804995c 00000004 R _IO_stdin_used
08049dd4 r .eh_frame_hdr
08049e18 r .eh_frame
*/

bool SymbolLookup::parseNMSymbolFile( const String& symbolFilePath, uint64_t addressBias/*=0*/ )
{
	int retVal;
	int lineCount = 0;
	int linesInFileCount=0;
	uint64_t lastAddress=0;
	uint64_t currentAddress=0;

	FILE* fd = fopen(symbolFilePath.c_str(),"rt");
	if (!fd)
	{
		return false;
	}

	char buffer[1024];
	regexp* re_file_line=0;

	//this pattern works in java but fails with stock POSIX regcomp,regexec.  I punted and used a library that
	//I know works
	// lines are formatted like this:
	// 08048ad0 0000004c T card_class::suit_to_string()	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../card.cpp:32
	const char* fileLinePattern = "([^:]+):([0-9]+)";

	retVal =  re_comp(&re_file_line,fileLinePattern);
	if (retVal< 0)
	{
		fprintf(stderr,"ERROR failed to compile regex '%s'\n",fileLinePattern);
		return false;
	}

	bool waitingForEndOfRange=false;

	//the functionInfo is always the PREVIOUS line, that is the one you want to add.  so, when you get a new
	//address, it is finishing the previous address
	FunctionInfo functionInfo;

	unsigned subExpCount = re_nsubexp(re_file_line);

	regmatch* matches = new regmatch[subExpCount];

	char* line;
	while( (line=fgets(buffer,sizeof(buffer)-1,fd))!= 0)
	{
		char* pEnd=0;
		const char* pStart=line;

		linesInFileCount++;

		//first off check to see if we can extract an address from the current line
		currentAddress = (uint64_t)strtoull(pStart,&pEnd,16);
		if (pEnd==line || pEnd == 0)
		{
			//unable to extract an address as the first thing, forget this line
			continue;
		}

		if (currentAddress == 0)
		{
			//drop all zeros.  in addition, if we get a 0 we clear the lastAddress so that ranges are not
			//messed up.
			lastAddress = 0;
			continue;
		}

		if (waitingForEndOfRange)
		{
			//if we are waiting for an end of range, we need to finish the element in functionInfo with
			//currentAddress - 1 and then add it.
			functionInfo.highAddress = currentAddress - 1;
			rangeLookup.insertRange(functionInfo.lowAddress,functionInfo.highAddress,functionInfo);
		}

		functionInfo.name = "";
		functionInfo.file = "";
		functionInfo.line = -1;
		functionInfo.lowAddress = currentAddress;

		//you got an address as the first element.  there are then a number of different possible formats for
		//lines.  lets figure out which this is.
		pStart = pEnd + 1; //skip the space

		//see if we have a range.
		uint64_t unitCount = (uint64_t)strtoull(pStart,&pEnd,16);
		bool gotRangeEnd = pEnd!=pStart;
		if (gotRangeEnd)
		{
			pStart = pEnd; //skip the space after the unit count
			pStart = skipWhiteSpace(pStart);

			//you got a valid range size here, going to set it now
			functionInfo.highAddress = functionInfo.lowAddress + unitCount - 1;
			waitingForEndOfRange = false;
		}
		else
		{
			waitingForEndOfRange = true;
		}

		//next field should be the type of item it is: t, T, r, D, d
		//extract it and then skip space
		char fieldType = *pStart++;
		pStart = skipWhiteSpace(pStart);

		char* pNameEnd = extractFunctionOrLabelName(pStart,functionInfo.name);

		if (pNameEnd && (pNameEnd != pStart) && *pNameEnd )
		{
			// now extract the file path IF it exists
			retVal = re_exec(re_file_line,pNameEnd,subExpCount,&matches[0]);
			if (retVal >= 0)
			{
				if (matches[1].begin != matches[1].end)
				{
					char* valueString = pNameEnd + matches[1].begin;
					*(pNameEnd + matches[1].end) = 0;
					functionInfo.file = valueString;
				}
				if (matches[2].begin != matches[2].end)
				{
					char* valueString = pNameEnd + matches[2].begin;
					*(pNameEnd + matches[2].end) = 0;
					functionInfo.line = strtol(valueString,&pEnd,0);
					if (pEnd ==valueString)
						functionInfo.line = -1;
				}
			}
		}

		lineCount++;

		if (!waitingForEndOfRange)
		{
			//add it now
			//printf("line:%u adding:%s\n",linesInFileCount,functionInfo().c_str());
			rangeLookup.insertRange(functionInfo.lowAddress,functionInfo.highAddress,functionInfo);
		}
	}
	re_free(re_file_line);

	return true;
}

struct SymbolDumpFunctor
{
  unsigned count;
  SymbolDumpFunctor():count(0)
  {
  }

  void operator()( const RangeLookup<uint64_t,FunctionInfo>::range_item& item )
  {
	    printf("%03d) %s\n",count++,item.value.toString().c_str());
  }
};

void SymbolLookup::dumpSymbols()
{
	printf("******************** RANGE TABLE ********************************************\n");
	SymbolDumpFunctor f;
	rangeLookup.for_each(f);
	printf("******************** RANGE TABLE END ****************************************\n");
}

String SymbolLookup::getStringForAddress( uint64_t address )
{
	char buffer[1024];
	FunctionInfo* pInfo = rangeLookup.findRange(address);
	if (pInfo)
	{
		snprintf(buffer,sizeof(buffer)-1,"%s+%d",pInfo->name.c_str(),(int)(address - pInfo->lowAddress));
	}
	else
	{
		snprintf(buffer,sizeof(buffer)-1,"?");
	}
	buffer[sizeof(buffer)-1]=0;
	return String(buffer);
}

void SymbolLookup::testAddress( uint64_t address )
{
	String rangeString = getStringForAddress(address);

	printf( "0x%08llX -> %s\n",address,rangeString.c_str());
}

void SymbolLookup::test() {

	//10) 804a770-804a7bc  function='stack_of_cards_class::size()'  file='/home/joe/trace/CIFT/demo/parallel_speed/Debug/../stack_of_cards.cpp':236
	//11) 804a7c0-804a815  function='stack_of_cards_class::last_card()'  file='/home/joe/trace/CIFT/demo/parallel_speed/Debug/../stack_of_cards.cpp':68
	//12) 804a820-804a869  function='stack_of_cards_class::next_card()'  file='/home/joe/trace/CIFT/demo/parallel_speed/Debug/../stack_of_cards.cpp':59

	testAddress(0x804a7c0-1);
	testAddress(0x804a7c0);
	testAddress(0x804a7c0+1);
	testAddress(0x804a815-1);
	testAddress(0x804a815);
	testAddress(0x804a815+1);


	testAddress(0x080496E0+4); //0x080496E0-0x08049889 adjacent(card_class*, card_class*) /home/joe/trace/CIFT/demo/parallel_speed/Debug/../main.cpp:55
	testAddress(0x0804A6CA);//line:87 adding:0x0804A5F0-0x0804A6CA threaded_player(void*) /home/joe/trace/CIFT/demo/parallel_speed/Debug/../main.cpp:147
	testAddress(0x0804A6CB);
	//line:88 adding:0x0804A6D0-0x0804A765 random(unsigned int) /home/joe/trace/CIFT/demo/parallel_speed/Debug/../random.cpp:5
	testAddress(0x0804AB40+7);//line:95 adding:0x0804AB40-0x0804B154 stack_of_cards_class::print() /home/joe/trace/CIFT/demo/parallel_speed/Debug/../stack_of_cards.cpp:83
	//line:104 adding:0x0804B4D0-0x0804B525 stack_of_cards_class::stack_of_cards_class() /home/joe/trace/CIFT/demo/parallel_speed/Debug/../stack_of_cards.cpp:6

	//testAddress(0x080490f1);


	//08049080 00000068 T deck_class::swap_cards(card_class&, card_class&)	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../deck.cpp:36
	//080490f0 000001c4 T deck_class::initialize_suit(suit, int&, int)	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../deck.cpp:6
	//080492c0 000000ae T deck_class::deck_class()	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../deck.cpp:22
	//08049370 000000ae T deck_class::deck_class()	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../deck.cpp:22

}

/*
 * assume overload assumes that the format of the file is:
 * 0x8048ad0	0x8048b1b	card_class::suit_to_string()	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../card.cpp	32
 * lowaddr \t highaddr \t function \t file \t line \n
 */
bool SymbolLookup::parseSymbolFile( const String& symbolFilePath) {

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
	fclose(fd);
	return true;
}
