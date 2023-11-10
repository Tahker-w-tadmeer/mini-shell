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

#include "command.h"

char working_dir[256];

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

char* getFullPath(char* file) {
    char sep = '/';
    char* fullpath = new char[sizeof (file) + sizeof (working_dir) + 1];
    if(file && file[0] != sep) {
        strcpy(fullpath, working_dir);
        strcat(fullpath, &sep);

        strcat(fullpath, file);
    }

    return fullpath;
}

void Command::finishExecuting(int default_in, int default_out, int default_err) {
    dup2(default_in, 0);
    dup2(default_out, 1);
    dup2(default_err, 2);
    close(default_in);
    close(default_out);
    close(default_err);

    if (_background == 0) {
        waitpid(-1, nullptr, 0);
    }
    // Clear to prepare for next command
    clear();

    // Print new prompt
    prompt();
}

void Command::execute() {
    if (_numberOfSimpleCommands == 0) {
        prompt();
        return;
    }

    int default_in = dup(0); // 0 is the file descriptor for stdin
    int default_out = dup(1); // 1 is the file descriptor for stdout
    int default_err = dup(2); // 2 is the file descriptor for stderr

    //make exit function to say goodbye
    if (strcasecmp(_currentSimpleCommand->_arguments[0], "exit") == 0) {
        printf("Goodbye\n");
        exit(1);
    }
    //make cd function to change directory
    if (strcasecmp(_currentSimpleCommand->_arguments[0], "cd") == 0) {
        if (_currentSimpleCommand->_arguments[1] == nullptr) {
            chdir(getenv("HOME"));
        } else {
            chdir(_currentSimpleCommand->_arguments[1]);
        }

        getcwd(working_dir, sizeof (working_dir));
        finishExecuting(default_in, default_out, default_err);
        return;
    }

    int fdin = dup(default_in);
    int fdout = dup(default_out);
    int fderr = dup(default_err);
    int fpipes[2];

    // pipe command to pass data between processes it returns two file descriptors
    // also we check it is successful if it returns 0 and error if -1
    char* input = getFullPath(_inputFile);
    char* output = getFullPath(_outFile);
    char* error = getFullPath(_errFile);

    if (input[0] != '\0') {
        fdin = open(input, O_RDONLY);
    }
    dup2(fdin, 0);
    close(fdin);

    //here he check if i is the last command to set the output and error file
    //in case of single greater than sign it will overwrite the output file
    if(output[0] != '\0') {
        int mode = _append ? O_WRONLY | O_CREAT | O_APPEND : O_WRONLY | O_CREAT | O_TRUNC;

        fdout = open(output, mode, 0666);
        _append = 0;
    }
    dup2(fdout, 1);
    close(fdout);

    //in case of double greater than sign it will overwrite the error file
    if (error[0] != '\0') {
        fderr = open(error, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }
    dup2(fderr, 2);
    close(fderr);


    int pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        //child: Executing the actual command
        if (execvp(_currentSimpleCommand->_arguments[0], _currentSimpleCommand->_arguments) < 0) {
            perror("execvp");
            exit(1);
        }
    }

    finishExecuting(default_in, default_out, default_err);
}

// Shell implementation
void Command::prompt() {
    printf("%s# ", working_dir);
    fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int yyparse();

int main() {
    getcwd(working_dir, sizeof(working_dir));

    Command::_currentCommand.prompt();
    yyparse();
    return 0;
}