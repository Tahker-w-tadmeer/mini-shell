#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <string>
#include <sys/fcntl.h>
#include <iostream>

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

    _numberOfSimpleCommands = 0;
    _outFile = nullptr;
    _inputFile = nullptr;
    _errFile = nullptr;
    _background = 0;
    _append = 0;
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
    _outFile = nullptr;
    _inputFile = nullptr;
    _errFile = nullptr;
    _background = 0;
}

void Command::print() const {
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

char *getFullPath(char *file) {
    char sep[] = "/";
    char *fullpath = new char[sizeof(file) + sizeof(working_dir) + 1];
    if (file && file[0] != sep[0]) {
        strcpy(fullpath, working_dir);
        strcat(fullpath, sep);
        strcat(fullpath, file);
    }

    return fullpath;
}

void clearLogs() {
    remove("logs.txt");
}

// Shell implementation
void Command::prompt() {
    printf("%s# ", working_dir);
    fflush(stdout);
}

void Command::execute() {
    if (_numberOfSimpleCommands == 0) {
        prompt();
        return;
    }
    int default_in = dup(0); // 0 is the file descriptor for stdin
    int default_out = dup(1); // 1 is the file descriptor for stdout
    int default_err = dup(2); // 2 is the file descriptor for stderr

    int fpipes[2];
    int pid;
    int fdin, fdout, fderr;
    char *input = getFullPath(_inputFile);
    char *output = getFullPath(_outFile);
    char *error = getFullPath(_errFile);

    fdin = dup(default_in);
    if (input[0] != '\0') {
        fdin = open(input, O_RDONLY);
    }

    for (int i = 0; i < _numberOfSimpleCommands; i++) {
        bool isLastCommand = _numberOfSimpleCommands - 1 == i;

        SimpleCommand *currentSimpleCommand = _simpleCommands[i];
        char *cmd = currentSimpleCommand->_arguments[0];

        // Handle exit command
        if (strcasecmp(cmd, "exit") == 0) {
            printf("Goodbye\n");
            exit(1);
        }

        // Handle change directory cd
        if (strcasecmp(cmd, "cd") == 0) {
            chdir(
                    (currentSimpleCommand->_arguments[1] == nullptr)
                    ? getenv("HOME")
                    : currentSimpleCommand->_arguments[1]
            );

            getcwd(working_dir, sizeof(working_dir));

            continue;
        }

        dup2(fdin, 0);
        close(fdin);

        //in case of single greater than sign it will overwrite the output file
        if (output[0] != '\0' && isLastCommand) {
            int mode = _append ? O_WRONLY | O_CREAT | O_APPEND : O_WRONLY | O_CREAT | O_TRUNC;

            fdout = open(output, mode, 0666);
            _append = 0;
        } else {
            fdout = dup(default_out);
        }

        if (error[0] != '\0' && isLastCommand) {
            fderr = open(error, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        } else {
            fderr = dup(default_err);
        }

        if (!isLastCommand && _numberOfSimpleCommands > 1) {
            if (pipe(fpipes) == -1) {
                perror("pipe");
                exit(2);
            }

            fdout = fpipes[1];
            fdin = fpipes[0];
        }

        pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        }

        dup2(fdout, 1);
        dup2(fderr, 2);
        close(fdout);
        close(fderr);

        if (pid == 0) {
            //child: Executing the actual command

            if (strcasecmp(cmd, "echo") == 0) {
                char *command = new char[sizeof(_simpleCommands[i]->_arguments[0]) +
                                         sizeof(_simpleCommands[i]->_arguments[1]) + 1];
                strcpy(command, _simpleCommands[i]->_arguments[0]);
                strcat(command, " ");
                strcat(command, _simpleCommands[i]->_arguments[1]);

                execlp("/bin/sh", "sh", "-c", command, nullptr);
            }

            execvp(currentSimpleCommand->_arguments[0], currentSimpleCommand->_arguments);

            perror("Execute failed");
            exit(2);
        }
    }

    close(fdin);
    close(fdout);
    close(fderr);
    dup2(default_in, 0);
    dup2(default_out, 1);
    dup2(default_err, 2);

    if (!_background) {
        waitpid(pid, 0, 0);
    }

    // Clear to prepare for next command
    clear();

    // Print new prompt
    prompt();
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int yyparse();

void signalHandler(int signum) {
    Command::_currentCommand.clear();

    printf("\n");

    Command::_currentCommand.prompt();
}

void handleChildDeath(int sigchild) { // Funeral
    int default_out = dup(1);
    int log = open("logs.txt", O_WRONLY | O_CREAT | O_APPEND, 0666); // Sus
    dup2(log, 1);

    for (int i = 0; i < Command::_currentCommand._numberOfSimpleCommands; i++) {
        SimpleCommand *command = Command::_currentCommand._simpleCommands[i];

        std::cout << "Child Died: '";

        for (int j = 0; j < command->_numberOfArguments; j++) {
            std::cout << command->_arguments[j] << " ";
        }
        time_t Time = time(nullptr);
        tm *time_pointer = localtime((&Time));
        char currentTime[32];
        strcpy(currentTime, asctime(time_pointer));
        std::cout << "' " << currentTime;
        std::cout << std::endl;

    }
    close(log);
    dup2(default_out, 1);
    close(default_out);
}

int main() {
    clearLogs();

    signal(SIGINT, signalHandler);
    signal(SIGCHLD, handleChildDeath);

    getcwd(working_dir, sizeof(working_dir));

    Command::_currentCommand.prompt();
    yyparse();
    return 0;
}