#include "local_header.h"


Matrix * matrices = NULL;
int matrix_count = 0;
int next_matrix_id = 1;  

// Flush stdin
void flush_stdin() {
  int c;
  while ((c = getchar()) != '\n' && c != EOF);
}

// Function to get a float from user
float get_float(const char * prompt) {
  float f;
  char buffer[64];
  while (1) {
    printf("%s", prompt);
    if (!fgets(buffer, sizeof(buffer), stdin)) continue;
    buffer[strcspn(buffer, "\n")] = 0;
    if (sscanf(buffer, "%f", & f) == 1) return f;
    printf("Invalid input. Enter a number.\n");
  }
}

// Function to get a positive integer from user
int get_positive_int(const char * prompt) {
  int n;
  char buffer[32];
  while (1) {
    printf("%s", prompt);
    if (!fgets(buffer, sizeof(buffer), stdin)) continue;
    if (sscanf(buffer, "%d", & n) == 1 && n > 0) return n;
    printf("Invalid input. Enter a positive integer:\n");
  }
}
///1
Matrix create_matrix() {
  Matrix m;
  m.rows = get_positive_int("Enter number of rows: ");
  m.cols = get_positive_int("Enter number of columns: ");
  m.data = malloc(m.rows * m.cols * sizeof(float));
  if (!m.data) {
    printf("Memory allocation failed.\n");
    exit(1);
  }

  printf("Enter the elements of the matrix:\n");
  for (int i = 0; i < m.rows; i++) {
    for (int j = 0; j < m.cols; j++) {
      char prompt[64];
      snprintf(prompt, sizeof(prompt), "Element [%d][%d]: ", i + 1, j + 1);
      m.data[i * m.cols + j] = get_float(prompt);
    }
  }

  return m;
}

void add_matrix_to_memory(Matrix m) {
    matrices = realloc(matrices, (matrix_count + 1) * sizeof(Matrix));
    if (!matrices) {
        printf("Memory allocation failed.\n");
        free(m.data);
        exit(1);
    }

    m.id = next_matrix_id++;  // assign unique ID and increment
    matrices[matrix_count++] = m;
    printf("Matrix stored with ID %d.\n", m.id);
}

// Display flat matrix
void display_matrix(float * matrix, int rows, int cols) {
  printf("Matrix (%dx%d):\n", rows, cols);
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      printf("%8.2f ", matrix[i * cols + j]);
    }
    printf("\n");
  }
}
///2
void display_matrix_by_id() {
  int id;
  id = get_positive_int("Enter matrix ID to display: ");
  for (int i = 0; i < matrix_count; i++) {
    if (matrices[i].id == id) {
      display_matrix(matrices[i].data, matrices[i].rows, matrices[i].cols);
      return;
    }
  }
  printf("No matrix found with ID %d.\n", id);
}
/// 9
void display_all_matrices() {
  if (matrix_count == 0) {
    printf("No matrices in memory.\n");
    return;
  }

  for (int i = 0; i < matrix_count; i++) {
    printf("\nMatrix ID: %d\n", matrices[i].id);
    display_matrix(matrices[i].data, matrices[i].rows, matrices[i].cols);
  }
}

