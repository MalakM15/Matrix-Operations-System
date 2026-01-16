#ifndef MATRIX_MANAGER_H
#define MATRIX_MANAGER_H

typedef struct {
    int id;
    int rows;
    int cols;
    float *data;
} Matrix;

// Matrix creation and memory management
Matrix create_matrix();
void add_matrix_to_memory(Matrix m);
void free_all_matrices();
void flush_stdin();
// helpers you want to share
char *trim(char *s);
// Display / modify matrices
void display_matrix_by_id();
void display_all_matrices();
void delete_matrix_by_id();
void modify_matrix();
Matrix* get_amatrix_from_memory(void);
void get_two_matrices_from_memory(Matrix **A, Matrix **B);
void display_matrix(float * matrix, int rows, int cols) ;
int get_positive_int(const char * prompt);
// Load matrices from file or folder
Matrix load_matrix_from_file(const char *filename);
void load_matrices_from_folder(const char *folder);

void read_matrix_from_file(void);
void read_matrices_from_folder(void);
void save_matrix_to_file(void);
void save_all_matrices_to_folder(void);

#endif