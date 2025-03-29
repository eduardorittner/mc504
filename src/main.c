#include "linked-list.h"
#include "sched.h"
#include "sync.h"
#include "workers.h"
#include <stdio.h>
#include <stdlib.h>

#define INITIAL_SIZE 10000
#define SEARCHERS 1000
#define INSERTERS 100
#define DELETERS 100
int main(void) {
  run_cfg run = run_cfg_new(INITIAL_SIZE, SEARCHERS, INSERTERS, DELETERS);
  run_cfg_run(&run);
}
