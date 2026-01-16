#include "local_header.h"



// Time measurement helper
double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1.0e6;
}

/** Validate that two matrices have the same dimensions. */
int validate_same_dimensions(Matrix A, Matrix B) {
    if (A.rows != B.rows || A.cols != B.cols) {
        fprintf(stderr,
                "Error: Matrix size mismatch (%dx%d vs %dx%d)\n",
                A.rows, A.cols, B.rows, B.cols);
        return 0;
    }
    return 1;
}

/** Validate matrix dimensions for multiplication. */
int validate_multiplication_dimensions(Matrix A, Matrix B) {
    if (A.cols != B.rows) {
        fprintf(stderr,
                "Error: Matrix dimensions not compatible for multiplication (%dx%d vs %dx%d)\n",
                A.rows, A.cols, B.rows, B.cols);
        return 0;
    }
    return 1;
}

static Matrix execute_elementwise_multi(Matrix A, Matrix B, OperationType op) {
    

    int total_elements = A.rows * A.cols;
    int max_allowed = pool_get_max_children();
    int used_workers = (total_elements < max_allowed) ? total_elements : max_allowed;

    // Ensure enough workers
    while (pool_get_current_children() < used_workers) {
        if (add_child_to_pool(0) == -1) break;
    }

    int base = total_elements / used_workers;
    int extra = total_elements % used_workers;

    // Allocate result matrix
    Matrix R = { .rows = A.rows, .cols = A.cols, .id = -1 };
    R.data = malloc(total_elements * sizeof(float));
    if (!R.data) { perror("Result matrix allocation failed"); exit(1); }

    int index = 0;
    int worker_used[used_workers];

    /* Assign tasks to workers */
    for (int w = 0; w < used_workers; w++) {
        int batch_size = base + (w < extra ? 1 : 0);
        Task *tasks = malloc(batch_size * sizeof(Task));
        if (!tasks) { perror("Task allocation failed"); exit(1); }

        for (int t = 0; t < batch_size; t++) {
            tasks[t].index = index;
            tasks[t].a = A.data[index];
            tasks[t].b = B.data[index];
            tasks[t].op = op;
            index++;
        }

        int worker;
        while ((worker = get_free_child()) == -1); // busy wait
        pool_assign_batch(worker, tasks, batch_size);
        worker_used[w] = worker;
        free(tasks);
    }

    /* Collect results */
    for (int w = 0; w < used_workers; w++) {
        int batch_size = base + (w < extra ? 1 : 0);
        TaskResult *results = malloc(batch_size * sizeof(TaskResult));
        if (!results) { perror("Result allocation failed"); exit(1); }

        int worker = worker_used[w];
        pool_collect_batch(worker, results, batch_size);

        for (int t = 0; t < batch_size; t++)
            R.data[results[t].index] = results[t].result;

        free(results);
    }

    return R;
}

// ------------  Single-Thread Operations -----------------//

Matrix add_matrices_single_thread(Matrix A, Matrix B,int ompFlag) {
    Matrix R = { .rows = A.rows, .cols = A.cols, .id = -1 };
    R.data = malloc(A.rows * A.cols * sizeof(float));
    if (!R.data) { perror("malloc"); exit(1); }
    #pragma omp parallel for if(ompFlag)
    for (int i = 0; i < A.rows * A.cols; i++)
        R.data[i] = A.data[i] + B.data[i];

    return R;
}

Matrix sub_matrices_single_thread(Matrix A, Matrix B,int ompFlag) {
    Matrix R = { .rows = A.rows, .cols = A.cols, .id = -1 };
    R.data = malloc(A.rows * A.cols * sizeof(float));
    if (!R.data) { perror("malloc"); exit(1); }
    #pragma omp parallel for if(ompFlag)
    for (int i = 0; i < A.rows * A.cols; i++)
        R.data[i] = A.data[i] - B.data[i];

    return R;
}
// ------------  Multi-Thread Operations -----------------//
Matrix add_matrices_multi_process(Matrix A, Matrix B) {
    return execute_elementwise_multi(A, B, OP_ADD);
}

Matrix sub_matrices_multi_process(Matrix A, Matrix B) {
    return execute_elementwise_multi(A, B, OP_SUB);
}

