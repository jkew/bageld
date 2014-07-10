bageld
======

Bageld - A high-performance, network enabled 
system for the organization, storage, and 
retrieval of Bagel information

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

## Files:
bageld.c
bageld.h
string.c
string.h

## Compilation:
cc -Wall -ansi -pedantic -o bageld *.c

## Usage:
```
./bageld
Awaiting new connections on port 2002
```

```
telnet localhost 2002
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
Does it swim? > yes
Does it have feathers? > no
Is it the coolest damn bagel you've ever seen in your life? > yes
Does it stink? > no
I guess you have a Bob bagel.
Is this correct?[yes,no] > no
No? Then what type of bagel is it? > fish stick bagel
What is a good yes/no question about fish stick bagel bagels that would distinguish them from Bob bagels? > Is the bagel flakey and delicious?

I'm sure I'll guess it now.Connection closed by foreign host.
```


Without a database, the program will first
ask you for a bagel type. Then begin filling
the database with Caleb bagels, Monkey bagels,
and Toast bagels.

All answers are of "yes", "no", [Bagel Name], or
a question about a bagel.