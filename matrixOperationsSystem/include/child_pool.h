#ifndef CHILD_POOL_H
#define CHILD_POOL_H

#include <sys/types.h>
#include <omp.h>

// Represents a worker (child process) in the pool
typedef struct {
    int pid;                          // Child PID
    int pipe_parent_to_child[2];      // Parent writes, child reads
    int pipe_child_to_parent[2];      // Child writes, parent reads
    int busy;                         // 0 = free, 1 = processing
} ChildProcess;

typedef enum {
    OP_ADD = 0, /**< Element-wise addition */
    OP_SUB = 1,  /**< Element-wise subtraction */
    OP_MUL ,
    OP_DET
} OperationType;

// Represents a single task sent to a worker (child process).
// One task performs one operation on one matrix element.

typedef struct {
    int index;      // Index of the element inside the matrix (1D representation) //
    float a, b;     // for add/sub
    int row, col;   // for multiply
    int A_rows, A_cols, B_cols;
    float *A;
    float *B;
    OperationType op;
    int minor_skip_col; //for determinant
} Task;
// Represents a result returned by a worker for one matrix element.
typedef struct {
    int index;   /**< Same index that was sent in Task */
    float result;/**< Computed result for the given element */
} TaskResult;

typedef struct TaskTracker {
    void (*task_fn)(void*);
    void *arg;
    pid_t pid;
} TaskTracker;


// ---- Pool Initialization --
 //Initializes the worker (child) pool.
 //Must be called once before adding workers or assigning tasks.
void init_pool(int max);
//Creates a new child process and adds it to the pool.
int add_child_to_pool(int openmpFlag);
int get_free_child();
void pool_assign_batch(int child_index, Task *tasks, int batch_size);
void pool_collect_batch(int child_index, TaskResult *results, int batch_size);
int pool_get_current_children();
int pool_get_max_children();
int get_free_child();
void pool_assign_task(void (*task_fn)(void*), void* arg);
void pool_wait_task();
int pool_get_current_children();

#endif /* CHILD_POOL_H */