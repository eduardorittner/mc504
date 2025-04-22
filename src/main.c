#include "sched.h"

#define INITIAL_SIZE 10
#define SEARCHERS 5
#define INSERTERS 5
#define DELETERS 5
#define RANDOM_UPPER_BOUND 20

int main(void) {
  run_cfg run = run_cfg_new(INITIAL_SIZE, SEARCHERS, INSERTERS, DELETERS, RANDOM_UPPER_BOUND);
  run_cfg_run(&run, RANDOM_UPPER_BOUND);
}
