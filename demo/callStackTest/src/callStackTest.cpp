//============================================================================
// Name        : callStackTest.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
using namespace std;

volatile int someVariable = 0;

//a hack to introduce a little bit of delay so that the timestamps are not too compressed
volatile unsigned delayVar=0;
#define DELAY() { unsigned i;for (i=0;i<1000000;++i) {delayVar++;}  }

int function4( int param )
{
	int returnValue;
	DELAY();
	returnValue = param + 1;
	DELAY();
	return returnValue;
}
int function3( int param )
{
	int returnValue;
	DELAY();
	returnValue = function4(param) + 1;
	DELAY();
	return returnValue;
}
int function2( int param )
{
	int returnValue;
	DELAY();
	returnValue = function3(param) + 1;
	DELAY();
	return returnValue;
}
int function1( int param )
{
	int returnValue;
	DELAY();
	returnValue = function2(param) + 1;
	DELAY();
	return returnValue;
}


int main() {
	cout << "running call stack test" << endl; // prints !!!Hello World!!!
	function1(0);
	cout << "call stack test complete" << endl; // prints !!!Hello World!!!
	return 0;
}
