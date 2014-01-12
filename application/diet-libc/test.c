#include <stdio.h>
#include <stdlib.h>

void delay(int n)
{
	while (n--);
}

int main(int argc, char **argv)
{
	while(1) {
		printf("I Love This Game ! %d %d %d\n", 123, 456, 789);
		delay(600000);
	}
	return 0;
}