// ------------  Matrix Multiplication -----------------//
static Matrix execute_multiply_multi(Matrix A, Matrix B,int openmpFlag) {
    if (A.cols != B.rows) {
        fprintf(stderr,
                "Error: Matrix dimensions not compatible (%dx%d vs %dx%d)\n",
                A.rows, A.cols, B.rows, B.cols);
        Matrix empty = {0};
        return empty;
    }

    int total_elements = A.rows * B.cols;
    int max_allowed = pool_get_max_children();
    int used_workers = (total_elements < max_allowed) ? total_elements : max_allowed;

    while (pool_get_current_children() < used_workers)
        if (add_child_to_pool(openmpFlag) == -1) break;

    Matrix R = { .rows = A.rows, .cols = B.cols, .id = -1 };
    R.data = malloc(total_elements * sizeof(float));
    if (!R.data) { perror("malloc"); exit(1); }

    int base = total_elements / used_workers;
    int extra = total_elements % used_workers;
    int index = 0;
    int worker_used[used_workers];

    /* Assign batches */
    #pragma omp parallel for
    for (int w = 0; w < used_workers; w++) {
        int batch_size = base + (w < extra ? 1 : 0);
        Task *tasks = malloc(batch_size * sizeof(Task));
        if (!tasks) { perror("malloc"); exit(1); }

        for (int t = 0; t < batch_size; t++) {
            int row = index / B.cols;
            int col = index % B.cols;

            tasks[t].index = index;
            tasks[t].row = row;
            tasks[t].col = col;
            tasks[t].A_rows = A.rows;
            tasks[t].A_cols = A.cols;
            tasks[t].B_cols = B.cols;
            tasks[t].op = OP_MUL;
            tasks[t].A = A.data;
            tasks[t].B = B.data;
            index++;
        }

        int worker;
        #pragma omp critical
        {
        while ((worker = get_free_child()) == -1);
        }
        pool_assign_batch(worker, tasks, batch_size);
        worker_used[w] = worker;
        free(tasks);
    }

    /* Collect results */
    #pragma omp parallel for
    for (int w = 0; w < used_workers; w++) {
        int batch_size = base + (w < extra ? 1 : 0);
        TaskResult *results = malloc(batch_size * sizeof(TaskResult));
        if (!results) { perror("malloc"); exit(1); }

        int worker = worker_used[w];
        pool_collect_batch(worker, results, batch_size);

        for (int t = 0; t < batch_size; t++)
            R.data[results[t].index] = results[t].result;

        free(results);
    }

    return R;
}

Matrix multiply_matrices_single_thread(Matrix A, Matrix B) {
    if (A.cols != B.rows) {
        fprintf(stderr, "Error: Matrix dimensions not compatible.\n");
        Matrix empty = {0};
        return empty;
    }

    Matrix R = { .rows = A.rows, .cols = B.cols, .id = -1 };
    R.data = malloc(R.rows * R.cols * sizeof(float));
    if (!R.data) { perror("malloc"); exit(1); }

    for (int i = 0; i < A.rows; i++)
        for (int j = 0; j < B.cols; j++) {
            float sum = 0;
            for (int k = 0; k < A.cols; k++)
                sum += A.data[i * A.cols + k] * B.data[k * B.cols + j];
            R.data[i * R.cols + j] = sum;
        }

    return R;
}

Matrix multiply_matrices_multi_process(Matrix A, Matrix B, int ompFlag) {
    return execute_multiply_multi(A, B, ompFlag);
}
 ///////////--------------Determinant-------------////////////////
