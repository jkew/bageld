bageld
======

Bageld - A system for the organization,
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

# Files:
bageld.c
bageld.h
string.c
string.h

# Compilation:
cc -Wall -ansi -pedantic -o bageld *.c

Usage:
./bageld

Without a database, the program will first
ask you for a bagel type. Then begin filling
the database with Caleb bagels, Monkey bagels,
and Toast bagels.

All answers are of "yes", "no", [Bagel Name], or
a question about a bagel.