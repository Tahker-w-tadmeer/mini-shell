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

void Command::finishExecuting(int default_in, int default_out, int default_err) {
    dup2(default_in, 0);
    dup2(default_out, 1);
    dup2(default_err, 2);
    close(default_in);
    close(default_out);
    close(default_err);

    printf("Hello\n");
    if (_background == 0) {
        waitpid(-1, nullptr, 0);
    }
    printf("After %d\n", _background);

    // Clear to prepare for next command
    clear();

    // Print new prompt
    prompt();
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

        if (!isLastCommand && _numberOfSimpleCommands > 1) {
            if (pipe(fpipes) == -1) {
                perror("pipe");
                exit(2);
            }
        }

        int fdin = i == 0 ? default_in : fpipes[0];
        int fdout = isLastCommand ? default_out : fpipes[1];

        int fderr = default_err;

        char *input = getFullPath(_inputFile);
        char *output = getFullPath(_outFile);
        char *error = getFullPath(_errFile);

        if (input[0] != '\0' && i == 0) {
            fdin = open(input, O_RDONLY);
        }
        dup2(fdin, 0);

        //in case of single greater than sign it will overwrite the output file
        if (output[0] != '\0' && isLastCommand) {
            int mode = _append ? O_WRONLY | O_CREAT | O_APPEND : O_WRONLY | O_CREAT | O_TRUNC;

            fdout = open(output, mode, 0666);
            _append = 0;
        }
        dup2(fdout, 1);

        if (error[0] != '\0' && isLastCommand) {
            fderr = open(error, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        }
        dup2(fderr, 2);

        pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            //child: Executing the actual command
            close(fpipes[0]);
            close(fpipes[1]);
            close(default_in);
            close(default_out);
            close(default_err);

            execvp(currentSimpleCommand->_arguments[0], currentSimpleCommand->_arguments);

            perror("Execute failed");
            exit(2);
        }
    }

    close(fpipes[0]);
    close(fpipes[1]);
    finishExecuting(default_in, default_out, default_err);
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int yyparse();

void signalHandler(int signum) {
    Command::_currentCommand.clear();

    printf("\n");

    Command::_currentCommand.prompt();
}

//void handleChildDeath(int sig_num) {
//    pid_t child_pid;
//    int status;
//    char path_to_log[64];
//    strcpy(path_to_log, argv[1]);
//    strcat(path_to_log, "/child-log.txt");
//    FILE *logFile = fopen(path_to_log, "a");
//    if (logFile == NULL) {
//        perror("Error opening log file");
//        exit(EXIT_FAILURE);
//    }
//    time_t Time = time(NULL);
//    tm *time_pointer = localtime((&Time));
//    char currentTime[32];
//    strcpy(currentTime, asctime(time_pointer));
//    removeNewline(currentTime, 32);
//    fprintf(logFile, "%s: Child Terminated\n", currentTime);
//    fclose(logFile);
//}

void handleChildDeath(int sigchild) { // Funeral
    int default_out = dup(1);
    int log = open("logs.txt", O_WRONLY | O_CREAT | O_APPEND, 0666); // Sus
    dup2(log, 1);

    SimpleCommand *lastCommand = Command::_currentCommand._simpleCommands[
            Command::_currentCommand._numberOfSimpleCommands - 1];

    for (int i = 0; i < lastCommand->_numberOfArguments; i++) {
        std::cout << lastCommand->_arguments[i] << " ";
    }
    std::cout << std::endl;

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