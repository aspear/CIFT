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

int function3()
{
	someVariable++;
	return someVariable;
}
int function2()
{
	return function3();
}
int function1()
{
	int value = function2();
	value += function3();
	return value;
}


int main() {
	cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
	function1();
	return 0;
}
