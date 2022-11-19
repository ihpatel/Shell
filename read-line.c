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
 * CS252: Systems Programming
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_LINE 2048

extern void tty_raw_mode(void);

// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];

// cursor index in line
int cursor_index;

// History array
int history_index = 0;
char** history = NULL;
int history_length = 0;

void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " left arrow   Moves the cursor one position to the left\n"
    " right arrow  Moves the cursor one position to the right\n"
    " ctrl-d       Deletes character at the cursor\n"
    " Backspace    Deletes character to the left of cursor\n"
    " ctrl-h       Same as backspace\n"
    " ctrl-a       Moves cursor to the beginning of the line\n"
    " ctrl-e       Moves cursor to the end of the line\n"
    " up arrow     See previous command in the history\n"
    " down arrow   See next command in the history\n";

  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {

  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;
  cursor_index = 0;

  if (!history) {
    history = calloc(1, sizeof(char*));
    history[0] = calloc(1, 1);
    ++history_length;
  }

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch>=32 && ch != 127) {
      // It is a printable character.
      // Handle insert in middle
      int i = cursor_index;
      char tmp = 0;
      for (; i < line_length; ++i) {
        tmp = line_buffer[i];
        write(1, &ch, 1);
        line_buffer[i] = ch;
        ch = tmp;
      }
      // Do echo
      write(1,&ch,1);
      ++i;
      ++cursor_index;
      for (; i > cursor_index; --i) {
        write(1, "\033[D", 3);
      }

      // If max number of character reached return.
      if (line_length==MAX_BUFFER_LINE-2) break; 

      // add char to buffer.
      line_buffer[line_length]=ch;
      line_length++;
    }
    else if (ch==10) {
      // <Enter> was typed. Return line
      // Print newline
      write(1,&ch,1);
      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }
    else if (ch == 8 || ch == 127) {
      // <backspace> was typed. Remove character to the left of cursor.
      if (cursor_index == 0) {
        // if cursor is at the beginning of the line (no characters to the left), do nothing
        continue;
      }

      // move cursor left one space
      write(1, "\033[D", 3);
      --cursor_index;

      // overwrite characters with characters to the right
      int i = cursor_index;
      for (; i < line_length - 1; ++i) {
        write(1, line_buffer + i + 1, 1);
        line_buffer[i] = line_buffer[i + 1];
      }

      write(1, " ", 1);
      line_buffer[i] = 0;
      ++i;

      // move cursor back to where it should be
      for (; i > cursor_index; --i) {
        write(1, "\033[D", 3);
      }

      // Remove one character from buffer
      line_length--;
    }
    else if (ch == 4) {
      // ctrl-d, delete character under cursor
      // overwrite characters with characters to the right
      int i = cursor_index;
      for (; i < line_length - 1; ++i) {
        write(1, line_buffer + i + 1, 1);
        line_buffer[i] = line_buffer[i + 1];
      }

      write(1, " ", 1);
      line_buffer[i] = 0;
      ++i;

      // move cursor back to where it should be
      for (; i > cursor_index; --i) {
        write(1, "\033[D", 3);
      }

      // Remove one character from buffer
      line_length--;
    }
    else if (ch == 1) {
      // ctrl-a, move cursor to beginning of line
      while (cursor_index > 0) {
        write(1, "\033[D", 3);
        --cursor_index;
      }
    }
    else if (ch == 5) {
      // ctrl-e, move cursor to end of line
      while (cursor_index < line_length) {
        write(1, "\033[C", 3);
        ++cursor_index;
      }
    }
    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      if (ch1==91 && ch2==65) {
        // Up arrow. Print next line in history.
        if (history_index == history_length - 1) {
          // if already at top of history, do nothing
          continue;
        }
        // Erase old line
        // Print backspaces
        int i = 0;
        for (i =0; i < line_length; i++) {
          ch = 8;
          write(1,&ch,1);
        }

        // Print spaces on top
        for (i =0; i < line_length; i++) {
          ch = ' ';
          write(1,&ch,1);
        }

        // Print backspaces
        for (i = 0; i < line_length; i++) {
          ch = 8;
          write(1,&ch,1);
        }	

        // Copy line from history
        ++history_index;
        strcpy(line_buffer, history[history_index]);
        line_length = strlen(line_buffer);

        // echo line
        write(1, line_buffer, line_length);

        // move cursor to end of line
        cursor_index = line_length;
      } else if (ch1==91 && ch2==66) {
        // down arrow, print previous line in history
        if (history_index == 0) {
          // if already at bottom of history, do nothing
          continue;
        }
        // Erase old line
        // Print backspaces
        int i = 0;
        for (i =0; i < line_length; i++) {
          ch = 8;
          write(1,&ch,1);
        }

        // Print spaces on top
        for (i =0; i < line_length; i++) {
          ch = ' ';
          write(1,&ch,1);
        }

        // Print backspaces
        for (i = 0; i < line_length; i++) {
          ch = 8;
          write(1,&ch,1);
        }	

        // Copy line from history
        --history_index;
        strcpy(line_buffer, history[history_index]);
        line_length = strlen(line_buffer);

        // echo line
        write(1, line_buffer, line_length);

        // move cursor to end of line
        cursor_index = line_length;
      } else if (ch1==91 && ch2==67) {
        // right arrow, move cursor forward
        if (cursor_index < line_length) {
          write(1, "\033[C", 3);
          ++cursor_index;
        }
      } else if (ch1==91 && ch2==68) {
        // left arrow, move cursor back
        if (cursor_index > 0) {
          write(1, "\033[D", 3);
          --cursor_index;
        }
      }
    }
  }

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;

  // insert new line into history array
  char* new_history_line = calloc(line_length, 1);
  for (int i = 0; i < line_length - 1; ++i) {
    new_history_line[i] = line_buffer[i];
  }
  char** tmp = (char**) calloc(history_length + 2, sizeof(char*));
  tmp[0] = history[0];
  tmp[1] = new_history_line;
  for (int i = 1; i < history_length; ++i) {
    tmp[i + 1] = history[i];
  }
  free(history);
  history = tmp;
  ++history_length;

  // reset history index
  history_index = 0;

  if (!strcmp(line_buffer, "exit\n")) {
    // if exiting shell session, free allocated memory for history
    for (int i = 0; i < history_length; ++i) {
      free(history[i]);
    }
    free(history);
  }

  return line_buffer;
}