/** Parallelized single-level determinant using child pool */
float matrix_determinant(Matrix *A) {
    if (A->rows != A->cols) {
        fprintf(stderr, "Error: determinant defined only for square matrices\n");
        exit(1);
    }

    int n = A->rows;

    // Base cases
    if (n == 1) return A->data[0];
    if (n == 2) return A->data[0] * A->data[3] - A->data[1] * A->data[2];

    float det = 0.0f;
    int sign = 1;

    TaskResult results[n];     // store results of minors
    float *minors[n];          // keep track of minor matrices
    float coeffs[n];           // coefficients for each minor

    for (int c = 0; c < n; c++) {
        // Allocate minor
        int sub_n = n - 1;
        float *minor = malloc(sub_n * sub_n * sizeof(float));
        if (!minor) { perror("malloc"); exit(1); }
        minors[c] = minor;

        int mi = 0;
        for (int i = 0; i < n; i++) { // Iterate through all rows of A
            if (i == 0) continue; // *** Skip the first row (A[0])
            for (int j = 0; j < n; j++) { // Iterate through all columns of A
                if (j == c) continue; // *** Skip the current column c
                    minor[mi++] = A->data[i * n + j];
            }
        }
        // Prepare task
        Task task = {0};
        task.op = OP_DET;
        task.A_rows = sub_n;
        task.A_cols = sub_n;
        task.A = minor;

        // Store coefficient
        coeffs[c] = sign * A->data[c];
        sign = -sign;

        // Assign to available child and immediately collect
        int child_id = get_free_child();
        if (child_id == -1) {
            fprintf(stderr, "No free child available!\n");
            exit(1);
        }

        pool_assign_batch(child_id, &task, 1);
        pool_collect_batch(child_id, &results[c], 1);
    }

    // Compute final determinant
    for (int c = 0; c < n; c++) {
        det += coeffs[c] * results[c].result;
        free(minors[c]);
    }

    return det;
}

float compute_det_sequential(float *data, int n, int openmpFlag) {
    if (n == 1) return data[0];
    if (n == 2) return data[0] * data[3] - data[1] * data[2];

    float det = 0.0f;
    float partial_dets[n];
    
    #pragma omp parallel for num_threads(8) if(openmpFlag)
    for (int c = 0; c < n; c++) {
        int sub_n = n - 1;
        float *minor = malloc(sub_n * sub_n * sizeof(float));
        if (!minor) { perror("malloc"); exit(1); }

        int mi = 0;
        // skip the current column c for minor
        for (int i = 1; i < n; i++) {      // start from row 1
            for (int j = 0; j < n; j++) {
                if (j == c) continue;      // skip column c
                minor[mi++] = data[i * n + j];
            }
        }

        float subdet = compute_det_sequential(minor, sub_n, openmpFlag);
        float term = data[c] * subdet;
        if (c % 2 != 0) { //for sign
             term = -term;
        }
        partial_dets[c] = term;
        free(minor);
    }
    
    // Sum the partial results (needs to be sequential after the parallel loop)
    for (int c = 0; c < n; c++) {
        det += partial_dets[c];
    }
    return det;
}


float determinant_multi(Matrix A,int openmpFlag) {
    int n = A.rows;
        if (n != A.cols) {
        fprintf(stderr, "Error: determinant only for square matrices\n");
        exit(1);
    }
    if (n == 1) return A.data[0];
    if (n == 2) return A.data[0] * A.data[3] - A.data[1] * A.data[2];

    int max_allowed = pool_get_max_children();
    int used_workers = (n < max_allowed) ? n : max_allowed;
    // Ensure enough workers
    while (pool_get_current_children() < used_workers) {

        if (add_child_to_pool(openmpFlag) == -1) break;
    }

    // Prepare minor tasks
    Task *tasks = malloc(used_workers * sizeof(Task));
    if (!tasks) { perror("Task alloc failed"); exit(1); }
for (int i = 0; i < used_workers; i++) {
        tasks[i].index = i;
        tasks[i].op = OP_DET;
        tasks[i].A = A.data;       // pass original matrix
        tasks[i].A_rows = n;
        tasks[i].A_cols = n;
        tasks[i].minor_skip_col = i;  // skip column i
        tasks[i].a = A.data[i];    
    }

    // Assign tasks to workers
    int worker_used[used_workers];
    for (int i = 0; i < used_workers; i++) {
        int worker;
        while ((worker = get_free_child()) == -1); // wait for available worker
        pool_assign_batch(worker, &tasks[i], 1);
        worker_used[i] = worker;
    }

    // Collect results
    TaskResult *results = malloc(used_workers * sizeof(TaskResult));
    if (!results) { perror("result malloc"); exit(1); }

    float det = 0.0f;
    int sign = 1;

    for (int i = 0; i < used_workers; i++) {
        pool_collect_batch(worker_used[i], &results[i], 1);
        det += sign * tasks[i].a * results[i].result;
        sign = -sign;
    }

    free(tasks);
    free(results);

    return det;
}

