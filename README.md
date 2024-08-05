# muller
Yet another chess engine. 
- simple search via depth only
- MPI capable distribution of possible moves from list, rank 0 acts as I/O UCI controller
- UCI capable
- 'd' command shows current board and state

This project was aimed at how fast modern CPUs can handle moves, memory access is therefore as limited as possible. Very compressed board representation, no hash tables. Basic materialistic eval with few bonuses that do not cost much crunch time.

In that way this little project succeeded as the nodes per second surpassed 100M/s on my laptop. But, speed is not everything for a chess engine and a good eval (NN based) plus selective depth beats this engine easily.

```sh
sudo apt install openmpi-dev
make
./run
go depth 6
```



