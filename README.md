# MC504 - Operating Systems O_o

This repository houses the first project for the OS course at UNICAMP.

Group:

* [Eduardo Rittner](https://github.com/eduardorittner)
* [Lucas Cardoso](https://github.com/lcardosott)
* [Caio Porto](https://github.com/lcaioporto)

## Project

We must implement a solution to a synchronization problem utilizing semaphores
and mutexes as synchronization primitives. Our problem of choice was the
search-insert-delete problem from the Little Book of Semaphores (section 6.1).
The premise is simple: Given a simple linked list, implement concurrent
searcher, inserters and deleters such that:
* Searchers may execute concurrently with each other
* Searchers may execute concurrently with inserters, but not deleters
* At most one inserter can be active at any point in time
* A deleter can only run if there are no searchers and inserters active
* Only one deleter can run at a time

## Compiling and running

To compile this project run `make` on the repo's root, and the compiled binary
will be saved in `build/main`. To compile and immediately run, one can use
`make run`. To compile and run tests, run `make test`. If you wish to compile
tests without running them, `cd` into `test` and run `make`, the test binary
will be stored in `test/build/test`

The default compiler is gcc, to use a different compiler, change the `CC`
variable in the Makefile to be whatever you want. This project has been tested
to work with clang and tinycc in addition to gcc.

## Our solution

In order to solve the problem, we implemented a linked list and its operations:
find, push_back and delete. These functions are simple and don't concern
themselves with any synchronization, which must be handled by their callers.

To simplify the implementation, we have considered that the linked list is
composed only by positive integers. By default, the list is initialized with 10
random elements ranging from 1 to 20, and the program will create 5 threads of
each type (searchers, inserters and deleters). To adjust the project for
different list sizes, thread counts, or value ranges, simply modify the
constants `INITIAL_SIZE`, `SEARCHERS`, `INSERTERS`, `DELETERS` and
`RANDOM_UPPER_BOUND` in the `main.c` file.

For synchronization, we use mutexes, semaphores and atomic integers. Our
linked-list definition is:

```c
typedef struct {
    lnode* head;
    sem_t no_searcher;
    sem_t no_inserter;
    pthread_mutex_t searcher_mutex;
    atomic_int searcher_count;
    state st;
} llist;
```

`head` points to the first node of the linked list, and `state` is a struct
which holds internal state used for debugging and printing the overall state
accross all active threads.

`searcher_count` is used to store the number of active searchers. Whenever a
searcher enters or leaves, it needs to first lock `searcher_mutex`, update
`searcher_count` and unlock `searcher_mutex`. Special care must be taken when
the first searcher enters (`searcher_count == 1`) and when the last searcher
leaves (`searcher_count == 0`), since they must `wait` and `post` the
`no_searcher` semaphore, respectively.

`no_inserter` is a semaphore which can be held by only one inserter or deleter
at a time.

While a deleter is running, it locks both `no_inserter` and `no_searcher`,
preventing any searchers or inserters from running. The Little Book of
Semaphores' solution proposes a third mutex meant for the deleter, so no two
deleters access the list concurrently. However, this is not necessary since we
can think of `no_inserter` and `no_searcher` combined as the deleter mutex,
since if a deleter holds both, then no other deleter will be able to run, since
they're already locked.

The `state` struct definition is shown below.

```c
typedef struct {
    int_list searchers;
    atomic_int searchers_waiting;
    size_t inserters;
    atomic_int inserters_waiting;
    size_t deleters;
    atomic_int deleters_waiting;
    pthread_mutex_t lock; }
state;
```

`searchers` is a list of integers representing the values being searched by the
searcher threads. Its size informs the number of active searcher threads - an
empty list indicates that no searcher threads are running.

`searchers_waiting`, `inserters_waiting` and `deleters_waiting` store the
number of searcher, inserter and deleter threads that are waiting for a
semaphore or mutex lock to run.

`inserters` and `deleters` stores either 0, indicating that no inserter or
deleter thread is running, or the value being inserted or deleted by the active
thread. Notably, here we use the premise that at most only one inserter or
deleter thread may execute at a time.

`lock` is a mutex used to ensure that only one thread is updating the state at
a time.

In order to better visualize the states and simulate the threads' work, we have
included a `sleep()` of three seconds after the thread acquires the necessary
conditions to run, but before it actually performs its operation (search,
insert or delete).

Given all these information, our program constantly prints the project state
while running. This is implemented by the `state_print` function defined within
`workers.c`, and has the following pattern:

```
[LINKED-LIST] STATUS: [Searching, Inserting, Deleting] {value} WAITING
QUEUE: Searchers waiting: {number_of_searchers_waiting} Inserters waiting:
{number_of_inserters_waiting} Deleters waiting: {number_of_deleters_waiting}
[If a thread finished at the current time] RESULT: [search] The value {value}
is not on the list; or The value {value} was found on the list; [insert] The
value {value} was inserted! [delete] The value {value} was not deleted because
it is not in the list; or The value {value} was deleted!
```

## Code organization

These are the main code files:
* `sync.c (.h)`: Wrappers around sem_* functions and pthread_mutex_* functions
for more cohesive naming (acquire/release) and error detection. On error they
print to stderr and exit the thread.
* `linked-list.c (.h)`: Linked list data structure and associated functions.
The find, delete and push_back functions are defined and implemented here.
* `workers.c (.h)`: Worker thread functions (those which are passed to
pthread_create), as well as worker-specific acquire/release function pairs. It
also includes the state print function.
* `sched.c (.h)`: Logic for orchestrating runs by creating an initial list and
then starting searchers, inserters and deleters at random, in hopes of testing
more of the problem state. The total number of searchers, inserters, deleters
and initial list size are all parameters which can be changed.
* `int-list.c (.h)`: Implements an integer list used for debug purposes to know
which values are being searched at a given time.

## Tests

Tests are stored in the `test` directory, we use the [Greatest C testing
framework](https://github.com/silentbicycle/greatest) for running our tests.

Tests are divided into two suites: `llist_suite` and `sync_suite`. The
`llist_suite` contains basic sanity checks that our linked list implementation
works as intented, they do not exercise any synchronization/parallel codepaths.
The `sync_suite` has more interesting tests, where we create a list, have a
worker acquire it, and then assert that the problem invariant still holds. For example,
in the `concurrent_inserters` test, we acquire the list with an `inserter`, and then
try to acquire the list with another `inserter`, asserting that the operation fails.
||||||| parent of 59bd8a4 (readme: hard-wrap lines to 80 characters)
pthread_create), as well as worker-specific acquire/release function pairs. It also includes the state print function.
* `sched.c (.h)`: Logic for orchestrating runs by creating an initial list and then
starting searchers, inserters and deleters at random, in hopes of testing more of
the problem state. The total number of searchers, inserters, deleters and
initial list size are all parameters which can be changed.
* `int-list.c (.h)`: Implements an integer list used for debug purposes to know which values are being searched at a given time.