//--------      Eigenvalues & Eigenvectors Helpers------------/
static int is_square(Matrix A) { 
    return (A.rows == A.cols);
}

static Matrix subtract_lambda_I(Matrix A, float lambda) {
    Matrix R;
    R.rows = A.rows;
    R.cols = A.cols;
    R.id = -1;
    R.data = (float *) malloc(R.rows * R.cols * sizeof(float));
    if (!R.data) {
        perror("subtract_lambda_I: malloc failed");
        exit(1);
    }

    for (int i = 0; i < R.rows; i++) {
        for (int j = 0; j < R.cols; j++) {
            float value = A.data[i * A.cols + j];
            if (i == j) value -= lambda;
            R.data[i * R.cols + j] = value;
        }
    }

    return R;
}
static float evaluate_characteristic_multi(Matrix A, float lambda,int ompFlag) {
    Matrix M = subtract_lambda_I(A, lambda);

   // float det_value = determinant_multi(M,1); 
    float det_value = compute_det_sequential(M.data, M.rows, ompFlag);
    free(M.data);  
    return det_value;
}
static float fsign(float x) {
    if (x > 0.0f) return 1.0f;
    if (x < 0.0f) return -1.0f;
    return 0.0f;
}

static int fapprox_zero(float x) {
    const float eps = 1e-4f;   // With float-only precision
    return (x > -eps && x < eps);
}

//-----    Eigenvalues (Multi-Process)-------//
/* --- CONFIGURABLE EIGEN SEARCH PARAMETERS -- */
static const float EIGEN_LAMBDA_START = -100.0f;
static const float EIGEN_LAMBDA_END   =  100.0f;
static const float EIGEN_LAMBDA_STEP  =   0.5f;
static const int   EIGEN_BISECT_ITER  = 20;
static const int   EIGEN_MAX_ROOTS    = 100;

static float refine_root_multi(Matrix A, float left, float right,int openmpFlag) {
    float mid;
    for (int iter = 0; iter < EIGEN_BISECT_ITER; iter++) {

        mid = (left + right) * 0.5f;

        float f_left  = evaluate_characteristic_multi(A, left,openmpFlag);
        float f_mid   = evaluate_characteristic_multi(A, mid,openmpFlag);

        if (fapprox_zero(f_mid))
            return mid;

        /* keep interval that contains sign change */
        if (fsign(f_left) != fsign(f_mid))
            right = mid;
        else
            left = mid;
    }
    return mid;
}

static Matrix compute_eigenvalues_multi(Matrix A,int ompFlag ) {

    if (!is_square(A)) {
        printf("Error: Matrix must be square to compute eigenvalues.\n");
        Matrix empty = {0,0,0,NULL};
        return empty;
    }
    /* storage for detected roots */
    float roots[EIGEN_MAX_ROOTS];
    int root_count = 0;

    float lambda_prev = EIGEN_LAMBDA_START;
    float f_prev = evaluate_characteristic_multi(A, lambda_prev,ompFlag);

    /* === SCAN RANGE & DETECT SIGN CHANGES === */
    for (float lambda = lambda_prev + EIGEN_LAMBDA_STEP;
         lambda <= EIGEN_LAMBDA_END;
         lambda += EIGEN_LAMBDA_STEP)
    {
        float f_curr = evaluate_characteristic_multi(A, lambda,ompFlag);

        if (fapprox_zero(f_curr)) {
            /* direct hit */
            if (root_count < EIGEN_MAX_ROOTS) {
                roots[root_count++] = lambda;
            }
        }
        else if (fsign(f_prev) != fsign(f_curr)) {
            /* sign change => refine interval */
            if (root_count < EIGEN_MAX_ROOTS) {
                float r = refine_root_multi(A, lambda_prev, lambda, ompFlag);
                roots[root_count++] = r;
            }
        }

        /* prepare for next iteration */
        lambda_prev = lambda;
        f_prev = f_curr;
    }

    if (root_count == 0) {
        printf("Warning: No real eigenvalues found in scanned range.\n");
        Matrix empty = {0,0,0,NULL};
        return empty;
    }

    /*---- SORT ASCENDING----- */
    for (int i = 0; i < root_count - 1; i++) {
        for (int j = i + 1; j < root_count; j++) {
            if (roots[j] < roots[i]) {
                float tmp = roots[i];
                roots[i] = roots[j];
                roots[j] = tmp;
            }
        }
    }

    /* --BUILD RESULT MATRIX (N×1) --*/
    Matrix R;
    R.rows = root_count;
    R.cols = 1;
    R.id = -1; 
    R.data = (float *) malloc(root_count * sizeof(float));
    if (!R.data) {
        perror("compute_eigenvalues_multi: malloc failed");
        exit(1);
    }

    for (int i = 0; i < root_count; i++)
        R.data[i] = roots[i];

    return R;
}

