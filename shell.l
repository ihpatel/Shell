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

/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <cstring>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include "command.hh"
#include "shell.hh"
#include "y.tab.hh"

//////////// Start added code ///////////

extern "C" char * read_line();

int mygetc(FILE * f) {
  static char *p;
  char ch;

  if (!isatty(0)) {
    // stdin is not a tty. Call real getc
    return getc(f);
  }

  // stdin is a tty. Call our read_line.
  if (p==NULL || *p == 0) {
    char * s = read_line();
    p = s;
  }

  ch = *p;
  p++;

  return ch;
}

#undef getc
#define getc(f) mygetc(f)

/////////// End added code ///////////

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%option noyywrap

%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

"<" {
  return LESS;
}

"2>" {
  return TWOGREAT;
}

"&" {
  return AMP;
}

">&" {
  return GREATAMP;
}

">>" {
  return GREATGREAT;
}

">>&" {
  return GREATGREATAMP;
}

"|" {
  return PIPE;
}

["][^\n\"]*["] { // "arg"
  // handle quotes
  std::string arg = std::string(yytext);
  yylval.cpp_string = new std::string(arg.substr(1, arg.length() - 2)); // remove quotes
  return WORD;
}

[^ \t\n|><&]*\\[^ \t\n]* {
  // handle escaping
  char* argPtr = yytext;
  char escapedArg[strlen(yytext) + 1];
  int i = 0;
  while (*argPtr) {
    if (*argPtr == '\\') {
      // skip escape character, add next character to escaped argument
      ++argPtr;
      if (*argPtr == 'n') {
        escapedArg[i] = '\n';
      } else if (*argPtr == 't') {
        escapedArg[i] = '\t';
      } else {
        escapedArg[i] = *argPtr;
      }
    } else {
      // add current character to escaped argument
      escapedArg[i] = *argPtr;
    }
    ++i;
    ++argPtr;
  }
  escapedArg[i] = 0;
  yylval.cpp_string = new std::string(escapedArg);
  return WORD;
}

source\ [^ \t\n]* {
  FILE* file = fopen(yytext + 7, "r");
  if (!file) {
    perror("fopen");
    exit(1);
  }
  std::string cmd;
  while (!feof(file)) {
    cmd += fgetc(file);
  }
  for (int i = cmd.length(); i >= 0; --i) {
    unput(cmd[i]);
  }
}


