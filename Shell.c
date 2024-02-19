#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "Jobs.h"
#include "Parser.h"
#include "Interpreter.h"
#include "error.h"

int main() {
  int eof=0;
  Jobs jobs=newJobs();
  char *prompt=0;

  // check if stdin is a terminal
  // if it is, enable command history (read from .history file), set the prompt to "$ "
  // if not, bind the tab key to insert a tab character, redirect readline's output to /dev/null to suppress it
  if (isatty(fileno(stdin))) {
    using_history();
    read_history(".history");
    prompt="$ ";
  } else {
    rl_bind_key('\t',rl_insert);
    rl_outstream=fopen("/dev/null","w");
  }
  
  // read commands from prompt, add to history, parse and interpret the command
  while (!eof) {
    char *line=readline(prompt);
    if (!line)
      break;
    if (*line)
      add_history(line);
    Tree tree=parseTree(line);
    free(line);
    interpretTree(tree,&eof,jobs);
    freeTree(tree);
  }

  // if stdin is a terminal, write the command history to .history file and clear the history
  // if not, close the output stream
  if (isatty(fileno(stdin))) {
    write_history(".history");
    clear_history();
  } else {
    fclose(rl_outstream);
  }

  // clean up any remaining resources
  freestateCommand();
  return 0;
}
