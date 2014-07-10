/* Bageld - A system for the organization,
   storage, and retrieval of Bagel information

   John V. Kew
   Assignment 2
   CPEx317 w/ Dr. Nico
   Winter, 2002

   This program sets up a decision tree for the
   organization of bagel information. The
   program will use a database file in the local
   directory called "bagels.db" - If this file
   does not exist, it will create it so that 
   bagel information can be added.

   Files:
   bageld.c
   bageld.h
   string.c
   string.h

   Compilation: 
   cc -Wall -ansi -pedantic -o bageld *.c

   Usage:
   ./bageld

   Without a database, the program will first
   ask you for a bagel type. Then begin filling
   the database with Caleb bagels, Monkey bagels,
   and Toast bagels.

   All answers are of "yes", "no", [Bagel Name], or
   a question about a bagel.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include "bageld.h"
#include "string.h"

#define SUCCESS 0
#define ERROR   1
#define PORT    2002


int main(int argc, char *argv[]) {
  struct node *root = readFromFile("bagels.db");
  struct node *curr = root;
  struct node *right = NULL;
  struct node *parent = root;
  struct node *prev = NULL;
  int path;
  int inputLength;
  char * input;
  int finished = 0;

  /* Bageld net code */
  unsigned int cliLen, newSd, s;
  struct sockaddr_in cliAddr, servAddr;

  s = socket(AF_INET, SOCK_STREAM, 0);

  if(s<0) {
    perror("Cannot open socket ");
    return 1;
  }

  /* bind server port */
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(PORT);

  if(bind(s, (struct sockaddr *) &servAddr, sizeof(servAddr))<0) {
    perror("Cannot bind to port");
    return 1;
  }

  listen(s, 10); /* State a willingness to listen for
                     incomming connections */

  while (1) {
    printf("Awaiting new connections on port %d", PORT);
    cliLen = sizeof(cliAddr);
    newSd = accept(s, (struct sockaddr *) &cliAddr, &cliLen);
    if(newSd<0) {
      perror("Cannot accept connection ");
      return 1;
    }
    printf("Connect from %s\n", inet_ntoa(cliAddr.sin_addr));
    finished = 0;
    parent = root;
    curr = root;
    /* Creation of initial bagel in database */
    if (root->left == NULL && root->right == NULL
	&& root->questionOrAnswer == NULL) {
      sockwrite("No bagels in database, starting anew.\n",newSd);
      sockwrite("Give the name of a bagel: ",newSd);
      input = sockreadline(&inputLength, newSd);
      root->questionOrAnswer = input;
      sockwrite("Thank you.\n",newSd);
    }
    
    
    while(!finished && errno >= 0) {
      if (curr->left == NULL || curr->right == NULL) { /* At answer node */
	sockwrite("I guess you have a ", newSd);
	sockwrite(curr->questionOrAnswer, newSd);
	sockwrite(" bagel.\n",newSd);
	sockwrite("Is this correct?[yes,no] > ", newSd);
	input = sockreadline(&inputLength, newSd);
	if (!strcmp("yes",input)) { /* Congrats message */
	  free(input);
	  finished = !finished;
          printf("Closing connection...");
	  close(newSd);
	  printf("done\n");
	} else if (!strcmp("no",input)) { /* Create new node */
	  free(input);
	  sockwrite("No? Then what type of bagel is it? > ", newSd);
	  input = sockreadline(&inputLength, newSd);
	  right = createnode();
	  right->questionOrAnswer = input;
	  right->question = 0;
	  right->right = NULL;
	  right->left = NULL;
	  sockwrite("\nWhat is a good yes/no question about ", newSd);
	  sockwrite(input, newSd);
	  sockwrite(" bagels that would distinguish them from ", newSd);
	  sockwrite(curr->questionOrAnswer, newSd);
	  sockwrite(" bagels? > ", newSd);
	  input = sockreadline(&inputLength, newSd);
	  parent = createnode();
	  parent->questionOrAnswer = input;
	  parent->question = 1;
	  parent->right = right;
	  parent->left = curr;
	  if (prev != NULL && path == RIGHT)
	    prev->right = parent;
	  else if (prev != NULL && path == LEFT)
	    prev->left = parent;
	  else if (root == parent->left)
	    root = parent;
	  curr = root;
	  sockwrite("\n\nI'm sure I'll guess it now.", newSd);
	  finished = !finished;
	  printf("Closing connection...");
	  close(newSd);
	  printf("done\n");
	} else {
	  free(input);
	  sockwrite("\nPlease answer \"yes\" or \"no\".\n", newSd);
	}
	
      } else { /* At question node */
	sockwrite("\n", newSd);
	sockwrite(curr->questionOrAnswer, newSd);
	sockwrite(" > ", newSd);
	input = sockreadline(&inputLength, newSd);
	printf("Input: %s\n", input);
	if (!strcmp("yes",input)) { /* Goto right node */
	  prev = curr;
	  path = RIGHT;
	  curr = curr->right;
	} else if (!strcmp("no",input)) { /* Goto left node */
	  prev = curr;
	  path = LEFT;
	  curr = curr->left;
	} else
	  sockwrite("\nPlease answer \"yes\" or \"no\".\n", newSd);
	free(input);
      }
    }
    printf("Saving data...");
    saveToFile("bagels.db", root);
    printf("done\n");
    errno = 0;
  }

  destroynode(root);
  return 0;
}
