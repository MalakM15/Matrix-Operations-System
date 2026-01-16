#include "local_header.h"

static ChildProcess *pool = NULL;  /* Dynamic array of workers */
static int max_children = 0;       /* Maximum allowed workers */
static int current_children = 0;   /* Currently created workers */
static TaskTracker current_task; // tracks the last submitted task
static int task_submitted = 0;  

static void child_process_loop(int read_fd, int write_fd,int openmpFlag) {

    while (1) {
        int batch_size;

        /* Read batch size */
        if (read(read_fd, &batch_size, sizeof(int)) <= 0) {
            exit(0); /* Parent closed pipe */
        }

        /* Exit signal */
        if (batch_size == -1) {
            exit(0);
        }

        Task tasks[batch_size];
        TaskResult results[batch_size];

        /* Read the batch of tasks */
        if (read(read_fd, tasks, batch_size * sizeof(Task)) <= 0) {
            perror("Child read failed");
            exit(1);
        }

        /* Process tasks */
        for (int i = 0; i < batch_size; i++) {
            results[i].index = tasks[i].index;

            switch (tasks[i].op) {
                case OP_ADD:
                    results[i].result = tasks[i].a + tasks[i].b;
                    break;

                case OP_SUB:
                    results[i].result = tasks[i].a - tasks[i].b;
                    break;
                case OP_MUL: 
                    float sum = 0;
                    
                    #pragma omp parallel for reduction(+:sum) num_threads(8) if(openmpFlag)
                
                    for (int k = 0; k < tasks[i].A_cols; k++) {
                        sum += tasks[i].A[tasks[i].row * tasks[i].A_cols + k] *
                               tasks[i].B[k * tasks[i].B_cols + tasks[i].col];
                    }
                    results[i].result = sum;
                    break;
                
                case OP_DET:
                int n = tasks[i].A_rows;
                int n_sub = n - 1;
                float *minor = malloc(n * n * sizeof(float));
                if (!minor) { perror("malloc"); exit(1); }

               // build minor excluding row 0 and column task->minor_skip_col

               int mi = 0;
               for (int r = 0; r < n; r++) { 
                   if (r == 0) continue;
                for (int c = 0; c < n; c++) {
                       if (c == tasks[i].minor_skip_col) continue;
                       minor[mi++] = tasks[i].A[r * n + c];
                   }
               }

               results->result = compute_det_sequential(minor, n_sub, openmpFlag);
               free(minor);
               break;
            }
        }

        /* Send results back */
        if (write(write_fd, results, batch_size * sizeof(TaskResult)) <= 0) {
            perror("Child write failed");
            exit(1);
        }
    }
}
void init_pool(int max) {
    max_children = max;
    current_children = 0;
    pool = (ChildProcess *) malloc(max_children * sizeof(ChildProcess));

    if (!pool) {
        perror("Pool allocation failed");
        exit(1);
    }
}

int add_child_to_pool(int openmpFlag) {
    if (current_children >= max_children) {
        return -1;  /* Pool full */
    }

    ChildProcess *child = &pool[current_children];

    /* Create pipes for communication */
    if (pipe(child->pipe_parent_to_child) == -1 ||
        pipe(child->pipe_child_to_parent) == -1) {
        perror("Pipe creation failed");
        exit(1);
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("Fork failed");
        exit(1);
    }

    if (pid == 0) {
        // -------------------- CHILD CODE -------------------- 
        close(child->pipe_parent_to_child[1]); /* Close write-end (parent->child) */
        close(child->pipe_child_to_parent[0]); /* Close read-end  (child->parent) */

        child_process_loop(child->pipe_parent_to_child[0],child->pipe_child_to_parent[1],openmpFlag);

        exit(0);
    }
    /* -------------------- PARENT CODE -------------------- */
    child->pid = pid;
    child->busy = 0;

    close(child->pipe_parent_to_child[0]); /* Parent does not read here */
    close(child->pipe_child_to_parent[1]); /* Parent does not write here */

    current_children++;
    return current_children - 1;
}

int get_free_child() {
    for (int i = 0; i < current_children; i++) {
        if (pool[i].busy == 0) {
            return i;
        }
    }
    return -1;
}

void pool_assign_batch(int child_index, Task *tasks, int batch_size) {
    ChildProcess *child = &pool[child_index];
    child->busy = 1;

    /* Send batch size */
    if (write(child->pipe_parent_to_child[1], &batch_size, sizeof(int)) <= 0) {
        perror("Parent write failed (batch size)");
        exit(1);
    }

    /* Send tasks */
    if (write(child->pipe_parent_to_child[1], tasks, batch_size * sizeof(Task)) <= 0) {
        perror("Parent write failed (tasks)");
        exit(1);
    }
}

void pool_collect_batch(int child_index, TaskResult *results, int batch_size) {
    ChildProcess *child = &pool[child_index];

    /* Read results */
    if (read(child->pipe_child_to_parent[0], results, batch_size * sizeof(TaskResult)) <= 0) {
        perror("Parent read failed (results)");
        exit(1);
    }

    child->busy = 0; /* Mark worker as free */
}

void pool_assign_task(void (*task_fn)(void*), void* arg) {
    if (task_submitted) {
        fprintf(stderr, "Error: a task is already running!\n");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {
        // CHILD executes the task
        task_fn(arg);
        exit(0); // child exits after finishing
    } else {
        // PARENT stores info
        current_task.task_fn = task_fn;
        current_task.arg = arg;
        current_task.pid = pid;
        task_submitted = 1;
    }
}
void pool_wait_task() {
    if (!task_submitted) return; // nothing to wait for

    waitpid(current_task.pid, NULL, 0); // wait for child
    task_submitted = 0;                 // reset flag
}
int pool_get_current_children() {
    return current_children;
}
int pool_get_max_children() {
    return max_children;
}