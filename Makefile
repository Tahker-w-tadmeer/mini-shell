
#Use GNU compiler
cc = gcc -g
CC = g++ -g

all: shell

lex.yy.o: shell.l 
	lex shell.l
	$(cc) -c lex.yy.c

y.tab.o: shell.y
	yacc -d shell.y
	$(CC) -c y.tab.c

command.o: command.cc
	$(CC) -c command.cc

ls_output.o: commands/ls_output.cc
	$(CC) -c commands/ls_output.cc

shell: y.tab.o lex.yy.o command.o ls_output.o
	$(CC) -o shell lex.yy.o y.tab.o ls_output.o command.o -ll

clean:
	rm -f lex.yy.c y.tab.c y.tab.h shell *.o

