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
#ifndef CIFT_NM_SYMBOL_EXTRACTOR_H
#define CIFT_NM_SYMBOL_EXTRACTOR_H


#include <string>
#include <stdio.h>
#include <stdint.h>
#include "address-range-lookup.h"

using namespace std;

typedef string String;

/** a class that wraps all info for functions.  address range, name, file */
class FunctionInfo {
public:
	FunctionInfo()
		:lowAddress(0),highAddress(0),line(-1)
	{
	}
	FunctionInfo(uint64_t _lowAddress,
			     uint64_t _highAddress,
			     const String& _name, const String& _file, int _line)
	:lowAddress(_lowAddress),highAddress(_highAddress),name(_name),file(_file),line(_line)
	{
	}
	FunctionInfo(const FunctionInfo& o)
		:lowAddress(o.lowAddress),highAddress(o.highAddress),name(o.name),file(o.file),line(o.line)
	{
	}

	String toString() const
	{
		char buffer[1024];
		snprintf(buffer,sizeof(buffer)-1,"0x%08llX-0x%08llX %s %s:%d",lowAddress,highAddress,name.c_str(),file.c_str(),line);
		buffer[sizeof(buffer)-1]=0;
		return String(buffer);
	}

	String toOffsetString( uint64_t address )
	{
		char buffer[1024];
		snprintf(buffer,sizeof(buffer)-1,"%s+%d",name.c_str(),(int)(address - lowAddress));
		buffer[sizeof(buffer)-1]=0;
		return String(buffer);
	}

	//yeah, public members is bad form, sorry.
	uint64_t 	lowAddress;
	uint64_t 	highAddress;
	String 		name;
	String 		file;
	int    		line;
};

/**
* a utility class that reads in symbols for a given application and
* then allows looking up function names by address.  the utility
* uses the symbols dumped by the Linux "nm" utility function which is
* effectively ELF symtab
*/
class SymbolLookup
{
	public:

		SymbolLookup(void);
		~SymbolLookup(void);

		/** clear the current symbol mappings and get ready for symbols to be loaded.
		* this should be followed with 1 or more calls to loadSymbolsForExecutable
		*/
		bool init(void);

		/**
		* use nm utility to dump the exported symbols for the given executable and read these symbols
		* into an internal lookup data structure.
		* @param executablePath full path to the executable
		* @param addressBias optional bias to add to each extracted address
		*/
		bool loadSymbolsForExecutable( const String& executablePath, uint64_t addressBias=0 );

		/**
		* utility function that allows you to format a string for a given address.
		* the string will come with with "function+<offset from function start>" as the format.
		* if the address does not correspond to a known function, a question mark is returned
		*/
		String getStringForAddress( uint64_t address );

		void dumpSymbols(void);

		//TODO remove these
		void testAddress( uint64_t address );
		void test();

	protected:

		/** called to spawn nm to dump the symbols to a file
		* nm --demangle -a -l -S --numeric-sort ./parallel_speed > ./parallel_speed.nmsymbols
		*/
		bool generateNMSymbolFile( const String& executablePath );

		/**
		* utility function to parse a symbol dump created by the generateNMSymbolFile call.
		* format:
		* 08048ad0 0000004c T card_class::suit_to_string()	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../card.cpp:32
		*/
		bool parseNMSymbolFile( const String& symbolFilePath, uint64_t addressBias=0 );

		/*
		* old format overload. assumes that the format of the file is:
		* 0x8048ad0	0x8048b1b	card_class::suit_to_string()	/home/joe/trace/CIFT/demo/parallel_speed/Debug/../card.cpp	32
		* lowaddr \t highaddr \t function \t file \t line \n
		*/
		bool parseSymbolFile( const String& symbolFilePath);

	protected:
		RangeLookup<uint64_t,FunctionInfo> rangeLookup;
};

#endif
