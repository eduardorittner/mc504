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

