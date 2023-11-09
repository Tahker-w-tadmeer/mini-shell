#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <csignal>
#include <iostream>
#include <string>
#include <sys/fcntl.h>

#include "commands/ls_output.h"

#include "command.h"

SimpleCommand::SimpleCommand() {
    // Create available space for 5 arguments
    _numberOfAvailableArguments = 5;
    _numberOfArguments = 0;
    _arguments = (char **) malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument) {
    if (_numberOfAvailableArguments == _numberOfArguments + 1) {
        // Double the available space
        _numberOfAvailableArguments *= 2;
        _arguments = (char **) realloc(_arguments,
                                       _numberOfAvailableArguments * sizeof(char *));
    }

    _arguments[_numberOfArguments] = argument;

    // Add NULL argument at the end
    _arguments[_numberOfArguments + 1] = nullptr;

    _numberOfArguments++;
}

Command::Command() {
    // Create available space for one simple command
    _numberOfAvailableSimpleCommands = 1;
    _simpleCommands = (SimpleCommand **)
            malloc(_numberOfSimpleCommands * sizeof(SimpleCommand *));

    _availableCommands = (std::string *) malloc(_numberOfSimpleCommands * sizeof(std::string));

    _numberOfSimpleCommands = 0;
    _outFile = nullptr;
    _inputFile = nullptr;
    _errFile = nullptr;
    _background = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand) {
    if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands) {
        _numberOfAvailableSimpleCommands *= 2;
        _simpleCommands = (SimpleCommand **) realloc(_simpleCommands,
                                                     _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
    }

    _simpleCommands[_numberOfSimpleCommands] = simpleCommand;

    _numberOfSimpleCommands++;
}

void Command::clear() {
    for (int i = 0; i < _numberOfSimpleCommands; i++) {
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++) {
            free(_simpleCommands[i]->_arguments[j]);
        }

        free(_simpleCommands[i]->_arguments);
        free(_simpleCommands[i]);
    }

    if (_outFile) {
        free(_outFile);
    }

    if (_inputFile) {
        free(_inputFile);
    }

    if (_errFile) {
        free(_errFile);
    }

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
}

void Command::print() {
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    for (int i = 0; i < _numberOfSimpleCommands; i++) {
        printf("  %-3d ", i);
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++) {
            printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
        }
    }

    printf("\n\n");
    printf("  Output       Input        Error        Background\n");
    printf("  ------------ ------------ ------------ ------------\n");
    printf("  %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
           _inputFile ? _inputFile : "default", _errFile ? _errFile : "default",
           _background ? "YES" : "NO");
    printf("\n\n");

}

void Command::execute() {
    printf("%s\n", _currentCommand._errFile);
    printf("%s\n", _currentCommand._inputFile);
    printf("%s\n", _currentCommand._outFile);
    printf("%d\n", _currentCommand._background);
    // Don't do anything if there are no simple commands
    if (_numberOfSimpleCommands == 0) {
        prompt();
        return;
    }


    // Print contents of Command data structure
     print();

//dup returns a new file descriptor that is a copy of the file descriptor passed as argument;
    int default_in = dup(0);//0 is the file descriptor for stdin
    int default_out = dup(1);//1 is the file descriptor for stdout
    int default_err = dup(2);//2 is the file descriptor for stderr
    int fdin;
    int fdout;
    int fderr;
    int pipes[2];
    //pipe command to pass data between processes it returns two file descriptors also we check it is successful if it returns 0 and error if -1
    if (pipe(pipes) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < _numberOfSimpleCommands; ++i) {
        if (strcasecmp(_simpleCommands[i]->_arguments[0], "exit") == 0) {
            printf("Good bye!!\n");
            exit(1);
        }

        if (_inputFile != nullptr) {
            fdin = open(_inputFile, O_RDONLY);
            if (fdin < 0) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(fdin, 0);
            close(fdin);
        } else {
            dup2(default_in, 0);
        }
        if (i == _numberOfSimpleCommands - 1) {
            if (_outFile != nullptr) {
                fdout = open(_outFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fdout < 0) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fdout, 1);
                close(fdout);
            } else {
                dup2(default_out, 1);
            }
            if (_errFile != nullptr) {
                fderr = open(_errFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fderr < 0) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fderr, 2);
                close(fderr);
            } else {
                dup2(default_err, 2);
            }
        } else {
            dup2(pipes[1], 1);
            close(pipes[1]);
            dup2(pipes[0], 0);
            close(pipes[0]);
        }
        int pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if(pid==0){
            if (_outFile!=nullptr)
                close(fdout);
            if (_errFile!=nullptr)
                close(fderr);
            if (_inputFile!=nullptr)
                close(fdin);

        }
        close(default_in);
        close(default_out);
        close(default_err);
        execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    dup2(default_in, 0);
    dup2(default_out, 1);
    dup2(default_err, 2);
    close(default_in);
    close(default_out);
    close(default_err);
    // Clear to prepare for next command
    clear();

    // Print new prompt
    prompt();
}

// Shell implementation

void Command::prompt() {
    printf("shell> ");
    fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int yyparse();

void defineCommand(std::string args[], int argsNumber = 0) {
    SimpleCommand *command = new SimpleCommand();
    for (int i = 0; i < argsNumber; i++) {
        char *char_arr = new char[args[i].length() + 1];
        strcpy(char_arr, args[i].c_str());
        command->insertArgument(char_arr);
    }

    Command::_currentCommand.insertSimpleCommand(command);
}

void defineCommands() {
    std::string args[] = {"ls"};
    defineCommand(args);

    std::string args1[] = {"cat"};
    defineCommand(args1);
}

int main() {
    defineCommands();

    Command::_currentCommand.prompt();
    yyparse();
    return 0;
}