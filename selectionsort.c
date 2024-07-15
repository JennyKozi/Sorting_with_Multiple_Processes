#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
# include <sys/times.h>
#include "header.h"

void main (int argc, char *argv[]) {

	// Variables to count time
	double cpu_time, t1, t2, ret_realtime, ret_cputime, tics_per_sec;
	struct tms tb1, tb2;
	int stop = -1;

	tics_per_sec = (double)sysconf(_SC_CLK_TCK);
	t1 = (double)times(&tb1);

	// Check number of arguments
	if (argc != 6) {
		perror("Wrong number of arguments for selection sort!\n");
		exit(1);
	}

	// Open file
	int rp;
	CHECK_FILE(rp = open(argv[1], O_RDONLY));

	// Set file pointer
	int pointer = atoi(argv[2]);
	lseek(rp, pointer, SEEK_SET);

	// Number of records that will be sorted
	int num_records = atoi(argv[3]);

	// Pipe
	int pipe = atoi(argv[4]);

	// Root pid
	pid_t root_pid = atoi(argv[5]);

	// Read the file
	Record *array;
	Record rec;

	CHECK_MALLOC_NULL(array = malloc(num_records * sizeof(Record)));

	for (int i = 0; i < num_records; i++) {
		read(rp, &rec, sizeof(rec));
		array[i] = rec;
	}

	// Sort the records
	for (int i = 0; i < num_records; i++) {
		int index = i;
		for (int j = i + 1; j < num_records; j++) {
			if (strcmp(array[j].last_name, array[index].last_name) < 0) {
				index = j;
			}
			// Same last name
			else if (strcmp(array[j].last_name, array[index].last_name) == 0) {
				if (strcmp(array[j].first_name, array[index].first_name) < 0) {
					index = j;
				}
				// Same last name and first name
				else if (strcmp(array[j].first_name, array[index].first_name) == 0) {
					if (array[j].voter_id < array[index].voter_id)
						index = j;
				}
			}
		}
		rec = array[i];
		array[i] = array[index];
		array[index] = rec;
	}

	// Write the sorted array in the pipe
	for (int i = 0; i < num_records; i++) {
		write(pipe, &array[i].voter_id, sizeof(int));
		write(pipe, array[i].first_name, sizeof(rec.first_name));
		write(pipe, array[i].last_name, sizeof(rec.last_name));
		write(pipe, array[i].postcode, sizeof(rec.postcode));
	}
	write(pipe, &stop, sizeof(int));

	close(rp); // Close file pointer for sorter
	free(array); // Free memory

	// Calculate time
	t2 = (double)times(&tb2);
	cpu_time = (double)((tb2.tms_utime + tb2.tms_stime) - (tb1.tms_utime + tb1.tms_stime));
	ret_realtime = (double)((t2 - t1) / tics_per_sec);
	ret_cputime = (double)(cpu_time / tics_per_sec);

	write(pipe, &ret_realtime, sizeof(double)); // Return real time for sorter
	write(pipe, &ret_cputime, sizeof(double)); // Return CPU time for sorter

	close(pipe); // Close write end for sorter
	kill(root_pid, SIGUSR2); // Send signal USR2 to root

	exit(0);
}
