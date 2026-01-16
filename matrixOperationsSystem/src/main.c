#include "local_header.h"

#define MAX_MENU_ITEMS 15
int menu_mapping[MAX_MENU_ITEMS];
int menu_len = 0;

static void expand_home(char *path, size_t cap) {
    if (path[0] == '~') {
        const char *home = getenv("HOME");
        if (home) {
            char tmp[1024];
            snprintf(tmp, sizeof(tmp), "%s%s", home, path + 1);
            snprintf(path, cap, "%s", tmp);
        }
    }
}
// ---------------------------------------------------------------------------

// Load ORDER and FOLDERS from config file (supports multiple folders)
void load_config_order(const char *config_path) {
    FILE *f = fopen(config_path, "r");
    if (!f) {
        printf("Could not open config file '%s'. Using default menu order.\n", config_path);
        for (int i = 0; i < MAX_MENU_ITEMS; i++) menu_mapping[i] = i + 1;
        menu_len = MAX_MENU_ITEMS;
        return;
    }

    char line[1024];
    menu_len = 0;

    while (fgets(line, sizeof(line), f)) {
        // strip newline
        line[strcspn(line, "\r\n")] = 0;

        // skip blanks/comments
        char *p = trim(line);
        if (*p == '\0' || *p == '#') continue;

        // ORDER=1,2,3,...
        if (strncmp(p, "ORDER=", 6) == 0) {
            char *q = p + 6;
            char *tok = strtok(q, ",;");
            while (tok && menu_len < MAX_MENU_ITEMS) {
                int num = atoi(trim(tok));
                if (num >= 1 && num <= MAX_MENU_ITEMS) {
                    menu_mapping[menu_len++] = num;
                }
                tok = strtok(NULL, ",;");
            }
            continue;
        }

        // FOLDERS=./a, ./b ; "./path with spaces"
        if (strncmp(p, "FOLDERS=", 8) == 0) {
            char *q = p + 8;

            // We’ll split on comma OR semicolon
            char *save = NULL;
            for (char *tok = strtok_r(q, ",;", &save); tok; tok = strtok_r(NULL, ",;", &save)) {
                tok = trim(tok);

                // remove optional surrounding quotes
                if (tok[0] == '"' && tok[strlen(tok) - 1] == '"' && strlen(tok) >= 2) {
                    tok[strlen(tok) - 1] = '\0';
                    tok++;
                }

                if (*tok == '\0') continue;

                // expand ~ to $HOME
                char path[1024];
                snprintf(path, sizeof(path), "%s", tok);
                expand_home(path, sizeof(path));

                load_matrices_from_folder(path);
            }
            continue;
        }
    }

    fclose(f);

    if (menu_len == 0) {
        for (int i = 0; i < MAX_MENU_ITEMS; i++) menu_mapping[i] = i + 1;
        menu_len = MAX_MENU_ITEMS;
    }
}

// Runs either Add or Subtract or multi  ( 0 = subtract,1 = add, 2=multi)
static void run_opp(int op_type) {
    // op_type: 1 = add, 0 = sub, 2 = multiply, 3=Determinant

   Matrix *A,*B;
    A = get_amatrix_from_memory();
    if (!A ) return; 
    if (op_type != 3){
    B = get_amatrix_from_memory(); 
    if (!B) return;        // ask user for two IDs
    }

    // Validate matrix dimensions based on operation
    if (op_type == 1 || op_type == 0) {
        if (!validate_same_dimensions(*A, *B)) {
            return;
        }
    } else if (op_type == 2) {
        if (!validate_multiplication_dimensions(*A, *B)) {
            return;
        } 
    }
    else if(op_type == 3){
          if (A->rows != A->cols) {
              fprintf(stderr, "Matrix must be square to compute determinant.\n");
              return;
          }
    }

    // Ask only for execution mode
    printf("\nChoose execution mode:\n");
    printf("1. Single-thread\n");
    printf("2. Multi-process\n");
    printf("3. OpenMP\n");
    int mode = get_positive_int("Enter choice: ");

    double start = get_time_ms();
    Matrix result;

    // Select operation + mode
    if (op_type == 1) { // ADD
        if (mode == 1){     result = add_matrices_single_thread(*A, *B,0);
        }else if (mode == 2){ result = add_matrices_multi_process(*A, *B);
        }else if (mode ==3 ){ result = add_matrices_single_thread(*A, *B, 1);
        }else { puts("Invalid choice."); return; }

    } else if (op_type == 0) { // SUB
        if (mode == 1)      result = sub_matrices_single_thread(*A, *B,0);
        else if (mode == 2) result = sub_matrices_multi_process(*A, *B);
        else if (mode ==3 ) result = sub_matrices_single_thread(*A, *B, 1);
        else { puts("Invalid choice."); return; }

    } else if (op_type == 2) { // MULTIPLY
        if (mode == 1)      result = multiply_matrices_single_thread(*A, *B);
        else if (mode == 2) result = multiply_matrices_multi_process(*A, *B, 0);
        else if (mode ==3 ) result = multiply_matrices_multi_process(*A, *B, 1);
        else { puts("Invalid choice."); return; }
    } else if(op_type == 3)   {
        float det;
        if (mode == 1){
            det = compute_det_sequential(A->data, A->rows,0);
        printf("\nResult: Determinant of matrix ID %d = %.3f\n", A->id, det);
        }
        else if (mode == 2){
            det = determinant_multi(*A,0);
            printf("\nResult: Determinant of matrix ID %d = %.3f\n", A->id, det);    
        }else if (mode ==3 ){
             det = determinant_multi(*A,1);  
             printf("\nResult: Determinant of matrix ID %d = %.3f\n", A->id, det);
        }
    } else {
        puts("Invalid operation type.");
        return;
    }
    if(op_type == 0 || op_type == 1 || op_type == 2){
    printf("\n--- Result ---\n");
    display_matrix(result.data, result.rows, result.cols);
    }
    double end = get_time_ms();
    printf("\nExecution Time: %.3f ms\n", end - start);
    if (op_type != 3){ 
    add_matrix_to_memory(result);
    printf("Stored in memory successfully\n");
    }
}