void free_matrix(float * matrix) {
  free(matrix);
}
void free_all_matrices() {
  for (int i = 0; i < matrix_count; i++) {
    free(matrices[i].data);
  }
  free(matrices);
  matrices = NULL;
  matrix_count = 0;
}
///3
void delete_matrix_by_id() {
    int id = get_positive_int("Enter matrix ID to delete: ");
    int found = 0;

    for (int i = 0; i < matrix_count; i++) {
        if (matrices[i].id == id) {
            free(matrices[i].data);

            // shift remaining matrices left
            for (int j = i; j < matrix_count - 1; j++)
                matrices[j] = matrices[j + 1];

            matrix_count--;

            if (matrix_count > 0) {
                matrices = realloc(matrices, matrix_count * sizeof(Matrix));
                if (!matrices) {
                    printf("Memory reallocation failed after deletion.\n");
                    exit(1);
                }
            } else {
                free(matrices);
                matrices = NULL;
            }

            printf("Matrix with ID %d deleted.\n", id);
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("No matrix found with ID %d.\n", id);
    }
}

/// 4 
void modify_matrix() {
    if (matrix_count == 0) {
        printf("No matrices in memory.\n");
        return;
    }

    int id = get_positive_int("Enter matrix ID to modify: ");

    Matrix *target = NULL;
    for (int i = 0; i < matrix_count; i++) {
        if (matrices[i].id == id) {
            target = &matrices[i];
            break;
        }
    }

    if (!target) {
        printf("No matrix found with ID %d.\n", id);
        return;
    }

    int modified = 0;
    int choice;

    while (1) {
        printf("\nModify Options:\n");
        printf("0. Cancel\n");
        printf("1. Modify a specific element\n");
        printf("2. Modify a full row\n");
        printf("3. Modify a full column\n");
        printf("Enter your choice: ");

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Try again.\n");
            flush_stdin();
            continue;
        }
        flush_stdin();

        if (choice == 0) {
            printf("Modification canceled.\n");
            return;
        }

        if (choice < 1 || choice > 3) {
            printf("Invalid choice. Please try again.\n");
            continue; // repeat menu
        }

        switch (choice) {
            case 1: {
                int r = get_positive_int("Enter row index (1-based): ");
                int c = get_positive_int("Enter column index (1-based): ");
                if (r < 1 || r > target->rows || c < 1 || c > target->cols) {
                    printf("Invalid indices.\n");
                    continue;
                }
                float val = get_float("Enter new value: ");
                target->data[(r - 1) * target->cols + (c - 1)] = val;
                printf("Element [%d][%d] updated.\n", r, c);
                modified = 1;
                break;
            }
            case 2: {
                int r = get_positive_int("Enter row index (1-based): ");
                if (r < 1 || r > target->rows) {
                    printf("Invalid row index.\n");
                    continue;
                }
                printf("Enter %d new values for row %d:\n", target->cols, r);
                for (int j = 0; j < target->cols; j++) {
                    char prompt[64];
                    snprintf(prompt, sizeof(prompt), "New value for [%d][%d]: ", r, j + 1);
                    target->data[(r - 1) * target->cols + j] = get_float(prompt);
                }
                printf("Row %d updated.\n", r);
                modified = 1;
                break;
            }
            case 3: {
                int c = get_positive_int("Enter column index (1-based): ");
                if (c < 1 || c > target->cols) {
                    printf("Invalid column index.\n");
                    continue;
                }
                printf("Enter %d new values for column %d:\n", target->rows, c);
                for (int i = 0; i < target->rows; i++) {
                    char prompt[64];
                    snprintf(prompt, sizeof(prompt), "New value for [%d][%d]: ", i + 1, c);
                    target->data[i * target->cols + (c - 1)] = get_float(prompt);
                }
                printf("Column %d updated.\n", c);
                modified = 1;
                break;
            }
        }

        break;
    }

    if (modified) {
        printf("\nUpdated Matrix:\n");
        display_matrix(target->data, target->rows, target->cols);
    }
}

// Load a matrix from file (space separated)
Matrix load_matrix_from_file(const char *filename) {
    FILE *fp = fopen(filename,"r");
    if(!fp){ printf("Cannot open file %s\n", filename); exit(1); }

    float values[1024]; int count=0;
    char line[1024];
    int cols=0;
    while(fgets(line,sizeof(line),fp)){
        char *token = strtok(line," \t\n");
        int line_cols=0;
        while(token){
            if(count<1024) values[count++] = atof(token);
            token=strtok(NULL," \t\n");
            line_cols++;
        }
        if(cols==0) cols=line_cols;
        else if(cols!=line_cols){ printf("Inconsistent row length in %s\n", filename); exit(1);}
    }
    fclose(fp);

    int rows = count/cols;
    Matrix m;
    m.rows = rows; m.cols = cols;
    m.data = malloc(rows*cols*sizeof(float));
    for(int i=0;i<rows*cols;i++) m.data[i]=values[i];
    return m;
}

// Load all matrices from a folder
void load_matrices_from_folder(const char *folder) {
    DIR *d = opendir(folder);
if (!d) {
    perror(folder); // prints the system error
    printf("Cannot open folder %s\n", folder);
    return;
}

    struct dirent *entry;
    while((entry=readdir(d))!=NULL){
        if(entry->d_type == DT_REG){
            char filepath[512];
            snprintf(filepath,sizeof(filepath), "%s/%s", folder, entry->d_name);
            Matrix m = load_matrix_from_file(filepath);
            add_matrix_to_memory(m);
        }
    }
    closedir(d);
}

