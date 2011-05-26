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
#include <regex.h> // POSIX regex support

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
	int lineCount = 0;
	int linesInFileCount=0;

	FILE* fd = fopen(symbolFilePath.c_str(),"rt");
	if (!fd)
	{
		return false;
	}

	char buffer[1024];
	regex_t re;
	regmatch_t pmatch[20];

	// lines are formatted like this:
	// 08048ad0 0000004c T card_class::suit_to_string()	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../card.cpp:32

	if(regcomp(&re,"^([0-9A-Fa-f]+)[ \t]+([0-9A-Fa-f]+)[ \t]+T[ \t]+([^\\)]+)\\)[ \t]+([a-zA-Z0-9/-_\\.]+):([0-9]+)", REG_EXTENDED|REG_NOSUB) != 0)
	{
		return false;
	}

	String functionName;
	String fileName;

	char* line;
	while( (line=fgets(buffer,sizeof(buffer)-1,fd))!= 0)
	{
		uint64_t startAddress=0;
		uint64_t endAddress=0;
		int lineNumber=-1;
		char* pEnd=0;
		int retVal;

		linesInFileCount++;

		//run the regexec to execute a match on the line
		retVal = regexec(&re,line,20,pmatch,REG_EXTENDED);
		if (retVal != 0)
		{
			char errorBuffer[1024];
			memset(errorBuffer,0,sizeof(errorBuffer));
			regerror(retVal,&re,errorBuffer,sizeof(errorBuffer)-1);
			fprintf(stderr,"ERROR running regexec '%s'\n",errorBuffer);
			continue;
		}

		if (pmatch[0].rm_so != -1)
		{
			char* valueString = line + pmatch[0].rm_so;
			*(line + pmatch[0].rm_eo) = 0;
			startAddress = (uint64_t)strtoull(valueString,&pEnd,0);
			if (pEnd ==valueString)
				continue;
			startAddress += addressBias;
		}
		if (pmatch[1].rm_so != -1)
		{
			char* valueString = line + pmatch[1].rm_so;
			*(line + pmatch[1].rm_eo) = 0;
			uint64_t unitCount = (uint64_t)strtoull(valueString,&pEnd,0);
			if (pEnd ==valueString)
				continue;
			endAddress = startAddress + unitCount - 1;
		}
		if (pmatch[2].rm_so != -1)
		{
			char* valueString = line + pmatch[2].rm_so;
			*(line + pmatch[2].rm_eo) = 0;
			functionName = valueString;
		}
		else
		{
			functionName = "";
		}
		if (pmatch[3].rm_so != -1)
		{
			char* valueString = line + pmatch[3].rm_so;
			*(line + pmatch[3].rm_eo) = 0;
			fileName = valueString;
		}
		else
		{
			fileName = "";
		}
		if (pmatch[4].rm_so != -1)
		{
			char* valueString = line + pmatch[4].rm_so;
			*(line + pmatch[4].rm_eo) = 0;
			lineNumber = strtol(valueString,&pEnd,0);
			if (pEnd ==valueString)
				lineNumber = -1;
		}

		lineCount++;

		//todo move construction of this out of this loop
		FunctionInfo info(startAddress,endAddress,functionName,fileName,lineNumber);

		rangeLookup.insertRange(startAddress,endAddress,info);
	}
	regfree(&re);
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

	//testAddress(0x080490f1);


	//08049080 00000068 T deck_class::swap_cards(card_class&, card_class&)	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../deck.cpp:36
	//080490f0 000001c4 T deck_class::initialize_suit(suit, int&, int)	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../deck.cpp:6
	//080492c0 000000ae T deck_class::deck_class()	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../deck.cpp:22
	//08049370 000000ae T deck_class::deck_class()	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../deck.cpp:22

}

