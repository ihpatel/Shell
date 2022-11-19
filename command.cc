/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT 
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE 
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM  
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <iostream>

#include "command.hh"
#include "shell.hh"


Command::Command() {
    // Initialize a new vector of Simple Commands
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simpleCommand);
}

void Command::clear() {
    // deallocate all the simple commands in the command vector
    for (auto simpleCommand : _simpleCommands) {
        delete simpleCommand;
    }

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();

    if ( _outFile ) {
        delete _outFile;
    }
    _outFile = NULL;

    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;

    if ( _errFile ) {
        delete _errFile;
    }
    _errFile = NULL;

    _background = false;
}

void Command::print() {
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simpleCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        simpleCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFile?_outFile->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}

extern char** environ;
void Command::execute() {
    // Don't do anything if there are no simple commands
    if ( _simpleCommands.size() == 0 ) {
        Shell::prompt();
        return;
    }

    // Print contents of Command data structure
    // print();

    // Add execution here
    // For every simple command fork a new process
    // Setup i/o redirection
    // and call exec

    // save default input, output, and error to restore at the end
    int defaultin = dup(0);
	int defaultout = dup(1);
	int defaulterr = dup(2);

    // set up i/o redirection
    if (_inFile) {
        int in_fd = open(_inFile->c_str(), O_RDONLY);
        if (in_fd < 0) {
            perror("open input file");
            exit(1);
        }
        dup2(in_fd, 0);
        close(in_fd);
    }
    if (_errFile) {
        int err_fd;
        if (_appendErr) {
            err_fd = open(_errFile->c_str(), O_RDWR | O_APPEND | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
        } else {
            err_fd = open(_errFile->c_str(), O_RDWR | O_TRUNC | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
        }
        if (err_fd < 0) {
            perror("open error file");
            exit(1);
        }
        dup2(err_fd, 2);
        close(err_fd);
    }

    int pid = 0; // declared outside loop so that we only wait on last simple command
    for (size_t i = 0; i < _simpleCommands.size(); ++i) {
        SimpleCommand* simpleCommand = _simpleCommands[i];
        Command::lastArg = *simpleCommand->_arguments[simpleCommand->_arguments.size() - 1];

        // handle exit
        if (!simpleCommand->_arguments[0]->compare("exit")) {
            // printf("Good bye!!\n");
            close(defaultin);
            close(defaultout);
            close(defaulterr);
            clear();
            exit(0);
        }

        int fdpipe[2];
        if (pipe(fdpipe) == -1) {
            perror( "pipe");
            exit(1);
        }
        if (i < _simpleCommands.size() - 1) {
            dup2(fdpipe[1], 1); // set up write end of pipe for current simple command
        } else {
            // if last command, set output to outfile
            if (_outFile) {
                int out_fd;
                if (_appendOut) {
                    out_fd = open(_outFile->c_str(), O_RDWR | O_APPEND | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
                } else {
                    out_fd = open(_outFile->c_str(), O_RDWR | O_TRUNC | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
                }
                if (out_fd < 0) {
                    perror("open output file");
                    exit(1);
                }
                dup2(out_fd, 1);
                close(out_fd);
            }
        }

        // handle builtin commands
        if (!simpleCommand->_arguments[0]->compare("setenv")) {
            if (setenv(simpleCommand->_arguments[1]->c_str(), simpleCommand->_arguments[2]->c_str(), 1)) {
                perror("setenv");
            }
        } else if (!simpleCommand->_arguments[0]->compare("unsetenv")) {
            if (unsetenv(simpleCommand->_arguments[1]->c_str())) {
                perror("unsetenv");
            }
        } else if (!simpleCommand->_arguments[0]->compare("cd")) {
            if (simpleCommand->_arguments.size() < 2) {
                // default to home directory if no directory is specified
                if (chdir(getenv("HOME"))) {
                    perror("cd");
                }
            } else {
                if (!simpleCommand->_arguments[1]->substr(0, 2).compare("${") && (*simpleCommand->_arguments[1])[simpleCommand->_arguments[1]->length() - 1] == '}') {
                    // if arg is of the form "${arg}", arg is an environment variable; treat it as such
                    std::string envVar = *simpleCommand->_arguments[1];
                    envVar = envVar.substr(2, envVar.length() - 3);
                    if (chdir(getenv(envVar.c_str()))) {
                        perror("cd");
                    }
                } else if (chdir(simpleCommand->_arguments[1]->c_str())) {
                    std::string errorMessage = "cd: can't cd to " + *simpleCommand->_arguments[1];
                    perror(errorMessage.c_str());
                }
            }
        } else {
            pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(1);
            } else if (pid == 0) {
                //Child
                // close unnecessary file descriptors
                close(fdpipe[1]);
                close(fdpipe[0]);
                close(defaultin);
                close(defaultout);
                close(defaulterr);

                // handle printenv
                if (!simpleCommand->_arguments[0]->compare("printenv")) {
                    int i = 0;
                    while (environ[i]) {
                        printf("%s\n", environ[i]);
                        ++i;
                    }
                    exit(0);
                } else {
                    // not a builtin function, set up args and execute command
                    // set up args array
                    std::vector<char *> argv(simpleCommand->_arguments.size() + 1);
                    for (std::size_t i = 0; i < simpleCommand->_arguments.size(); ++i) {
                        argv[i] = const_cast<char*>(simpleCommand->_arguments[i]->c_str());
                    }

                    // execute command
                    execvp(argv[0], argv.data());
                    // perror("exec");
                    exit(1);
                }
            }
        }

        dup2(fdpipe[0], 0); // set up read end of pipe for next simple command
        // close unnecessary file descriptors
        close(fdpipe[0]);
        close(fdpipe[1]);
        close(1);
        dup2(defaultout, 1);
    }

    close(0);
    close(1);
    close(2);

    // Restore input, output, and error
	dup2(defaultin, 0);
	dup2(defaultout, 1);
	dup2(defaulterr, 2);

    // Close file descriptors that are not needed
	close(defaultin);
	close(defaultout);
	close(defaulterr);

    int status = 0; // not used right now, will be used in part 3 to print error message
    if (_background) {
        // call waitpid with WNOHANG option so that the shell doesn't wait for the process to finish
        waitpid(pid, &status, WNOHANG);
        // set prevBackgroundPid to pid
        Command::prevBackgroundPid = pid;
    } else {
        waitpid(pid, &status, 0);
        // set prevReturncode to exit status
        Command::prevReturnCode = WEXITSTATUS(status);
    }

    // Clear to prepare for next command
    clear();

    // Print new prompt
    Shell::prompt();
}

pid_t Command::prevBackgroundPid;
int Command::prevReturnCode;
std::string Command::lastArg;
SimpleCommand * Command::_currentSimpleCommand;
