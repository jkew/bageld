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
#include <gssapi.h>
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
    printf("Awaiting new connections on port %d\n", PORT);
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

    int kerberosEnabled = 1;
    if (kerberosEnabled) {
      if (getenv("KRB5_KTNAME") == NULL) {
	putenv("KRB5_KTNAME=bageld.keytab");
      }
      sockwrite("Please send your kerberos service ticket: ", newSd);
      input = sockreadline(&inputLength, newSd);
      gss_buffer_desc gbuf;
      gbuf.length = inputLength;
      gbuf.value = input;
      printf("KERBEROS: Recieved a kerberos service ticket of length %d [%s]\n", gbuf.length, gbuf.value);
      gss_ctx_id_t ctx = GSS_C_NO_CONTEXT;
      OM_uint32 maj_stat, min_stat, gflags, lmin_s;
      gss_buffer_desc outbuf;
      gss_name_t name;
      int authorized = 0;
      do {
	maj_stat = gss_accept_sec_context(&min_stat,
					  &ctx,
					  GSS_C_NO_CREDENTIAL,
					  &gbuf,
					  GSS_C_NO_CHANNEL_BINDINGS,
					  &name,
					  NULL,
					  &outbuf,
					  &gflags,
					  NULL,
					  NULL);
	printf("KERBEROS: gss_accept_sec_context major: %d, minor: %d, outlen: %u, outflags: %x\n", maj_stat, min_stat, (unsigned int)outbuf.length, gflags);
        switch (maj_stat) {
	case GSS_S_CONTINUE_NEEDED:
	  if (outbuf.length != 0) {
            // Additional information needs to be sent to the client
	    printf("KERBEROS: sending GSS response token of length %u\n", (unsigned int) outbuf.length);
	    sockwrite("Here is something I think you need to know: [%s]", outbuf.value);
	    gss_release_buffer(&lmin_s, &outbuf);
	  }
	  break;
	case GSS_S_COMPLETE:
	  authorized = 1;
	  break;
	default:
	  gss_delete_sec_context(&lmin_s, &ctx, GSS_C_NO_BUFFER);
	  gss_buffer_desc gmsg;
	  OM_uint32   lmin_s, msg_ctx;
	  char        msg_major[128], msg_minor[128];
	  msg_ctx = 0;
          gss_display_status(&lmin_s, maj_stat, GSS_C_GSS_CODE, GSS_C_NO_OID, &msg_ctx, &gmsg);
          strlcpy(msg_major, gmsg.value, sizeof(msg_major));
          gss_release_buffer(&lmin_s, &gmsg);
          msg_ctx = 0;
          gss_display_status(&lmin_s, min_stat, GSS_C_MECH_CODE, GSS_C_NO_OID, &msg_ctx, &gmsg);
          strlcpy(msg_minor, gmsg.value, sizeof(msg_minor));
          gss_release_buffer(&lmin_s, &gmsg);
	  printf("KERBEROS: accepting GSS security context failed: %s: %s\n", msg_major, msg_minor);
	}
      } while (maj_stat == GSS_S_CONTINUE_NEEDED);
      

      if (authorized) {
        sockwrite("Good news; you are not an intruder!", newSd);
	free(input);
      } else {
	sockwrite("Invalid kerberos ticket. You are denied access!", newSd);
        close(newSd);
	free(input);
        continue;
      }
    }

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
