#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "Pipeline.h"
#include "deq.h"
#include "error.h"

typedef struct {
  Deq processes;
  int fg;			// not "&"
} *PipelineRep;

extern Pipeline newPipeline(int fg) {
  PipelineRep r=(PipelineRep)malloc(sizeof(*r));
  if (!r)
    ERROR("malloc() failed");
  r->processes=deq_new();
  r->fg=fg;
  return r;
}

extern void addPipeline(Pipeline pipeline, Command command) {
  PipelineRep r=(PipelineRep)pipeline;
  deq_tail_put(r->processes,command);
}

extern int sizePipeline(Pipeline pipeline) {
  PipelineRep r=(PipelineRep)pipeline;
  return deq_len(r->processes);
}

static void execute(Pipeline pipeline, Jobs jobs, int *jobbed, int *eof) {
  PipelineRep r=(PipelineRep)pipeline;
  
  // Declare file descriptors to iterate through the pipeline
  int currPipeFd[2], newPipeFd[2];

  // set pipe file descriptors to 0
  currPipeFd[0] = -1;
  currPipeFd[1] = -1;
  newPipeFd[0] = -1;
  newPipeFd[1] = -1;

  for (int i=0; i<sizePipeline(r) && !*eof; i++) {
    // create a new pipe if we are not at the end of the pipeline deque
    if (i<sizePipeline(r)-1) {
      // create pipe and check for errors
      if (pipe(newPipeFd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
      }
    }

    // execute the command
    execCommand(deq_head_ith(r->processes,i),pipeline,jobs,jobbed,eof,1,currPipeFd,newPipeFd);

    // set the new pipe as the current pipe
    currPipeFd[0]=newPipeFd[0];
    currPipeFd[1]=newPipeFd[1];
  }
}

extern void execPipeline(Pipeline pipeline, Jobs jobs, int *eof) {
  int jobbed=0;
  // execute the pipeline
  // this will execute each command in the pipeline in the context of the given jobs, and set jobbed to 1 if the pipeline is a job.
  execute(pipeline,jobs,&jobbed,eof);
  if (!jobbed)
    freePipeline(pipeline);	// for fg builtins, and such
}

extern void freePipeline(Pipeline pipeline) {
  PipelineRep r=(PipelineRep)pipeline;
  deq_del(r->processes,freeCommand);
  free(r);
}
