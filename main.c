// Code by Samuel Jamieson for Operating Systems 1 - CS374
// ONID: 934325118

#include <stdio.h>        // Input and Output (I/O) Library
#include <stdlib.h>       // Functions for memory allocation and process control
#include <string.h>       // Manipulation for standard C strings
#include <dirent.h>       // For directory handling
#include <sys/stat.h>     // For directory and file permissions
#include <sys/types.h>    // For mode_t type
#include <errno.h>        // Error handling
#include <time.h>         // Random number generation
#include <unistd.h>       // For file existence check

char *strdup(const char *s);

// Define a struct for movie
typedef struct movie {
    char *title;
    int year;
    char *languages;
    double rating;
    struct movie *next;
} Movie;

// Function declarations
Movie* create_movie(char *title, int year, char *languages, double rating);
void add_movie(Movie **head, char *title, int year, char *languages, double rating);
void free_movies(Movie *head);
int process_file(const char *filename, Movie **head);
void create_directory_and_files(Movie *head, const char *file_name);
void select_file_to_process();
void process_largest_file();
void process_smallest_file();
void process_specified_file();
void set_file_permissions(const char *path, mode_t mode);

// Custom strdup if not available in system
char *strdup(const char *s) {
    char *d = malloc(strlen(s) + 1);
    if (d == NULL) return NULL; // malloc failed
    strcpy(d, s);
    return d;
}

// Function definitions

// Create a movie struct
Movie* create_movie(char *title, int year, char *languages, double rating) {
    Movie *new_movie = (Movie*)malloc(sizeof(Movie));
    new_movie->title = strdup(title);
    new_movie->year = year;
    new_movie->languages = strdup(languages);
    new_movie->rating = rating;
    new_movie->next = NULL;
    return new_movie;
}

// Add a new movie to the linked list
void add_movie(Movie **head, char *title, int year, char *languages, double rating) {
    Movie *new_movie = create_movie(title, year, languages, rating);
    if (*head == NULL) {
        *head = new_movie;
    } else {
        Movie *temp = *head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_movie;
    }
}

// Frees allocated memory for the linked list of movies
void free_movies(Movie *head) {
    Movie *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp->title);
        free(temp->languages);
        free(temp);
    }
}

// Processes the CSV file and populates the linked list
int process_file(const char *filename, Movie **head) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Could not open file %s\n", filename);
        return -1;
    }

    char line[1024];
    int movie_count = 0;
    fgets(line, sizeof(line), file);  // Skip the header line

    while (fgets(line, sizeof(line), file)) {
        char *title = strtok(line, ",");
        int year = atoi(strtok(NULL, ","));
        char *languages = strtok(NULL, ",");
        double rating = atof(strtok(NULL, ","));

        add_movie(head, title, year, languages, rating);
        movie_count++;
    }

    fclose(file);
    printf("Processed file %s and parsed data for %d movies\n", filename, movie_count);
    return movie_count;
}

// Create a directory, set permissions, and write the movie data to year-specific files
void create_directory_and_files(Movie *head, const char *file_name) {
    char dir_name[256];
    int random_number = rand() % 100000; // Generate a random number
    sprintf(dir_name, "jamiessa.movies.%d", random_number); // Update with ONID

    // Create the directory with specific permissions (rwxr-x---)
    if (mkdir(dir_name, 0750) != 0) {
        perror("Failed to create directory");
        return;
    }
    printf("Created directory with name %s\n", dir_name);

    // Process movies by year and write to files in the directory
    Movie *temp = head;
    while (temp != NULL) {
        char file_path[512];
        sprintf(file_path, "%s/%d.txt", dir_name, temp->year);

        FILE *year_file = fopen(file_path, "a"); // Append to the file for that year
        if (year_file == NULL) {
            perror("Failed to create file");
            return;
        }

        fprintf(year_file, "%s\n", temp->title); // Write the movie title
        fclose(year_file);

        // Set file permissions (rw-r-----)
        set_file_permissions(file_path, 0640);

        temp = temp->next;
    }
}

