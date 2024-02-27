#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "Command.h"
#include "error.h"

typedef struct {
  char *file; // represents the command to be executed
  char **argv; // represents the arguments to the command
  char *inputRedir; // represents the input redirection file
  char *outputRedir; // represents the output redirection file
} *CommandRep;

#define BIARGS CommandRep r, int *eof, Jobs jobs
#define BINAME(name) bi_##name
#define BIDEFN(name) static void BINAME(name) (BIARGS)
#define BIENTRY(name) {#name,BINAME(name)}

static char *owd=0;
static char *cwd=0;

static void builtin_args(CommandRep r, int n) {
  char **argv=r->argv;
  for (n++; *argv++; n--);
  if (n)
    ERROR("wrong number of arguments to builtin command"); // warn
}

BIDEFN(exit) {
  builtin_args(r,0);
  *eof=1;
}

BIDEFN(pwd) {
  builtin_args(r,0);
  if (!cwd)
    cwd=getcwd(0,0);

  // check if r has output redirection
  // if true, create and write to the given filename, return (don't print to stdout)
  if (r->outputRedir) {
    int out = open(r->outputRedir, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
    if (out == -1) {
      ERROR("Failed to open output redirection file");
      return;
    }
    dprintf(out, "%s\n", cwd);
    close(out);
    return; // return (don't print to stdout)
  }
  printf("%s\n",cwd);
}

BIDEFN(cd) {
  builtin_args(r,1);
  if (strcmp(r->argv[1],"-")==0) {
    char *twd=cwd;
    cwd=owd;
    owd=twd;
  } else {
    if (owd) free(owd);
    owd=cwd;
    cwd=strdup(r->argv[1]);
  }
  if (cwd && chdir(cwd))
    ERROR("chdir() failed"); // warn
}

BIDEFN(source) {
  char *sourceCommand = malloc(strlen("bash -c 'source ") + strlen(r->argv[1]) + strlen("'") + 1);
  strcpy(sourceCommand, "bash -c 'source ");
  strcat(sourceCommand, r->argv[1]);
  strcat(sourceCommand, "'");
  system(sourceCommand);
  free(sourceCommand);
}

static int builtin(BIARGS) {
  typedef struct {
    char *s;
    void (*f)(BIARGS);
  } Builtin;
  static const Builtin builtins[]={
    BIENTRY(exit),
    BIENTRY(pwd),
    BIENTRY(cd),
    BIENTRY(source),
    {0,0}
  };
  int i;
  for (i=0; builtins[i].s; i++)
    if (!strcmp(r->file,builtins[i].s)) {
      builtins[i].f(r,eof,jobs);
      return 1;
    }
  return 0;
}

static char **getargs(T_words words) {
  int n=0;
  T_words p=words;
  while (p) {
    p=p->words;
    n++;
  }
  char **argv=(char **)malloc(sizeof(char *)*(n+1));
  if (!argv)
    ERROR("malloc() failed");
  p=words;
  int i=0;
  while (p) {
    argv[i++]=strdup(p->word->s);
    p=p->words;
  }
  argv[i]=0;
  return argv;
}

extern Command newCommand(T_command command) {
  CommandRep r=(CommandRep)malloc(sizeof(*r));
  if (!r)
    ERROR("malloc() failed");
  r->argv=getargs(command->words);
  r->file=r->argv[0];
  // add input and output redirection if they exist
  r->inputRedir=command->redir?command->redir->input:0;
  r->outputRedir=command->redir?command->redir->output:0;
  return r;
}

static void child(CommandRep r, int fg, int currPipeFd[2], int newPipeFd[2]) {
  int eof=0;
  Jobs jobs=newJobs();

  // check which pipelines are set
  int isCurrPipeSet = currPipeFd[0] != -1 && currPipeFd[1] != -1;
  int isNewPipeSet = newPipeFd[0] != -1 && newPipeFd[1] != -1;
  
  // if current pipe is set
  if (isCurrPipeSet) {
    close(currPipeFd[1]); // close the write end
    dup2(currPipeFd[0], STDIN_FILENO); // set the read end to stdin
    close(currPipeFd[0]); // close the read end
  }

  // if new pipe is set
  if (isNewPipeSet) {
    close(newPipeFd[0]); // close the read end
    dup2(newPipeFd[1], STDOUT_FILENO); // set the write end to stdout
    close(newPipeFd[1]); // close the write end
  }

  // Handle input redirection
  if (r->inputRedir) {
    int in = open(r->inputRedir, O_RDONLY);
    if (in == -1) {
      ERROR("Failed to open input redirection file");
      exit(1);
    }
    dup2(in, STDIN_FILENO);
    close(in);
  }

  // Handle output redirection
  if (r->outputRedir) {
    int out = open(r->outputRedir, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
    if (out == -1) {
      ERROR("Failed to open output redirection file");
      exit(1);
    }
    dup2(out, STDOUT_FILENO);
    close(out);
  }

  // execute built in command if it exists
  if (builtin(r,&eof,jobs))
    return;
  execvp(r->argv[0],r->argv);
  ERROR("execvp() failed");
  exit(0);
}

extern void execCommand(Command command, Pipeline pipeline, Jobs jobs,
			int *jobbed, int *eof, int fg, int currPipeFd[2], int newPipeFd[2]) {
  CommandRep r=command;

  // check which pipelines are set
  int isCurrPipeSet = currPipeFd[0] != -1 && currPipeFd[1] != -1;
  int isNewPipeSet = newPipeFd[0] != -1 && newPipeFd[1] != -1;

  if (!isCurrPipeSet && !isNewPipeSet && fg && builtin(r,eof,jobs))
    return;
  if (!*jobbed) {
    *jobbed=1;
    addJobs(jobs,pipeline);
  }
  int pid=fork();
  if (pid==-1) {
    ERROR("fork() failed");
  }
  else if (pid==0) {
    child(r,fg,currPipeFd,newPipeFd);
  }
  else {
    // parent process: close both ends of currPipeFd and wait for the child process to finish
    if (isCurrPipeSet) {
      close(currPipeFd[0]);
      close(currPipeFd[1]);
    }
    wait(NULL);
  }
}

extern void freeCommand(Command command) {
  CommandRep r=command;
  char **argv=r->argv;
  while (*argv)
    free(*argv++);
  free(r->argv);
  free(r);
}

extern void freestateCommand() {
  if (cwd) free(cwd);
  if (owd) free(owd);
}
