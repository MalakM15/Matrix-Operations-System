#ifndef MATRIX_OPS_H
#define MATRIX_OPS_H

#include "matrix_manager.h"   /* For Matrix struct */
#include "child_pool.h"       /* For OperationType enum */
#include <time.h>

double get_time_ms(); // Time measurement
Matrix add_matrices_single_thread(Matrix A, Matrix B,int ompFlag);
Matrix sub_matrices_single_thread(Matrix A, Matrix B,int ompFlag);
Matrix add_matrices_multi_process(Matrix A, Matrix B);
Matrix sub_matrices_multi_process(Matrix A, Matrix B);
int validate_same_dimensions(Matrix A, Matrix B);
int validate_multiplication_dimensions(Matrix A, Matrix B);
Matrix multiply_matrices_single_thread(Matrix A, Matrix B);
Matrix multiply_matrices_multi_process(Matrix A, Matrix B,int openmpFlag) ;
float determinant_multi(Matrix A,int openmpFlag);
float compute_det_sequential(float *data, int n, int openmpFlag);
void compute_eigen();
#endif /* MATRIX_OPS_H */