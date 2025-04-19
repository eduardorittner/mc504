#include "sched.h"

#define INITIAL_SIZE 10
#define SEARCHERS 5
#define INSERTERS 5
#define DELETERS 5

int main(void) {
  run_cfg run = run_cfg_new(INITIAL_SIZE, SEARCHERS, INSERTERS, DELETERS);
  run_cfg_run(&run);
}
