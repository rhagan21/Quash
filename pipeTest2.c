/**
 * @file pipeTest2.c
 *
 * Second pipe test
 */

#include <stdio.h>
int main(int argc,char *argv[])
{
	if(argc==1){
            printf("Successfully communicated with pipe reader");
        } else if(argc>2){
            printf("Too many arguments supplied.\n");
        } else {
            printf("One argument expected.");
        }
        return 0;
}
