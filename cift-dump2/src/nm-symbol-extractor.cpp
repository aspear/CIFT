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
#include <string.h>
#include "re2/regexp.h"

using namespace std;

NMSymbolExtractor::NMSymbolExtractor()
{
}
NMSymbolExtractor::~NMSymbolExtractor()
{
}

bool NMSymbolExtractor::init(){

	rangeLookup.clear();
	return true;
}

bool NMSymbolExtractor::loadSymbolsForExecutable( String executablePath, uint64_t addressBias/*=0*/ )
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
bool NMSymbolExtractor::generateNMSymbolFile( String executablePath ) {
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

/*
 * assume overload assumes that the format of the file is:
 * 0x8048ad0	0x8048b1b	card_class::suit_to_string()	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../card.cpp	32
 * lowaddr \t highaddr \t function \t file \t line \n
 */
bool NMSymbolExtractor::parseSymbolFile(String symbolFilePath) {

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

bool NMSymbolExtractor::parseNMSymbolFile(String symbolFilePath, uint64_t addressBias/*=0*/ )
{
	int retVal;
	int lineCount = 0;
	int linesInFileCount=0;

	FILE* fd = fopen(symbolFilePath.c_str(),"rt");
	if (!fd)
	{
		return false;
	}

	char buffer[1024];
	regexp* re=0;

	//this pattern works in java but fails with stock POSIX regcomp,regexec.  I punted and used a library that
	//I know works
	// lines are formatted like this:
	// 08048ad0 0000004c T card_class::suit_to_string()	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../card.cpp:32
	const char* linePattern = "^([0-9A-Fa-f]+)[ \t]+([0-9A-Fa-f]+)[ \t]+T[ \t]+([^\\)]+)\\)[ \t]+([a-zA-Z0-9/-_\\.]+):([0-9]+)";

	retVal =  re_comp(&re,linePattern);
	if (retVal< 0)
	{
		fprintf(stderr,"ERROR failed to compile regex '%s'\n",linePattern);
		return false;
	}

	FunctionInfo functionInfo;

	unsigned subExpCount = re_nsubexp(re);

	regmatch* matches = new regmatch[subExpCount];

	char* line;
	while( (line=fgets(buffer,sizeof(buffer)-1,fd))!= 0)
	{
		char* pEnd=0;

		linesInFileCount++;

		//execute a match on this line
		retVal = re_exec(re,line,subExpCount,&matches[0]);

		if (retVal < 1)
		{
			//failed to match this line
			continue;
		}

		if (matches[1].begin != matches[1].end)
		{
			char* valueString = line + matches[1].begin;
			*(line + matches[1].end) = 0;
			functionInfo.lowAddress = (uint64_t)strtoull(valueString,&pEnd,16);
			if (pEnd ==valueString)
				continue;
			functionInfo.lowAddress += addressBias;
		}
		else
		{
			//if you don't get the start address then you have nothing.
			continue;
		}
		if (matches[2].begin != matches[2].end)
		{
			char* valueString = line + matches[2].begin;
			*(line + matches[2].end) = 0;
			uint64_t unitCount = (uint64_t)strtoull(valueString,&pEnd,16);
			if (pEnd ==valueString)
				continue;
			functionInfo.highAddress = functionInfo.lowAddress + unitCount - 1;
		}
		if (matches[3].begin != matches[3].end)
		{
			char* valueString = line + matches[3].begin;
			*(line + matches[3].end) = 0;
			functionInfo.name = valueString;
			functionInfo.name += ")";
		}
		else
		{
			functionInfo.name = "";
		}
		if (matches[4].begin != matches[4].end)
		{
			char* valueString = line + matches[4].begin;
			*(line + matches[4].end) = 0;
			functionInfo.file = valueString;
		}
		else
		{
			functionInfo.file = "";
		}
		if (matches[5].begin != matches[5].end)
		{
			char* valueString = line + matches[5].begin;
			*(line + matches[5].end) = 0;
			functionInfo.line = strtol(valueString,&pEnd,0);
			if (pEnd ==valueString)
				functionInfo.line = -1;
		}
		else
		{
			functionInfo.line = -1;
		}

		lineCount++;

		//printf("line:%u adding:%s\n",linesInFileCount,functionInfo().c_str());

		rangeLookup.insertRange(functionInfo.lowAddress,functionInfo.highAddress,functionInfo);
	}
	re_free(re);

	rangeLookup.dump();

	return true;
}


String NMSymbolExtractor::getStringForAddress( uint64_t address )
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

void NMSymbolExtractor::testAddress( uint64_t address )
{
	String rangeString = getStringForAddress(address);

	printf( "0x%08llX -> %s\n",address,rangeString.c_str());
}

void NMSymbolExtractor::test() {

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

