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
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires 
{
#include <string>
#include <string.h>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%token <cpp_string> WORD
%token NOTOKEN GREAT NEWLINE LESS TWOGREAT AMP GREATAMP GREATGREAT GREATGREATAMP PIPE

%{
//#define yylex yylex
#include <cstdio>
#include "shell.hh"

void yyerror(const char * s);
int yylex();

%}

%%

goal:
  commands
  ;

commands:
  command
  | commands command
  ;

command: simple_command
       ;

simple_command:	
  command_and_args iomodifier_opt NEWLINE {
    // printf("   Yacc: Execute command\n");
    Shell::_currentCommand.execute();
  }
  | command_and_args PIPE
  | NEWLINE 
  | error NEWLINE { yyerrok; }
  ;

command_and_args:
  command_word argument_list {
    Shell::_currentCommand.
    insertSimpleCommand( Command::_currentSimpleCommand );
  }
  ;

argument_list:
  argument_list argument
  | /* can be empty */
  ;

argument:
  WORD {
    // printf("   Yacc: insert argument \"%s\"\n", $1->c_str());
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  ;

command_word:
  WORD {
    // printf("   Yacc: insert command \"%s\"\n", $1->c_str());
    Command::_currentSimpleCommand = new SimpleCommand();
    std::string * arg = $1;
    if (arg->find("*") != std::string::npos) {
      Command::_currentSimpleCommand->insertArgument( $1 );
    } else if (arg->find("?") != std::string::npos) {
      Command::_currentSimpleCommand->insertArgument( $1 );
    } else {
      Command::_currentSimpleCommand->insertArgument( $1 );
    }
  }
  ;

iomodifier_opt:
  iomodifier_opt iomodifier_opt
  | GREAT WORD {
    // printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    if (Shell::_currentCommand._outFile) {
      printf("Ambiguous output redirect.\n");
    }
    Shell::_currentCommand._outFile = $2;
  }
  | LESS WORD {
    Shell::_currentCommand._inFile = $2;
  }
  | TWOGREAT WORD {
    Shell::_currentCommand._errFile = $2;
  }
  | AMP {
    Shell::_currentCommand._background = true;
  }
  | GREATAMP WORD {
    Shell::_currentCommand._outFile = new std::string($2->c_str());
    Shell::_currentCommand._errFile = $2;
  }
  | GREATGREAT WORD {
    Shell::_currentCommand._outFile = $2;
    Shell::_currentCommand._appendOut = true;
  }
  | GREATGREATAMP WORD {
    Shell::_currentCommand._outFile = new std::string($2->c_str());
    Shell::_currentCommand._errFile = $2;
    Shell::_currentCommand._appendOut = true;
    Shell::_currentCommand._appendErr = true;
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
