%token	<string_val> WORD

%token 	NOTOKEN GREAT NEWLINE GREATAND DOUBLEGREAT LESS PIPE BG DOUBLEGREATAND

%union	{
    char *string_val;
}

%{
extern "C"

{
	int yylex();
	void yyerror (char const *s);
}
#define yylex yylex
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include "command.h"
%}

%%
goal:
	commands
	;

commands:
	command
	| commands  command
	;

command: simple_command io_redirect_bg
    | complex_grammer
    ;

complex_grammer:
    command_and_args separator_list io_redirect io_redirect_bg NEWLINE{
    printf("   Yacc: Execute complex command\n");
    Command::_currentCommand.execute();
    }
    | command_and_args separator_list io_redirect NEWLINE {
    printf("   Yacc: Execute complex command\n");
    Command::_currentCommand.execute();
    }
    ;
simple_command:
	command_and_args NEWLINE {
		printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE
	| error NEWLINE { yyerrok; }
	;

command_and_args:
	command_word arg_list {
		Command::_currentCommand.insertSimpleCommand(Command::_currentSimpleCommand);
	}
	;

arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD {
		printf("   Yacc: insert argument \"%s\"\n", $1);
		Command::_currentSimpleCommand->insertArgument($1);
	}
	;

command_word:
	WORD {
		printf("   Yacc: insert command \"%s\"\n", $1);
		Command::_currentSimpleCommand = new SimpleCommand();
		Command::_currentSimpleCommand->insertArgument($1);
	}
	;
separator_list:
    separator_list separator
    | /* can be empty */
    ;
separator:
	PIPE command_and_args{
	}
	| /* can be empty */
	;

io_redirect_bg:
	io_list BG {
		// Fork the process and run the command in the child process.
		pid_t pid = fork();
		if (pid == 0) {
			// Child process.
			Command::_currentCommand.execute();
			exit(0);
		}
	}
	| /* can be empty */
	;
io_list:
	io_list io_redirect
	| /* can be empty */
	;



io_redirect:
    DOUBLEGREAT WORD {
        printf("   Yacc: redirect stdout and stderr to %s\n", $2);
        Command::_currentCommand._outFile = $2;
    }
    |   GREAT WORD {
        printf("   Yacc: redirect stdout to %s\n", $2);
        Command::_currentCommand._outFile = $2;
    }
    |   LESS WORD {
            Command::_currentCommand._inputFile = $2;
        }
    |   DOUBLEGREATAND WORD {
        Command::_currentCommand._outFile = $2;
        Command::_currentCommand._errFile = $2;
    }
    |   GREATAND WORD {
        Command::_currentCommand._outFile = $2;
        Command::_currentCommand._errFile = $2;
    }
    |   BG{
         printf("   Yacc: your program run in background\n");
         Command::_currentCommand._background = true;
        }
    | /* can be empty */
    ;

%%

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