char *trim(char *s) {
    if (!s) return s;
    while (*s && isspace((unsigned char)*s)) s++;             // left trim
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) --end; // right trim
    *end = '\0';
    return s;
}
/// 5
void read_matrix_from_file(void) {
    for (;;) {
        char path[1024];

        printf("Path to matrix file (or 'q' to cancel): ");
        if (!fgets(path, sizeof(path), stdin)) {
            puts("Cancelled.");
            return;
        }
        trim(path);
        if (*path == '\0') {
            puts("Empty path. Try again.");
            continue;
        }
        if ((path[0] == 'q' || path[0] == 'Q') && path[1] == '\0') {
            puts("Cancelled.");
            return;
        }

        FILE *probe = fopen(path, "r");
        if (!probe) {
            perror(path);
            puts("File not found. Try again.");
            continue;     
        }
        fclose(probe);
        Matrix m = load_matrix_from_file(path);
        add_matrix_to_memory(m);
        break;                   // done
    }
}
/// 6
void read_matrices_from_folder(void) {
    for (;;) {  // keep asking until success or user quits
        char folder[512];

        printf("Folder containing matrix files (or q to cancel): ");
        if (!fgets(folder, sizeof(folder), stdin)) { puts("Cancelled."); return; }
        trim(folder);
        if (!*folder || strcasecmp(folder, "q") == 0) { puts("Cancelled."); return; }

        DIR *d = opendir(folder);
        if (!d) {
            perror(folder);
            puts("Cannot open folder; please try again.\n");
            continue; // ask again
        }

        // collect file names
        size_t cap = 16, n = 0;
        char **names = malloc(cap * sizeof(char *));
        if (!names) { puts("Out of memory."); closedir(d); return; }

        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            // skip . and ..
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

            // grow list
            if (n == cap) {
                cap *= 2;
                char **tmp = realloc(names, cap * sizeof(char *));
                if (!tmp) { puts("Out of memory."); break; }
                names = tmp;
            }
            names[n] = strdup(ent->d_name);
            if (!names[n]) { puts("Out of memory."); break; }
            n++;
        }
        closedir(d);

        if (n == 0) {
            puts("Folder is empty; please choose another folder.\n");
            free(names);
            continue; // ask for folder again
        }

        // show options
        printf("\nFiles in '%s':\n", folder);
        for (size_t i = 0; i < n; i++) printf("  %zu) %s\n", i + 1, names[i]);

        // ask selection
        char sel[1024];
        printf("\nEnter files to import (comma-separated indices, or q to cancel): ");
        if (!fgets(sel, sizeof(sel), stdin)) {
            puts("Cancelled.");
            for (size_t i = 0; i < n; i++) free(names[i]);
            free(names);
            return;
        }
        trim(sel);
        if (!*sel || strcasecmp(sel, "q") == 0) {
            puts("Cancelled.");
            for (size_t i = 0; i < n; i++) free(names[i]);
            free(names);
            return;
        }

        size_t imported = 0, errors = 0;
        char *save = NULL;
        for (char *tok = strtok_r(sel, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
            tok = trim(tok);
            if (!*tok) continue;

            int idx = atoi(tok);
            if (idx < 1 || (size_t)idx > n) {
                printf("  ! Skipping invalid index: %s\n", tok);
                errors++;
                continue;
            }

            char pathbuf[1024];
            snprintf(pathbuf, sizeof(pathbuf), "%s/%s", folder, names[idx - 1]);

            Matrix m = load_matrix_from_file(pathbuf);
            add_matrix_to_memory(m);
            imported++;
        }

        for (size_t i = 0; i < n; i++) free(names[i]);
        free(names);

        if (imported == 0) {
            puts("\nNo files imported. Please try again.\n");
            continue; // re-ask from the top
        }

        printf("\nImported %zu file(s)", imported);
        if (errors) printf(" (%zu invalid index/es skipped)", errors);
        printf(".\n");
        break;
    }
}

