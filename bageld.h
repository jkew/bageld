#ifndef BAGELD_H
#define BAGELD_H 1
#include <stdlib.h>
#include "string.h"
#define RIGHT 1
#define LEFT 0

/* Defines a node in the decision tree.
   This node can represent either a question or
   an answer. The question is, can Caleb run fast
   enough? The answer, not with his pathetic gait.
   Help me Dr. Nico, help me rid this earth of Caleb
   and his infuriating stance. 
*/
struct node {
  char * questionOrAnswer; /* The question to be asked, or the answer
                              to be found */
  int question;            /* boolean - is this really a question, 
                              or a bad excuse for DESTROYING 
                              CALEB?!!! */
  struct node *left;       
  struct node *right;
};

/* Creates an empty node. You better make
   sure to DESTROY this node (and Caleb)
   when you are done. */
struct node * createnode() {
  struct node * n;
  n = (struct node *) malloc(sizeof(*n));
  n->questionOrAnswer = NULL;
  n->question = 0;
  n->left = NULL;
  n->right = NULL;
  return n;
}

/* This destroys a tree of nodes given a
   root node. It is recursive. I wish I
   could write a function to destroy Caleb.
   Unfortunatly that's just silly, but a 
   large anvil will do just fine */
void destroynode(struct node *n) {
  if (n == NULL)
    return;
  if (n->questionOrAnswer != NULL)
    free(n->questionOrAnswer);
  destroynode(n->left);
  destroynode(n->right);
  if (n != NULL)
    free(n);
}

/* This function writes a tree of nodes to a
   filestream. Unfortunatly, passing in
   Caleb->brain->memory->lifesupportConstants->
   heartbeatInterval as a filestream does not
   work as expected. The method tranverses the
   tree and does some other stuff so reading the
   tree back in is not a problem. */
void writetree(FILE *file, struct node * node) {
  fprintf(file, "%s:%d\n",node->questionOrAnswer,node->question);
  if (node->left != NULL && node->right != NULL) {
    writetree(file, node->right);
    writetree(file, node->left);
  }
}


/* This method reads in a tree of nodes written
   tweleve munks and their jolly friend. Oh lord,
   I just remembered that I need to add a bunch of
   emails to the lug list. I also have some pending
   emails on my lug account I haven't responded to.
   Darn. Perhaps while I am logged onto the lug
   server I'll send a threatening email to Caleb. */
struct node * readtree(FILE *file) {
  int inputLength;
  char * input = NULL;
  char *info[2];
  struct node * n;
  if (file == NULL)
    return createnode();
  input = readline(&inputLength, file);
  info[0] = (char *) strtok (input, ":");
  info[1] = (char *) strtok (NULL, ":");

  n = createnode();
  n->questionOrAnswer = info[0];
  if (info[1][0] == '0') {
    n->question = 0;
    n->right = NULL;
    n->left = NULL;
  } else if (info[1][0] == '1') {
    n->question = 1;
    n->right = readtree(file);
    n->left = readtree(file);
  }

  return n;
}

/* Takes a filename, opens it, and begins
   reading in the tree of nodes. Yay. Dang,
   Caleb just said something in the irc
   channel. Every day that kid stands, sits,
   or walks funny is a another day closer to
   the apocolypse. I don't know how to spell
   that word. I suppose I could use the emacs
   spellchecker, but I'm not in the mood to
   press control or Meta right now. Phooey.*/
struct node * readFromFile(char * filename) {
  FILE * read = fopen(filename, "r");
  struct node * root;
  if (read == NULL)
    return createnode();
  root = readtree(read);
  fclose(read);
  return root;
}

/* Saves the decision tree to a file. No really,
   this method will ACTUALLY OPEN AND WRITE TO 
   A FILE ON UNIX!!! Amazing. I've never seen
   anything like it in my life. Please kill Caleb.*/
void saveToFile(char * filename, struct node * root) {
  FILE * write = fopen(filename, "w");
  writetree(write, root);
  fclose(write); 
}


#endif
