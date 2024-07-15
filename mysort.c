#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "header.h"

#define READ_END 0
#define WRITE_END 1

void usr1_handler();
void usr2_handler();
static int Check_Int(char *);
void merge(Record **, int, int, int);

// Global variables to count signals received by root
int count_usr1 = 0, count_usr2 = 0;

int main(int argc, char *argv[]) {

	// MYSORT

	// Check number of arguments
	if (argc != 9) {
		perror("Wrong number of arguments!\n");
		exit(1);
	}

	int i, j, k, total_records, rp, stat, file_size, status, stop = -1;
	double time;
	char *data_file, *sort1, *sort2, *prog1, *prog2;
	Record rec;
	struct stat buffer;
	pid_t root_pid = getpid(); // pid of the process root

	// Check that the flags are correct
	for (i = 1; i <= 7; i += 2) {

		// File with data
		if (strcmp("-i", argv[i]) == 0) {
			data_file = argv[i + 1];
		}
		
		// k children
		else if (strcmp("-k", argv[i]) == 0) {
			if (Check_Int(argv[i + 1]) == 1) {
				k = atoi(argv[i + 1]);
			}
			else {
				perror("k must a positive integer!\n");
				exit(1);
			}
			if (k < 1) {
				perror("k must a positive integer!\n");
				exit(1);
			}
		}

		// Sorting program 1
		else if (strcmp("-e1", argv[i]) == 0) {
			sort1 = argv[i + 1];
		}

		// Sorting program 2
		else if (strcmp("-e2", argv[i]) == 0) {
			sort2 = argv[i + 1];
		}
		else {
			perror("Error with flags!\n");
			exit(1);
		}
	}

	// Preparing the strings for the execution of the programs
	CHECK_MALLOC_NULL(prog1 = malloc((strlen(sort1) + 3) * sizeof(char)));
	CHECK_MALLOC_NULL(prog2 = malloc((strlen(sort2) + 3) * sizeof(char)));
	strcpy(prog1, "./");
	strcat(prog1, sort1);
	strcpy(prog2, "./");
	strcat(prog2, sort2);

	CHECK_FILE(rp = open(data_file, O_RDONLY)); // Open binary data file

	// Get number of records
	stat = fstat(rp, &buffer);
	if (stat != 0) {
		perror("Error reading file!\n");
		close(rp);
		exit(1);
	}
	close(rp); // Close file pointer for root
	file_size = buffer.st_size;
	total_records = (int)file_size / sizeof(rec); // Total records in the file

	int load_splitter = total_records / k; // Number of records for one splitter
	int remaining_load_splitters = total_records % k; // The first children (spliters) will get one extra record
	int total_sorters = k * (k + 1) / 2; // Number of total sorters

	// Useful info for the splitters
	pid_t *splitters;
	int *splitters_pointers, *splitters_numof_records, **splitters_pipes;

	CHECK_MALLOC_NULL(splitters = malloc(k * sizeof(pid_t)));
	CHECK_MALLOC_NULL(splitters_pointers = malloc(k * sizeof(int)));
	CHECK_MALLOC_NULL(splitters_numof_records = malloc(k * sizeof(int)));
	CHECK_MALLOC_NULL(splitters_pipes = malloc(k * sizeof(int *)));

	// Create pipes for splitters
	for (i = 0; i < k; i++) {
		CHECK_MALLOC_NULL(splitters_pipes[i] = malloc(2 * sizeof(int)));
		if (pipe(splitters_pipes[i]) < 0) {
			perror("Error with pipe!\n");
			exit(1);
		}
	}
	
	// Number of records for the splitters
	for (i = 0; i < k; i++) {
		if (remaining_load_splitters == 0) {
			splitters_numof_records[i] = load_splitter;
		}
		else {
			if (i < remaining_load_splitters)
				splitters_numof_records[i] = load_splitter + 1;
			else
				splitters_numof_records[i] = load_splitter;
		}
	}

	// File pointers for each splitter
	splitters_pointers[0] = 0;
	
	for (i = 1; i < k; i++) {
		splitters_pointers[i] = splitters_pointers[i - 1] + (splitters_numof_records[i - 1] * sizeof(Record));
	}

	// Create k splitters with fork
	for (i = 0; i < k; i++) {

		splitters[i] = fork();
		if (splitters[i] == -1) {
			perror("Failed to fork!\n");
			exit(1);
		}

		// SPLITTERS
		if (splitters[i] == 0) {

			close(splitters_pipes[i][READ_END]); // Close read end for splitter (splitter - root)

			// Number of sorters this splitter has
			int numof_sorters = k - i;
			int load_sorter = splitters_numof_records[i] / numof_sorters; // The first children (spliters) will get one extra record
			int remaining_load_sorters = splitters_numof_records[i] % numof_sorters; // The first children (spliters) will get one extra record

			// Useful info for the sorters of this splitter
			pid_t *sorters;
			int *sorters_pointers, *sorters_numof_records, **sorters_pipes;
			double **sorters_time;
			CHECK_MALLOC_NULL(sorters = malloc(numof_sorters * sizeof(pid_t)));
			CHECK_MALLOC_NULL(sorters_pointers = malloc(numof_sorters * sizeof(int)));
			CHECK_MALLOC_NULL(sorters_numof_records = malloc(numof_sorters * sizeof(int)));
			CHECK_MALLOC_NULL(sorters_pipes = malloc(numof_sorters * sizeof(int *)));
			CHECK_MALLOC_NULL(sorters_time = malloc(numof_sorters * sizeof(double *)));

			// Matrix for real time and cpu time of each sorter
			for (j = 0; j < numof_sorters; j++) {
				CHECK_MALLOC_NULL(sorters_time[j] = malloc(2 * sizeof(double)));
			}

			// Create pipes for splitters
			for (j = 0; j < numof_sorters; j++) {
				CHECK_MALLOC_NULL(sorters_pipes[j] = malloc(2 * sizeof(int)));
				if (pipe(sorters_pipes[j]) < 0) {
					perror("Error with pipe!\n");
					exit(1);
				}
			}

			// Number of records for the sorters
			for (j = 0; j < numof_sorters; j++) {
				if (remaining_load_sorters == 0) {
					sorters_numof_records[j] = load_sorter;
				}
				else {
					if (j < remaining_load_sorters)
						sorters_numof_records[j] = load_sorter + 1;
					else
						sorters_numof_records[j] = load_sorter;
				}
			}

			// File pointers for each sorter
			sorters_pointers[0] = splitters_pointers[i];
	
			for (j = 1; j < numof_sorters; j++) {
				sorters_pointers[j] = sorters_pointers[j - 1] + (sorters_numof_records[j - 1] * sizeof(Record));
			}

			// Create sorters of this splitter with fork
			for (j = 0; j < numof_sorters; j++) {

				sorters[j] = fork();
				if (sorters[j] == -1) {
					perror("Failed to fork!\n");
					exit(1);
				}

				// SORTERS
				if (sorters[j] == 0) {

					close(sorters_pipes[j][READ_END]); // Close read end for sorter (sorter - splitter)

					char pointer[10], num[10], pipe[10], pid[10];
					snprintf(pointer, sizeof(pointer), "%d", sorters_pointers[j]);
					snprintf(num, sizeof(num), "%d", sorters_numof_records[j]);
					snprintf(pipe, sizeof(pipe), "%d", sorters_pipes[j][WRITE_END]);
					snprintf(pid, sizeof(pid), "%d", root_pid);

					// Execute a sorting algorithm (choose an algorithm)
					if (j % 2 == 0) {
						execl(prog1, prog1, data_file, pointer, num, pipe, pid, (char *)NULL);
						perror("exec failure!\n");
						exit(1);
					}
					else {
						execl(prog2, prog2, data_file, pointer, num, pipe, pid, (char *)NULL);
						perror("exec failure!\n");
						exit(1);
					}
				}

				// Splitter
				else {
					close(sorters_pipes[j][WRITE_END]); // Close write end for splitter (splitter - sorter)
				}
			}

			// Parent process: SPLITTER
			Record **sorters_results;
			CHECK_MALLOC_NULL(sorters_results = malloc(numof_sorters * sizeof(Record *)));
			for (j = 0; j < numof_sorters; j++) {
				CHECK_MALLOC_NULL(sorters_results[j] = malloc(sorters_numof_records[j] * sizeof(Record)));
			}

			// Read the sorted records from pipe (sorter -> splitter)
			for (j = 0; j < numof_sorters; j++) {
				waitpid(sorters[j], &status, WNOHANG); // Wait for sorter process to finish
				int count = 0;
				while (read(sorters_pipes[j][READ_END], &rec.voter_id, sizeof(int)) > 0 && rec.voter_id != -1) {
					read(sorters_pipes[j][READ_END], rec.first_name, sizeof(rec.first_name));
					read(sorters_pipes[j][READ_END], rec.last_name, sizeof(rec.last_name));
					read(sorters_pipes[j][READ_END], rec.postcode, sizeof(rec.postcode));
					sorters_results[j][count++] = rec;
				}
				// Read time from sorter
				read(sorters_pipes[j][READ_END], &sorters_time[j][0], sizeof(double)); // Real time
				read(sorters_pipes[j][READ_END], &sorters_time[j][1], sizeof(double)); // CPU time
				close(sorters_pipes[j][READ_END]); // Close read end for splitter (splitter - sorter)
			}

			// Merge sorted records
			Record *final_result;
			int prev_size = 0;
			int current_size = sorters_numof_records[0];
			CHECK_MALLOC_NULL(final_result = malloc(current_size * sizeof(Record)));
			for (j = 0; j < current_size; j++) {
				final_result[j] = sorters_results[0][j];
			}
	
			for (j = 1; j < numof_sorters; j++) {
				int count = 0;
				prev_size = current_size;
				current_size += sorters_numof_records[j];
				final_result = realloc(final_result, current_size * sizeof(Record));
				for (int h = prev_size; h < current_size; h++) {
					final_result[h] = sorters_results[j][count++];
				}
				merge(&final_result, 0, prev_size - 1,  current_size - 1);
			}

			// Write the sorted array in the pipe (splitter -> root)
			for (j = 0; j < splitters_numof_records[i]; j++) {
				write(splitters_pipes[i][WRITE_END], &final_result[j].voter_id, sizeof(int));
				write(splitters_pipes[i][WRITE_END], final_result[j].first_name, sizeof(rec.first_name));
				write(splitters_pipes[i][WRITE_END], final_result[j].last_name, sizeof(rec.last_name));
				write(splitters_pipes[i][WRITE_END], final_result[j].postcode, sizeof(rec.postcode));
			}
			write(splitters_pipes[i][WRITE_END], &stop, sizeof(int));

			// Write the time of each sorter (splitter -> root)
			for (j = 0; j < numof_sorters; j++) {
				write(splitters_pipes[i][WRITE_END], &sorters_time[j][0], sizeof(double)); // Real time
				write(splitters_pipes[i][WRITE_END], &sorters_time[j][1], sizeof(double)); // CPU time
			}
			close(splitters_pipes[i][WRITE_END]); // Close write end for splitter (splitter - root)

			// Free memory allocated by splitter
			free(sorters);
			free(sorters_pointers);
			free(sorters_numof_records);
			for (j = 0; j < numof_sorters; j++) {
				free(sorters_pipes[j]);
			}
			free(sorters_pipes);
			for (j = 0; j < numof_sorters; j++) {
				free(sorters_results[j]);
			}
			free(sorters_results);
			for (j = 0; j < numof_sorters; j++) {
				free(sorters_time[j]);
			}
			free(sorters_time);
			free(final_result);

			// Free memory allocated by root
			free(prog1);
			free(prog2);
			free(splitters);
			free(splitters_pointers);
			free(splitters_numof_records);
			for (i = 0; i < k; i++) {
				free(splitters_pipes[i]);
			}
			free(splitters_pipes);

			kill(root_pid, SIGUSR1); // Send signal USR1 to root
			exit(0); // End splitter process
		}

		// Root
		else {
			signal(SIGUSR1, usr1_handler); // Set signal handler for root (USR1)
			signal(SIGUSR2, usr2_handler); // Set signal handler for root (USR2)
			close(splitters_pipes[i][WRITE_END]); // Close write end for root (root - splitter)
		}
	}

	// Parent process: MYSORT
	double **splitters_time;
	Record **splitters_results;

	// Matrix for real time and CPU time of each sorter
	CHECK_MALLOC_NULL(splitters_time = malloc(total_sorters * sizeof(double *)));
	for (i = 0; i < total_sorters; i++) {
		CHECK_MALLOC_NULL(splitters_time[i] = malloc(2 * sizeof(double)));
	}

	// Matrix for the results of the splitters
	CHECK_MALLOC_NULL(splitters_results = malloc(k * sizeof(Record *)));
	for (i = 0; i < k; i++) {
		CHECK_MALLOC_NULL(splitters_results[i] = malloc(splitters_numof_records[i] * sizeof(Record)));
	}

	// Read the sorted records from pipes (splitter -> root)
	int t = 0;
	for (i = 0; i < k; i++) {
		waitpid(splitters[i], &status, WNOHANG); // Wait for splitter process to finish
		int count = 0;
		while (read(splitters_pipes[i][READ_END], &rec.voter_id, sizeof(int)) > 0 && rec.voter_id != -1) {
			read(splitters_pipes[i][READ_END], rec.first_name, sizeof(rec.first_name));
			read(splitters_pipes[i][READ_END], rec.last_name, sizeof(rec.last_name));
			read(splitters_pipes[i][READ_END], rec.postcode, sizeof(rec.postcode));
			splitters_results[i][count++] = rec;
		}
		// Read time from splitter
		while (read(splitters_pipes[i][READ_END], &time, sizeof(double)) > 0) {
			splitters_time[t][0] = time; // Real time
			read(splitters_pipes[i][READ_END], &splitters_time[t][1], sizeof(double)); // CPU time
			t++;
		}
		close(splitters_pipes[i][READ_END]); // Close read end for root (root - splitter)
	}

	// Merge sorted records
	Record *final_result;
	int prev_size = 0;
	int current_size = splitters_numof_records[0];
	CHECK_MALLOC_NULL(final_result = malloc(current_size * sizeof(Record)));
	for (i = 0; i < current_size; i++) {
		final_result[i] = splitters_results[0][i];
	}
	
	for (i = 1; i < k; i++) {
		int count = 0;
		prev_size = current_size;
		current_size += splitters_numof_records[i];
		final_result = realloc(final_result, current_size * sizeof(Record));
		for (int j = prev_size; j < current_size; j++) {
			final_result[j] = splitters_results[i][count++];
		}
		merge(&final_result, 0, prev_size - 1,  current_size - 1);
	}
	
	// Print sorted list
	printf("\n");
	for (i = 0; i < total_records; i++) {
		printf("%-12s %-12s %-6d   %s\n", final_result[i].last_name, final_result[i].first_name, final_result[i].voter_id, final_result[i].postcode);
	}
	printf("\n");

	// Print time of sorters
	for (i = 0; i < total_sorters; i++) {
		printf("Sorter %d: Real time: %f, CPU time: %f\n", i, splitters_time[i][0], splitters_time[i][1]);
	}
	printf("\n");

	// Print number of signals received from splitters and sorters
	printf("USR1 signals received: %d\n", count_usr1);
	printf("USR2 signals received: %d\n\n", count_usr2);

	// Free memory allocated by root
	free(prog1);
	free(prog2);
	free(splitters);
	free(splitters_pointers);
	free(splitters_numof_records);
	for (i = 0; i < k; i++) {
		free(splitters_pipes[i]);
	}
	free(splitters_pipes);
	for (i = 0; i < k; i++) {
		free(splitters_results[i]);
	}
	free(splitters_results);
	for (i = 0; i < total_sorters; i++) {
		free(splitters_time[i]);
	}
	free(splitters_time);
	free(final_result);

	return 0;
}

// Check that it is an int
static int Check_Int(char *string) {

	// Check for chars
	for (int i = 0; i < strlen(string); i++) {
		if (isalpha(string[i])) {
			return 0;
		}
	}

	// Check if num is float
	float float_num = atof(string);
	int num = float_num;
	float diff = float_num - (float)num;

	if (diff != 0.0) {
		return 0;
	}
	return 1;
}

// Handlers for signals
void usr1_handler() {
	signal(SIGUSR1, usr1_handler);
	count_usr1++;
}

void usr2_handler() {
	signal(SIGUSR2, usr2_handler);
	count_usr2++;
}