// Set file permissions for a specific file
void set_file_permissions(const char *path, mode_t mode) {
    if (chmod(path, mode) != 0) {
        perror("Failed to set file permissions");
    }
}

// Function to select a file to process
void select_file_to_process() {
    int choice;
    printf("\nWhich file do you want to process?\n");
    printf("Enter 1 to pick the largest file\n");
    printf("Enter 2 to pick the smallest file\n");
    printf("Enter 3 to specify the name of a file\n");
    printf("Enter a choice from 1 to 3: ");
    scanf("%d", &choice);

    if (choice == 1) {
        process_largest_file();
    } else if (choice == 2) {
        process_smallest_file();
    } else if (choice == 3) {
        process_specified_file();
    } else {
        printf("Invalid choice. Please try again.\n");
    }
}

// Function to process the largest file
void process_largest_file() {
    DIR *d;
    struct dirent *dir;
    struct stat st;
    char *largest_file = NULL;
    long largest_size = -1;

    d = opendir(".");
    if (d == NULL) {
        perror("Could not open current directory");
        return;
    }

    while ((dir = readdir(d)) != NULL) {
        if (strncmp(dir->d_name, "movies_", 7) == 0 && strstr(dir->d_name, ".csv") != NULL) {
            if (stat(dir->d_name, &st) == 0 && S_ISREG(st.st_mode)) {
                if (st.st_size > largest_size) {
                    largest_size = st.st_size;
                    largest_file = dir->d_name;
                }
            }
        }
    }
    closedir(d);

    if (largest_file != NULL) {
        printf("Now processing the largest file named %s\n", largest_file);
        Movie *head = NULL;
        process_file(largest_file, &head);
        create_directory_and_files(head, largest_file);
        free_movies(head);
    } else {
        printf("No suitable file found.\n");
    }
}

// Function to process the smallest file
void process_smallest_file() {
    DIR *d;
    struct dirent *dir;
    struct stat st;
    char *smallest_file = NULL;
    long smallest_size = -1;

    d = opendir(".");
    if (d == NULL) {
        perror("Could not open current directory");
        return;
    }

    while ((dir = readdir(d)) != NULL) {
        if (strncmp(dir->d_name, "movies_", 7) == 0 && strstr(dir->d_name, ".csv") != NULL) {
            if (stat(dir->d_name, &st) == 0 && S_ISREG(st.st_mode)) {
                if (smallest_size == -1 || st.st_size < smallest_size) {
                    smallest_size = st.st_size;
                    smallest_file = dir->d_name;
                }
            }
        }
    }
    closedir(d);

    if (smallest_file != NULL) {
        printf("Now processing the smallest file named %s\n", smallest_file);
        Movie *head = NULL;
        process_file(smallest_file, &head);
        create_directory_and_files(head, smallest_file);
        free_movies(head);
    } else {
        printf("No suitable file found.\n");
    }
}

// Function to process a user-specified file
void process_specified_file() {
    char file_name[256];
    printf("Enter the complete file name: ");
    scanf("%s", file_name);

    // Check if the file exists
    if (access(file_name, F_OK) != 0) {
        printf("The file %s was not found. Try again.\n", file_name);
        return;
    }

    // Process the specified file
    printf("Now processing the chosen file named %s\n", file_name);
    Movie *head = NULL;
    process_file(file_name, &head);
    create_directory_and_files(head, file_name);
    free_movies(head);
}

// Main function with the main menu
int main() {
    srand(time(NULL)); // Seed the random number generator
    int choice = 0;

    while (1) {
        printf("\n1. Select file to process\n");
        printf("2. Exit the program\n");
        printf("Enter a choice 1 or 2: ");
        scanf("%d", &choice);

        if (choice == 1) {
            select_file_to_process();
        } else if (choice == 2) {
            printf("Exiting the program.\n");
            exit(0);
        } else {
            printf("Invalid choice. Please try again.\n");
        }
    }

    return 0;
}

