#include <time.h>
#include <stdlib.h>
#include <stdio.h>

int random(unsigned int max)
{
	static bool initial = true;

	if (initial) {
		initial = false;
		srand((unsigned int) time(NULL));
	}

	return (int) ((rand() - 1) /(float) RAND_MAX * max);
}