\$\([^\n\$]*\)|`[^\n`]*` { // $(cmd) or `cmd`
  // create subshell
  std::string cmd = std::string(yytext);
  cmd = cmd.substr(1, cmd.length() - 2); // remove first and last character of the command whether its format is $(cmd) or `cmd`
  if (*yytext == '$') {
    // if the format is $(cmd), remove another character from the front
    cmd = cmd.substr(1);
  }

  // save default input and output to restore at the end
  int defaultin = dup(0);
	int defaultout = dup(1);

  // set up i/o redirection for subshell
  // create pipe to send command from parent to subshell
  int cmdpipe[2];
  if (pipe(cmdpipe) == -1) {
    perror( "pipe");
    exit(1);
  }
  // create pipe to send output from subshell to parent
  int outpipe[2];
  if (pipe(outpipe) == -1) {
    perror( "pipe");
    exit(1);
  }

  // write command to cmdpipe
  write(cmdpipe[1], cmd.c_str(), cmd.length());
  write(cmdpipe[1], "\nexit\n", 6);
  close(cmdpipe[1]);

  // redirect stdin to read end of cmdpipe
  dup2(cmdpipe[0], 0);
  // redirect stdout to write end of outpipe
  dup2(outpipe[1], 1);

  // close redundant file descriptors
  close(cmdpipe[0]);
  close(outpipe[1]);

  // fork process for subshell
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(1);
  } else if (pid == 0) {
    // child
    char* argv[1];
    argv[0] = strdup("/proc/self/exe");
    execvp("/proc/self/exe", argv);
    perror("exec");
    exit(1);
  }

  // restore default input and output
	dup2(defaultin, 0);
	dup2(defaultout, 1);

  // close redundant file descriptors
  close(defaultin);
  close(defaultout);

  // read output from subshell
  char* buf = (char*) calloc(1, 1);
  size_t length = 0;
  char* i = buf;
  while (read(outpipe[0], i, 1)) {
    // read from pipe 1 byte at a time
    ++length;
    char* temp = (char*) calloc(length + 1, 1);
    for (size_t j = 0; j < length; ++j) {
      if (buf[j] == '\n') {
        temp[j] = ' ';
      } else {
        temp[j] = buf[j];
      }
    }
    free(buf);
    buf = temp;
    i = buf + length;
  }

  // put the output back into the buffer backwards
  for (int j = length - 1; j >= 0; --j) {
    unput(buf[j]);
  }

  free(buf);
}

[^ \t\n]*\$\{[^\n\}]*\}[^ \t\n]* { // ${arg}
  // arg is an environment variable, replace it with the real value
  std::string envVar = std::string(yytext);
  int openBraceIdx = envVar.find("${", 0);
  int closeBraceIdx = envVar.find("}", 0);
  envVar = envVar.substr(openBraceIdx + 2, closeBraceIdx - openBraceIdx - 2); // delete first two characters ("${") and last character ("}")
  char* envVarValue;
  char buf[PATH_MAX];
  if (!strcmp(envVar.c_str(), "$")) {
    envVarValue = const_cast<char*>(std::to_string(getpid()).c_str());
  } else if (!strcmp(envVar.c_str(), "?")) {
    envVarValue = const_cast<char*>(std::to_string(Command::prevReturnCode).c_str());
  } else if (!strcmp(envVar.c_str(), "!")) {
    envVarValue = const_cast<char*>(std::to_string(Command::prevBackgroundPid).c_str());
  } else if (!strcmp(envVar.c_str(), "_")) {
    envVarValue = const_cast<char*>(Command::lastArg.c_str());
  } else if (!strcmp(envVar.c_str(), "SHELL")) {
    realpath(Shell::relativePath.c_str(), buf);
    envVarValue = buf;
  } else if (!(envVarValue = getenv(envVar.c_str()))) {
    perror("getenv");
    exit(1);
  }
  int len = strlen(yytext);
  for (int i = len - 1; i >= closeBraceIdx + 1; --i) {
    unput(yytext[i]);
  }
  len = strlen(envVarValue);
  for (int i = len - 1; i >= 0; --i) {
    unput(envVarValue[i]);
  }
  for (int i = openBraceIdx - 1; i >= 0; --i) {
    unput(yytext[i]);
  }
}

~[^ \t\n]* {
  char* dir = getenv("HOME");
  if (strlen(yytext) > 1) {
    for (int i = strlen(yytext) - 1; i > 0; --i) {
      unput(yytext[i]);
    }
    std::string dirString = std::string(dir);
    size_t lastSlashIdx = dirString.find_last_of("/");
    for (int i = lastSlashIdx; i >= 0; --i) {
      unput(dir[i]);
    }
  } else {
    for (int i = strlen(dir); i >= 0; --i) {
      unput(dir[i]);
    }
  }
}

[^ \t\n]*[><|&][><|&]*[^ \t\n]* {
  // add spaces before and after every (unescaped) i/o redirection operator
  std::string ioRedirectionOp = std::string(yytext);

  // just some crazy logic to handle all the different i/o redirection operators as concisely as possible
  for (int i = ioRedirectionOp.length() - 1; i >= 0; --i) {
    if (ioRedirectionOp[i] == '>' || ioRedirectionOp[i] == '<' || ioRedirectionOp[i] == '|' || ioRedirectionOp[i] == '&') {
      if (i - 1 < 0 || ioRedirectionOp[i - 1] != '\\') {
        if ((ioRedirectionOp[i] == '>' && ioRedirectionOp[i - 1] == '>') || (ioRedirectionOp[i] == '<' && ioRedirectionOp[i - 1] == '<')
            || (ioRedirectionOp[i] == '>' && ioRedirectionOp[i - 1] == '2') || (ioRedirectionOp[i] == '&' && ioRedirectionOp[i - 1] == '>')) {
          if (i - 2 < 0 || ioRedirectionOp[i - 2] != '\\') {
            if (ioRedirectionOp[i] == '&' && ioRedirectionOp[i - 1] == '>' && ioRedirectionOp[i - 2] == '>') {
              if (i - 3 < 0 || ioRedirectionOp[i - 3] != '\\') {
                unput(' ');
                unput(ioRedirectionOp[i]);
                unput(ioRedirectionOp[i - 1]);
                unput(ioRedirectionOp[i - 2]);
                unput(' ');
                i -= 2;
              } else {
                unput(ioRedirectionOp[i]);
                unput(ioRedirectionOp[i - 1]);
                unput(ioRedirectionOp[i - 2]);
                i -= 2;
              }
            } else {
              unput(' ');
              unput(ioRedirectionOp[i]);
              unput(ioRedirectionOp[i - 1]);
              unput(' ');
              --i;
            }
          } else {
            unput(ioRedirectionOp[i]);
            unput(ioRedirectionOp[i - 1]);
            --i;
          }
        } else {
          unput(' ');
          unput(ioRedirectionOp[i]);
          unput(' ');
        }
      } else {
        unput(ioRedirectionOp[i]);
      }
    } else {
      unput(ioRedirectionOp[i]);
    }
  }
}

[^ \t\n][^ \t\n]*  {
  /* Assume that file names have only alpha chars */
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}