Matrix compute_eigenvalues(Matrix A,int openmpFlag) {
    return compute_eigenvalues_multi(A,openmpFlag);
}

//     Eigenvectors (EV-LS + L2 + Pivoting)
//Solve (A - λI)v = 0 using Gaussian Elimination ---/

static void normalize_vector_l2(float *v, int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; i++)
        sum += v[i] * v[i];

    if (sum <= 0.0f) return;  // zero vector

    float norm = 1.0f / sqrtf(sum);
    for (int i = 0; i < n; i++)
        v[i] *= norm;

    /* make first non-zero positive */
    for (int i = 0; i < n; i++) {
        if (fabsf(v[i]) > 1e-6f) {     // first non-zero element
            if (v[i] < 0.0f) {
                for (int j = 0; j < n; j++)
                    v[j] = -v[j];
            }
            break;
        }
    }
}

static void gaussian_eliminate_with_pivot(float *M, int n) {

    for (int col = 0; col < n; col++) {

        /* --- PARTIAL PIVOTING --- */
        int pivot = col;
        float max_val = fabsf(M[col * n + col]);

        for (int r = col + 1; r < n; r++) {
            float val = fabsf(M[r * n + col]);
            if (val > max_val) {
                max_val = val;
                pivot = r;
            }
        }

        /* swap rows if needed */
        if (pivot != col) {
            for (int c = 0; c < n; c++) {
                float tmp = M[col * n + c];
                M[col * n + c] = M[pivot * n + c];
                M[pivot * n + c] = tmp;
            }
        }

        /* eliminate below pivot */
        float diag = M[col * n + col];
        if (fabsf(diag) < 1e-6f) continue;   // skip zero pivot
        
        for (int r = col + 1; r < n; r++) {
            float factor = M[r * n + col] / diag;
            for (int c = col; c < n; c++)
                M[r * n + c] -= factor * M[col * n + c];
        }
    }
}

static float* solve_nullspace_ls(Matrix M) {

    int n = M.rows;
    float *A = (float*) malloc(n * n * sizeof(float));
    if (!A) {
        perror("solve_nullspace_ls malloc failed");
        exit(1);
    }

    /* Copy matrix data */
    for (int i = 0; i < n * n; i++)
        A[i] = M.data[i];

    /* Gaussian elimination with pivoting */
    gaussian_eliminate_with_pivot(A, n);

    /* Allocate eigenvector */
    float *v = (float*) calloc(n, sizeof(float));
    if (!v) {
        perror("solve_nullspace_ls calloc failed");
        exit(1);
    }

    /* Back Substitution (treat last variable = 1) */
    v[n - 1] = 1.0f;

    for (int i = n - 2; i >= 0; i--) {
        float sum = 0.0f;
        for (int j = i + 1; j < n; j++)
            sum += A[i * n + j] * v[j];

        float diag = A[i * n + i];
        if (fabsf(diag) < 1e-6f) {
            v[i] = 0.0f;  // free variable
        } else {
            v[i] = -sum / diag;
        }
    }

    /* Normalize */
    normalize_vector_l2(v, n);

    free(A);
    return v;
}

