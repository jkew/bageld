#ifndef STRING_H
#define STRING_H 1
#include <stdio.h>
#include <string.h>
#define BUFF_SIZE 10

/* reads a string from a file stream, and
   returns a pointer to the string, it sets
   the integer pointed to by numchars to the
   number of characters. Nico, help me destroy 
   Caleb, he stands funny.*/
char * readline(int *numchars, FILE *instream);
char* sockreadline(int *numchars, int fd);
int sockwrite(char *string, int fd);
#endif
