#define SIZE_NAME 20
#define SIZE_POST 6

typedef struct {
	int voter_id;
	char first_name[SIZE_NAME];
	char last_name[SIZE_NAME];
	char postcode[SIZE_POST];
} Record;

// Check if the memory is allocated and malloc didn't fail
#define CHECK_MALLOC_NULL(p)  \
if ((p) == NULL) {  \
	perror("Cannot allocate memory!\n"); \
	exit(1);  \
};

// Check if the file was opened
#define CHECK_FILE(f)  \
if ((f) == -1) {  \
	perror("Cannot open file!\n");  \
	exit(1);  \
};