Matrix compute_eigenvectors(Matrix A, Matrix eigenvalues) {

    if (!is_square(A)) {
        printf("Error: Matrix must be square.\n");
        Matrix empty = {0,0,0,NULL};
        return empty;
    }

    int n = A.rows;
    int k = eigenvalues.rows;

    Matrix EV;
    EV.rows = n;
    EV.cols = k;
    EV.id = -1;
    EV.data = (float*) malloc(n * k * sizeof(float));
    if (!EV.data) {
        perror("compute_eigenvectors malloc failed");
        exit(1);
    }

    for (int idx = 0; idx < k; idx++) {

        float lambda = eigenvalues.data[idx];

        /* Build (A - λI) */
        Matrix M = subtract_lambda_I(A, lambda);

        /* Solve nullspace-> eigenvector */
        float *v = solve_nullspace_ls(M);

        /* Insert into EV matrix as column */
        for (int r = 0; r < n; r++)
            EV.data[r * k + idx] = v[r];

        free(v);
        free(M.data);
    }

    return EV;
}


/* ====== Eigenvectors + compute_eigen() ====== */

static float* back_substitution_upper(float *U, int n) {
    /* U is upper-triangular (or nearly). We solve Ux = 0 with the
       last variable set to 1 (least-squares-ish nullspace pick). */
    float *x = (float*) calloc(n, sizeof(float));
    if (!x) { perror("back_substitution_upper calloc"); exit(1); }

    const float eps = 1e-6f;
    x[n - 1] = 1.0f;

    for (int i = n - 2; i >= 0; i--) {
        float sum = 0.0f;
        for (int j = i + 1; j < n; j++)
            sum += U[i * n + j] * x[j];

        float diag = U[i * n + i];
        if (fabsf(diag) < eps) {
            /* free variable -> keep x[i] = 0 */
            x[i] = 0.0f;
        } else {
            x[i] = -sum / diag;
        }
    }
    return x;
}

static float* solve_nullspace_ls_full(Matrix M) {
    int n = M.rows;
    float *A = (float*) malloc(n * n * sizeof(float));
    if (!A) { perror("solve_nullspace_ls_full malloc"); exit(1); }
    for (int i = 0; i < n * n; i++) A[i] = M.data[i];

    extern void gaussian_eliminate_with_pivot(float *M_, int n); /* ensure visible */
    gaussian_eliminate_with_pivot(A, n);

    float *v = back_substitution_upper(A, n);
    free(A);
    return v;
}

static Matrix compute_eigenvectors_from_lambdas(Matrix A, Matrix lambdas) {
    // Build N×k (columns are eigenvectors)
    int n = A.rows;
    int k = lambdas.rows; /* lambdas is k×1 */

    Matrix EV;
    EV.rows = n;
    EV.cols = k;
    EV.id   = -1;
    EV.data = (float*) calloc(n * k, sizeof(float));
    if (!EV.data) { perror("compute_eigenvectors_from_lambdas calloc"); exit(1); }

    for (int c = 0; c < k; c++) {
        float lambda = lambdas.data[c];
        Matrix M = { .rows = n, .cols = n, .id = -1, .data = NULL };
        M = (Matrix){ .rows = n, .cols = n, .id = -1, .data = NULL };

        /* A - λI */
        extern Matrix subtract_lambda_I(Matrix A_, float lambda_);
        M = subtract_lambda_I(A, lambda);

        /* Solve (A - λI)v = 0 */
        float *v = solve_nullspace_ls_full(M);

        /* Normalize (L2) and fix sign */
        extern void normalize_vector_l2(float *v_, int n_);
        normalize_vector_l2(v, n);

        /* Put as column c */
        for (int r = 0; r < n; r++)
            EV.data[r * k + c] = v[r];

        free(v);
        free(M.data);
    }

    return EV;
}

/***********************  EIGEN: SINGLE + OPENMP  **************************/
/* --------  evaluate characteristic using single (sequential) ------- */
static float eval_char_single(Matrix A, float lambda) {
    Matrix M = subtract_lambda_I(A, lambda);
    float detv = compute_det_sequential(M.data, M.rows, 0); /* 0 => no OpenMP */
    free(M.data);
    return detv;
}

