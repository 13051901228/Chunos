#include <stdio.h>

int func(int a,int b)
{
	return a+b;
}

int main(int argc,char **argv)
{
	int a =5;
	int b = 6;
	int c;

	c = func(a,b);

	printf("%d\n",c);

	return 0;
}
