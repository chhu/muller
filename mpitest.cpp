#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
// This line **must** come **before** including <time.h> in order to bring in
// the POSIX functions such as `clock_gettime()`, `nanosleep()`, etc., from
// `<time.h>`!
#define _POSIX_C_SOURCE 199309L

// For `nanosleep()`:
#include <time.h>

void sleep_us(unsigned long microseconds)
{
    struct timespec ts;
    ts.tv_sec = microseconds / 1000000ul;            // whole seconds
    ts.tv_nsec = (microseconds % 1000000ul) * 1000;  // remainder, in nanoseconds
    nanosleep(&ts, NULL);
}

/**

 * @brief Illustrate how to put data into a target window.

 * @details This application consists of two MPI processes. MPI process 1

 * exposes a window containing an integer. MPI process 0 puts the value 12345

 * in it via MPI_Put. After the MPI_Put is issued, synchronisation takes place

 * via MPI_Win_fence and the MPI process 1 prints the value in its window.

 **/

int main(int argc, char* argv[])

{

    MPI_Init(&argc, &argv);



    // Check that only 2 MPI processes are spawn

    int comm_size;

    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    if(comm_size != 2)
    {
        printf("This application is meant to be run with 2 MPI processes, not %d.\n", comm_size);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    // Get my rank
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    printf("%d crd\n",my_rank);
    // Create the window
    volatile int *window_buffer;
    MPI_Win window;

    //MPI_Win_create(&window_buffer, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &window);
    MPI_Win_allocate(1*sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &window_buffer, &window);
    window_buffer[0] = 0;

    MPI_Win_fence(0, window);

    if(my_rank == 1) {
    	double a = 2;
    	unsigned long long it = 0;
    	while (!window_buffer[0]) {
    		a *= 1.00001;
    		it++;
            //MPI_Win_fence(0, window);
    	}
    	printf("[MPI process 1] Value in my window_buffer before MPI_Put: %g %ull.\n", a, it);
    }

    if(my_rank == 0) {

        // Push my value into the first integer in MPI process 1 window
    	int i = 0;
    	while (1) {
    		sleep_us(1000000);
            //MPI_Win_fence(0, window);
            printf("[MPI process 0] Value in my window_buffer : %d.\n", window_buffer[0]);
            if (++i == 3) {
            	int x = 1234;
                MPI_Put(&x, 1, MPI_INT, 1, 0, 1, MPI_INT, window);
            }
    	}

        int my_value = 12345;


        printf("[MPI process 0] I put data %d in MPI process 1 window via MPI_Put.\n", my_value);

    }



    // Wait for the MPI_Put issued to complete before going any further


    // Destroy the window

    MPI_Win_free(&window);



    MPI_Finalize();



    return EXIT_SUCCESS;

}