/* -------------------- eigenvalues: SINGLE-THREAD -------------------- */
Matrix compute_eigenvalues_single(Matrix A) {
    Matrix Z = (Matrix){0};
    if (!is_square(A)) return Z;

    float roots[EIGEN_MAX_ROOTS];
    int k = 0;

    float lam_prev = EIGEN_LAMBDA_START;
    float f_prev   = eval_char_single(A, lam_prev);

    for (float lam = lam_prev + EIGEN_LAMBDA_STEP; lam <= EIGEN_LAMBDA_END; lam += EIGEN_LAMBDA_STEP) {
        float f_curr = eval_char_single(A, lam);

        if (fapprox_zero(f_curr)) {
            if (k < EIGEN_MAX_ROOTS) roots[k++] = lam;
        } else if (fsign(f_prev) != fsign(f_curr)) {
            float L = lam_prev, R = lam, mid = 0.5f*(L+R);

            for (int it=0; it<EIGEN_BISECT_ITER; ++it) {
                mid = 0.5f*(L+R);
                float fL = eval_char_single(A, L);
                float fM = eval_char_single(A, mid);
                if (fapprox_zero(fM)) break;
                if (fsign(fL) != fsign(fM)) R = mid; else L = mid;
            }
            if (k < EIGEN_MAX_ROOTS) roots[k++] = 0.5f*(L+R);
        }

        lam_prev = lam;
        f_prev   = f_curr;
    }

    if (k == 0) return Z;

    /* sort ascending */
    for (int i=0;i<k-1;i++)
        for (int j=i+1;j<k;j++)
            if (roots[j] < roots[i]) { float t=roots[i]; roots[i]=roots[j]; roots[j]=t; }

    Matrix R = (Matrix){ .rows = k, .cols = 1, .id = -1 };
    R.data = (float*)malloc(k * sizeof(float));
    if (!R.data) { perror("eigs_single malloc"); exit(1); }
    for (int i=0;i<k;i++) R.data[i] = roots[i];
    return R;
}
void compute_eigen() {
    Matrix *Aptr = get_amatrix_from_memory();
    if (!Aptr) return;
    Matrix A = *Aptr;

    if (A.rows != A.cols) {
        fprintf(stderr, "Matrix must be square to compute eigenvalues/eigenvectors.\n");
        return;
    }

    printf("\nChoose execution mode for eigenvalues/eigenvectors:\n");
    printf("1. Single-thread\n");
    printf("2. Multi-process\n");
    printf("3. OpenMP\n");
    int mode = get_positive_int("Enter choice: ");

    double t0 = get_time_ms();
    Matrix lambdas = {0};
    if (mode == 1) {
        lambdas = compute_eigenvalues_single(A,0);
    } else if (mode == 2) {
        lambdas = compute_eigenvalues(A,0);       
    } else if (mode == 3) {
     //   lambdas =compute_eigenvalues_single(A,1);
         lambdas = compute_eigenvalues(A,1); 

    } else {
        puts("Invalid choice."); return;
    }
    double t1 = get_time_ms();

    if (!lambdas.data || lambdas.rows == 0) { puts("No real eigenvalues found."); return; }
    printf("Eigenvalue computation time: %.3f ms\n", t1 - t0);

    /* eigenvectors (N×k), per your choice Q1=B  */
    
    double start_vec = get_time_ms();
    Matrix eigvecs = compute_eigenvectors_from_lambdas(A, lambdas);

    double end_vec = get_time_ms();
    printf("Eigenvector computation time: %.3f ms\n", end_vec - start_vec);

    /* Print with 3 decimals (Q2=A) */
    printf("\n=== Eigenvalues (k=%d) ===\n", lambdas.rows);
    for (int i = 0; i < lambdas.rows; i++)
        printf("λ[%d] = %.3f\n", i + 1, lambdas.data[i]);

    printf("\n=== Eigenvectors (columns) %dx%d ===\n", eigvecs.rows, eigvecs.cols);
    for (int r = 0; r < eigvecs.rows; r++) {
        for (int c = 0; c < eigvecs.cols; c++) {
            printf("%8.3f ", eigvecs.data[r * eigvecs.cols + c]);
        }
        printf("\n");
    }
    /* Store both in memory for later use */
    add_matrix_to_memory(lambdas);
    add_matrix_to_memory(eigvecs);
    printf("\nStored eigenvalues and eigenvectors in memory.\n");
}