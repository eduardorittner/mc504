# MC504 - Operating Systems O_o

This repository houses the first project for the OS course at UNICAMP.

Group:

* [Eduardo Rittner](https://github.com/eduardorittner)
* [Lucas Cardoso](https://github.com/lcardosott)
* [Caio Porto](https://github.com/lcaioporto)

## Project

We must implement a solution to a synchronization problem utilizing semaphores
and mutexes as synchronization primitives. Our problem of choice was the
search-insert-delete problem from the Little Book of Semaphores. The premise is
simple: Given a simple linked list, implement concurrent searcher, inserters
and deleters such that:
* Searchers may execute concurrently with each other
* Searchers may execute concurrently with inserters, but not deleters
* At most one inserter can be active at any point in time
* A deleter can only run if there are no searchers and inserters active

## Compiling and running

To compile this project run `make` on the repo's root, and the compiled binary
will be saved in `build/main`. To compile and immediately run, one can use
`make run`. By default debug information is printed, to disable it comment the
`#define LOG` line in `workers.h`.

The default compiler is gcc, to use a different compiler, change the `CC`
variable in the Makefile to be whatever you want. This project has been tested
to work with clang and tinycc in addition to gcc.

## Our solution

In order to solve the problem, we implemented a linked list and its operations:
find, push_back and delete. These functions are simple and don't concern
themselves with any synchronization, which must be handled by their callers.

For synchronization, we use mutexes, semaphores and atomic integers, our
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

`head` points to the first node of the linked list, and `state` is internal
state used for debugging and printing current state.

`searcher_count` is used to store the number of active searchers. Whenever a
searcher enters or leaves, it needs to first lock `searcher_mutex`, update
`searcher_count` and unlock `searcher_mutex`. Special care must be taken when
the first searcher enters (`searcher_count == 1`) and when the last searcher
leaves (`searcher_count == 0`), since they must `wait` and `post` the
`no_searcher` semaphore, respectively.

`no_inserter` is a semaphore which cna be held by only on inserter or deleter
at a time.

While a deleter is running, it locks both `no_inserter` and `no_searcher`,
preventing any searchers or inserters from running.

## Code organization

These are the main code files:
* sync.c (.h): Wrappers around sem_* functions and pthread_mutex_* functions
for more cohesive naming (acquire/release) and error detection. On error they
print to stderr and exit the thread.
* linked-list.c (.h): Linked list data structure and associated functions. The
find, delete and push_back functions are defined and implemented here.
* workers.c (.h): Worker thread functions (those which are passed to
pthread_create), as well as worker-specific acquire/release function pairs.
* sched.c (.h): Logic for orchestrating runs, creates an initial list and then
starts searchers, inserters and deleters at random, in hopes of testing more of
the problem state. The total number of searchers, inserters, deleters and
initial list size are all parameters which can be changed.
