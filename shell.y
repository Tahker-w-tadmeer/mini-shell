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
	| commands command
	| commands PIPE command
	;

command: simple_command

    | complex_command

    ;

simple_command:
	command_and_args error_opt NEWLINE {
		Command::_currentCommand.execute();
	}
    | command_and_args BG error_opt NEWLINE {
        Command::_currentCommand.execute();
    }
	| NEWLINE
	| error NEWLINE { yyerrok; }
	;

complex_command:
    command_and_args PIPE io_list io_redirect_bg NEWLINE {
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
    command_and_args PIPE command_word arg_list {
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
		Command::_currentSimpleCommand->insertArgument($1);
	}
	;


command_word:
	WORD {
		Command::_currentSimpleCommand = new SimpleCommand();
		Command::_currentSimpleCommand->insertArgument($1);
	}
	;


io_redirect_bg:
	  BG {
        Command::_currentCommand._background = 1;
    }
	;

io_list:
	io_list io_redirect
	| /* can be empty */
	;



io_redirect:
    DOUBLEGREAT WORD {
        Command::_currentCommand._append = 1;
        Command::_currentCommand._outFile = $2;
    }
    |   GREAT WORD {
        Command::_currentCommand._outFile = $2;
    }
    |   LESS WORD {
            Command::_currentCommand._inputFile = $2;
        }
    ;

error_opt:
	ERR WORD{
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