static int write_matrix_to_fp(FILE *fp, const Matrix *m) {
    // space-separated values, newline per row
    for (int i = 0; i < m->rows; i++) {
        for (int j = 0; j < m->cols; j++) {
            fprintf(fp, "%g", m->data[i * m->cols + j]);
            if (j + 1 < m->cols) fputc(' ', fp);
        }
        fputc('\n', fp);
    }
    return 0;
}
// Create folder if it doesn’t exist. Return 0 on success, -1 on error.
static int ensure_dir_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) return 0;
        fprintf(stderr, "Path exists but is not a directory: %s\n", path);
        return -1;
    }
    if (mkdir(path, 0777) == 0) return 0;
    perror("mkdir");
    return -1;
}
static int directory_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0) && S_ISDIR(st.st_mode);
}


/* ---- 7) Save ONE matrix to a file (ask folder + filename) ---- */
void save_matrix_to_file(void) {
    if (matrix_count == 0) {
        puts("No matrices in memory.");
        return;
    }

    int id = get_positive_int("Enter matrix ID to save: ");

    const Matrix *target = NULL;
    for (int i = 0; i < matrix_count; i++) {
        if (matrices[i].id == id) { target = &matrices[i]; break; }
    }
    if (!target) {
        printf("No matrix found with ID %d.\n", id);
        return;
    }

    char folder[1024], name[256];
    printf("Folder to save into: ");
    if (!fgets(folder, sizeof(folder), stdin)) { puts("Cancelled."); return; }
    trim(folder);
    if (!*folder) { puts("Empty folder."); return; }

    if (!directory_exists(folder)) {
        printf("Folder does not exist: %s\n", folder);
        return;
    }

    printf("File name (e.g., matrix_%d.txt): ", id);
    if (!fgets(name, sizeof(name), stdin)) { puts("Cancelled."); return; }
    trim(name);
    if (!*name) { puts("Empty file name."); return; }

    char path[1400];
    snprintf(path, sizeof(path), "%s/%s", folder, name);

    FILE *fp = fopen(path, "w");
    if (!fp) {
        perror(path);
        puts("Failed to open output file.");
        return;
    }
    write_matrix_to_fp(fp, target);
    fclose(fp);
    printf("Saved matrix %d to: %s\n", id, path);
}

/* ---- 8) Save ALL matrices to a folder ---- */
void save_all_matrices_to_folder(void) {
    if (matrix_count == 0) {
        puts("No matrices in memory.");
        return;
    }

    char folder[1024];
    printf("Enter destination folder (will be created if missing): ");
    if (!fgets(folder, sizeof(folder), stdin)) { puts("Cancelled."); return; }
    trim(folder);
    if (!*folder) { puts("Empty path."); return; }

    if (ensure_dir_exists(folder) != 0) {
        puts("Cannot use/create destination folder.");
        return;
    }

    // Save each matrix as: matrix_<id>_<rows>x<cols>.txt
    size_t saved = 0, failed = 0;
    for (int i = 0; i < matrix_count; i++) {
        char filepath[1400];
        snprintf(filepath, sizeof(filepath), "%s/matrix_%d_%dx%d.txt",
                 folder, matrices[i].id, matrices[i].rows, matrices[i].cols);

        FILE *fp = fopen(filepath, "w");
        if (!fp) {
            perror(filepath);
            failed++;
            continue;
        }
        write_matrix_to_fp(fp, &matrices[i]);
        fclose(fp);
        saved++;
    }

    printf("Saved %zu/%d matrices to folder: %s\n", saved, matrix_count, folder);
    if (failed) printf("  (%zu file(s) failed to write)\n", failed);
}

Matrix* get_amatrix_from_memory() {
    if (matrix_count < 1) {
        printf("Error: No matrices in memory.\n");
        return NULL;
    }

    int id;
    Matrix *m = NULL;

    while (1) {
        id = get_positive_int("Enter Matrix ID: ");
        for (int i = 0; i < matrix_count; i++) {
            if (matrices[i].id == id) {
                m = &matrices[i];
                break;
            }
        }
        if (m != NULL) break;
        printf("No matrix found with ID %d. Try again.\n", id);
    }

    return m;
}

void get_two_matrices_from_memory(Matrix **A, Matrix **B) {

    if (matrix_count < 2) {
        printf("Error: Less than 2 matrices in memory. Please create/load more first.\n");
        *A = NULL;
        *B = NULL;
        return;
    }

    printf("Select Matrix A, ");
    *A = get_amatrix_from_memory();

    printf("Select Matrix B, ");
    *B = get_amatrix_from_memory();
}