//---Determint---//not used
//void find_determinant() {
//    Matrix *A = get_amatrix_from_memory();
//    if (!A) return;
//
//    if (A->rows != A->cols) {
//        fprintf(stderr, "Matrix must be square to compute determinant.\n");
//        return;
//    }
//
//    printf("\nChoose execution mode:\n");
//    printf("1. Single-thread\n");
//    printf("2. Multi-process\n");
//    int mode = get_positive_int("Enter choice: ");
//    double start = get_time_ms();
//    float det;
//    if (mode == 1)
//        det = compute_det_sequential(A->data, A->rows, openmpFlag);
//    else if (mode == 2)
//        det = determinant_multi(*A,0);
//    else {
//        puts("Invalid choice.");
//        return;
//    }
//    
//
//    printf("\nResult: Determinant of matrix ID %d = %.3f\n", A->id, det);
//
//    double end = get_time_ms();
//    printf("\nExecution Time: %.3f ms\n", end - start);
//}

// Show menu dynamically according to menu_mapping
void showMenuDynamic() {
printf("\n=== MATRIX OPERATIONS MENU ===\n");
for (int i = 0; i < menu_len; i++) {
printf("%d. ", i+1);
switch(menu_mapping[i]) {
case 1: printf("Enter a matrix\n"); break;
case 2: printf("Display a matrix\n"); break;
case 3: printf("Delete a matrix\n"); break;
case 4: printf("Modify a matrix\n"); break;
case 5: printf("Read a matrix from a file\n"); break;
case 6: printf("Read matrices from a folder\n"); break;
case 7: printf("Save a matrix to a file\n"); break;
case 8: printf("Save all matrices to a folder\n"); break;
case 9: printf("Display all matrices\n"); break;
case 10: printf("Add two matrices\n"); break;
case 11: printf("Subtract two matrices\n"); break;
case 12: printf("Multiply two matrices\n"); break;
case 13: printf("Find determinant of a matrix\n"); break;
case 14: printf("Find eigenvalues and eigenvectors\n"); break;
case 15: printf("Exit\n"); break;
}
}
printf("===============================\nEnter your choice: ");
}

int main(int argc, char *argv[]) {
int running = 1;
int choice;

// Load config
const char *config_file = (argc >= 2) ? argv[1] : "default_config.txt";
load_config_order(config_file);

printf("Welcome to the Matrix Operations Tool!\n");
init_pool(100);
while (running) {
    showMenuDynamic();
    if (scanf("%d", &choice) != 1 || choice < 1 || choice > menu_len) {
        printf("Invalid input. Try again.\n");
        flush_stdin();
        continue;
    }
    flush_stdin();
    int action = menu_mapping[choice-1]; // map 1..N -> real action
    switch(action) {
        case 1: { Matrix m=create_matrix(); add_matrix_to_memory(m); break; }
        case 2: display_matrix_by_id(); break;
        case 3: delete_matrix_by_id(); break;
        case 4: modify_matrix(); break;
        case 5: read_matrix_from_file(); break;
        case 6: read_matrices_from_folder(); break;
        case 7: save_matrix_to_file(); break;
        case 8: save_all_matrices_to_folder(); break;
        case 9: display_all_matrices(); break;
        case 10: { run_opp(1);  break;}
        case 11: { run_opp(0);  break;}
        case 12: { run_opp(2);  break;}
        case 13: { run_opp(3);  break;}
 //       case 13: find_determinant(); break;
        case 14: compute_eigen(); break;
        case 15: running=0; printf("Exiting...\n"); break;
    }
}


free_all_matrices();
return 0;

}