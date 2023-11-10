%token	<string_val> WORD

%token 	NOTOKEN GREAT NEWLINE ERR DOUBLEGREAT LESS PIPE BG

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

command: simple_command

    | complex_command

    ;



simple_command:
	command_and_args error_opt NEWLINE {
		printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE
	| error NEWLINE { yyerrok; }
	;

complex_command:
    command_and_args PIPE io_list io_redirect_bg NEWLINE {
        printf("   Yacc: Execute command\n");
        Command::_currentCommand.execute();
    }
    | command_and_args io_list NEWLINE {
        Command::_currentCommand.execute();
    }
    |
    ;

command_and_args:
	command_word arg_list {
		Command::_currentCommand.insertSimpleCommand(Command::_currentSimpleCommand);
	}
	|
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


io_redirect_bg:
	  BG {
        printf("   Yacc: your program run in background\n");
        Command::_currentCommand._background = 1;
    }
	;

io_list:
	io_list io_redirect
	| /* can be empty */
	;



io_redirect:
    DOUBLEGREAT WORD {
        printf("   Yacc: redirect stdout and stderr to %s\n", $2);
        Command::_currentCommand._append = 1;
        Command::_currentCommand._outFile = $2;
    }
    |   GREAT WORD {
        printf("   Yacc: redirect stdout to %s\n", $2);
        Command::_currentCommand._outFile = $2;
    }
    |   LESS WORD {
            Command::_currentCommand._inputFile = $2;
        }
    ;

error_opt:
	ERR WORD{
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._errFile = $2;
	}
	|
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
