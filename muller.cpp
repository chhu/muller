#include <chrono>
#include <future>
#include <vector>
#include <set>
#include <list>
#include <algorithm>
#include <iostream>
#include <stdint.h>
#include <bit>
#include <bitset>
#include <ctime>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <map>
#include <array>
#include <mpi.h>
// This line **must** come **before** including <time.h> in order to bring in
// the POSIX functions such as `clock_gettime()`, `nanosleep()`, etc., from
// `<time.h>`!
//#define _POSIX_C_SOURCE 199309L

// For `nanosleep()`:
#include <time.h>



using namespace std;
using std::chrono::high_resolution_clock;
using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

// Engine setup
#define ENG_AB_CUT                   // Enable alpha-beta branch cut
#define ENG_ORDER_MOVES              // Order moves for highest capture first to aid branch cuts
#define ENG_POS_SCORE_ACCURACY 0    // Value subtracted for branch cut condition: 0=full accuracy, 100=up to a pawn loss inaccurate
#define ENG_MOVE_DEVIATION 50       // move deviation if draw by rep is threatened

typedef uint16_t Move;
void printMove(uint16_t m);

int crank;
int cpu_count;
MPI_Win eng_halt_win;

#include "engine.hpp"
#include "tools.hpp"
#include "game.hpp"
#include "uci.hpp"


int main(int argc, char *argv[]) {

	// MPI Setup
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &cpu_count);
	MPI_Comm_rank(MPI_COMM_WORLD, &crank);

	MPI_Win_allocate(sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, (void *)&engine_halt, &eng_halt_win);
//	MPI_Win_create((void *)&engine_halt, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &eng_halt_win);
	MPI_Win_fence(0, eng_halt_win);

	if(cpu_count < 2) {
	   printf("This application is meant to be run with at least 2 MPI processes\n");
	   MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
	}
	if (crank > 0) { // worker process
        Game g;
        g.receiverLoop(); // never returns
    } else
    	UCIloop(argc, argv);

	MPI_Abort(MPI_COMM_WORLD, 0);
	return 0;

	// Play / test w/o UCI, comment UCIloop and uncomment this to have the engine auto-play against each other
    Game grr = Game();
    // mate in 11: "3k4/8/2K5/2B5/2B5/8/8/8 w - - 0 1"
    // mate in  5: "3nk3/8/3B1K2/8/8/6Q1/8/8 w - - 0 1"
    // mate in 15: "8/1k6/3R4/3K4/8/5n2/8/8 w - - 0 1"
    limits.debug_mainline = false;
    int wd = 7;
    int bd = 7;
    while (1) {
        grr.current.print();
        limits.mate_search = 0;
        int &depth = grr.white_to_move ? wd : bd;
        limits.pos_score_enabled = true;//grr.white_to_move; // only white gets pos scoring
        grr.startSearchMPI(depth);
        while (!grr.processSearchQ()) {
        	sleep_ms(10);
        }
        if (since(grr.last_search_start) < 100)
        	depth++;
        if (since(grr.last_search_start) > 5000)
        	depth--;

        auto move = grr.selectMove(grr.last_search_result);
        if (move.move == 0)
        	break; // mate or stale
        cout << grr.board_history.size() << ": R"<< grr.checkRepetition() << " WD"<<wd<<" BD"<<bd <<"; EXECUTED: "; printMove(move);cout << endl;
        grr.execMove(move);
        if (grr.checkRepetition() > 5) {
            cout << "DRAW!\n";
            break;
        }
    }
    return 0;
}
