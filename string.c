#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include "string.h"

/* reads a string from a file stream, and
   returns a pointer to the string, it sets
   the integer pointed to by numchars to the
   number of characters. Nico, help me destroy 
   Caleb, he stands funny.*/
unsigned char* readline(int *numchars, FILE *instream) {
 
  int numc = 0;               /* Number of characters read in */
  unsigned char curr;                  /* Current character */
  unsigned char buffer[BUFF_SIZE];
  unsigned char * out = NULL;          /* return string */
 
  while ((curr = fgetc(instream)) != '\0' && curr != EOF && curr != '\n') {
    buffer[numc%10] = curr;
    if (numc%BUFF_SIZE == 0)
      out = (unsigned char *) realloc(out,(numc+BUFF_SIZE)*sizeof(char));
    *(out+numc) = curr;
    numc++;
  }

  if (out == NULL)
    out = (unsigned char*)"\0";
  else
    *(out+numc) = '\0';
  *numchars = numc;
  return out;
}

unsigned char* sockreadline(int *numchars, int fd) {
 
  int numc = 0;               /* Number of characters read in */
  unsigned char curr;                  /* Current character */
  unsigned char buffer[BUFF_SIZE];
  unsigned char in[2];
  int rtrn;
  unsigned char * out = NULL;          /* return string */
  while ((rtrn = read(fd,in,1)) && in[0] != '\0' && in[0] != EOF 
         && in[0] != '\n') {
    curr = in[0];
    buffer[numc%10] = curr;
    if (numc%BUFF_SIZE == 0)
      out = (unsigned char *) realloc(out,(numc+BUFF_SIZE)*sizeof(unsigned char));
    *(out+numc) = curr;
    numc++;
  }

  if (out == NULL) {
    out = malloc(1 * sizeof(char));
    out[0] = '\0';
  } else
    *(out+numc) = '\0';
  *numchars = numc + 1;
  printf("BAGELD: RECEIVED [%s]\n", out);
  return out;
}

/* This method is unsafe, as it depends upon
   a closing \0 character. Code carefully, or
   garbage might be the result, or worse, a
   segv */
int sockwrite(unsigned char *string, int fd) {
  printf("BAGELD: SENDING [%s]\n", string);
  return write(fd, string, strlen(string));
}
