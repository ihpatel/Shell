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
#include <unistd.h>
#include <signal.h>
#include <wait.h>

#include "shell.hh"

int yyparse(void);

void Shell::prompt() {
  if (isatty(0)) {
    printf("myshell>");
  }
  fflush(stdout);
}

extern "C" void signalHandler(int sig) {
  if (sig == SIGINT) {
    // print new line
    printf("\n");
  } else if (sig == SIGCHLD) {
    // reap child processes
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
      // printf("%d exited.", pid);
    }
  }
}

int main(int argc, char** argv) {
  if (argc > 0) {
    Shell::relativePath = std::string(argv[0]);
  }
  Shell::prompt();

  struct sigaction sa;
  sa.sa_handler = signalHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGINT, &sa, NULL)) {
    perror("sigaction");
    exit(1);
  }
  if (sigaction(SIGCHLD, &sa, NULL)) {
    perror("sigaction");
    exit(1);
  }

  yyparse();
}

std::string Shell::relativePath;
Command Shell::_currentCommand;