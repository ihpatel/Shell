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

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <iostream>
#include <regex.h>
#include <sys/types.h>

#include "simpleCommand.hh"

SimpleCommand::SimpleCommand() {
  _arguments = std::vector<std::string *>();
}

SimpleCommand::~SimpleCommand() {
  // iterate over all the arguments and delete them
  for (auto & arg : _arguments) {
    delete arg;
  }
}

bool compareStringPtrs(std::string* a, std::string* b) {
  return *a < *b;
}

void exploreDir(std::string dirName, std::string rest, std::vector<std::string*>* files) {
  size_t firstSlashIdx = rest.find("/");
  bool isLeaf = false;
  std::string nextDir = std::string(rest);
  std::string nextRest("");
  if (firstSlashIdx == std::string::npos) {
    isLeaf = true;
  } else {
    nextDir = rest.substr(0, firstSlashIdx);
    nextRest = rest.substr(firstSlashIdx + 1);
  }
    
  /*
  *  Add ^ and $ at the beginning and end of regular expression
  *  to force match of the entire string. See "man ed".
  */
  std::string regExpComplete = "^" + nextDir + "$";

  regex_t re;
  int result = regcomp(&re, regExpComplete.c_str(),  REG_EXTENDED|REG_NOSUB);
  if(result != 0) {
    fprintf(stderr, "Bad regular expresion \"%s\"\n", regExpComplete.c_str());
    exit(-1);
  }

  DIR* dirp = opendir(dirName.c_str());
  if (dirp != NULL) {
    struct dirent* de;
    while ((de = readdir(dirp)) != NULL) {
      if ((rest[2] == '*' || rest[2] == '+') && de->d_name[0] == '.') {
        // if first character of file name is . and first character of regex is wildcard don't match
        continue;
      }

      regmatch_t match;
      result = regexec(&re, de->d_name, 1, &match, 0);

      if (result == 0) {
        if (isLeaf) {
          if (dirName.compare(".") == 0) {
            files->push_back(new std::string(de->d_name));
          } else {
            files->push_back(new std::string(dirName + de->d_name));
          }
        } else {
          if (de->d_type == DT_DIR) {
            exploreDir(dirName + de->d_name + "/", nextRest, files);
          }
        }
      }
    }

    closedir(dirp);
  }
  regfree(&re);
}

void SimpleCommand::insertArgument(std::string* argument) {
  if (argument->find("*") != std::string::npos || argument->find("?") != std::string::npos) {
    size_t firstSlashIdx = argument->find("/");
    std::string dirName;
    std::string rest;
    if (firstSlashIdx != std::string::npos) {
      // extract directory and regex
      dirName = argument->substr(0, firstSlashIdx + 1);
      rest = argument->substr(firstSlashIdx + 1, argument->length() - firstSlashIdx - 1);
    } else {
      dirName = std::string(".");
      rest = std::string(*argument);
    }
    // replace * with (.*), ? with (.+) in regex
    for (size_t i = 0; i < rest.length(); ++i) {
      if (rest[i] == '*') {
        rest = rest.substr(0, i) + "(.*)" + rest.substr(i + 1);
        i += 3;
      } else if (rest[i] == '?') {
        rest = rest.substr(0, i) + "(.+)" + rest.substr(i + 1);
        i += 3;
      } else if (rest[i] == '.') {
        rest = rest.substr(0, i) + "\\." + rest.substr(i + 1);
        ++i;
      }
    }

    std::vector<std::string*> files;

    exploreDir(dirName, rest, &files); // explore relevant directories, add matching file names to files

    std::sort(files.begin(), files.end(), compareStringPtrs);

    if (files.empty()) {
      _arguments.push_back(argument);
    } else {
      for (std::string* file : files) {
        _arguments.push_back(file);
      }
      delete argument;
    }
  } else {
    // simply add the argument to the vector
    _arguments.push_back(argument);
  }
}

// Print out the simple command
void SimpleCommand::print() {
  for (auto & arg : _arguments) {
    std::cout << "\"" << *arg << "\" \t";
  }
  // effectively the same as printf("\n\n");
  std::cout << std::endl;
}
