// optimized version of matrix column normalization
#include "colnorm.h"

////////////////////////////////////////////////////////////////////////////////
// REQUIRED: Paste a copy of your sumdiag_benchmark from an ODD grace
// node below.
//
// -------REPLACE WITH YOUR RUN + TABLE --------
//
// grace9:~/216-sync/p5-code: ./colnorm_benchmark
// ==== Matrix Column Normalization Benchmark Version 1.1 ====
// Running with REPEATS: 2 and WARMUP: 1
// Running with 4 sizes and 4 thread_counts (max 4)
//   ROWS   COLS   BASE  T   OPTM SPDUP POINT TOTAL 
//   1111   2223  0.144  1  0.115  1.25  0.32  0.32 
//                       2  0.090  1.60  0.68  1.00 
//                       3  0.083  1.74  0.80  1.80 
//                       4  0.089  1.62  0.69  2.49 
//   2049   4098  0.950  1  0.398  2.39  1.26  3.75 
//                       2  0.200  4.76  2.25  6.00 
//                       3  0.205  4.63  2.21  8.21 
//                       4  0.146  6.53  2.71 10.92 
//   4099   8197  3.623  1  0.590  6.14  2.62 13.54 
//                       2  0.360 10.06  3.33 16.87 
//                       3  0.341 10.63  3.41 20.28 
//                       4  0.294 12.33  3.62 23.90 
//   6001  12003  6.155  1  1.256  4.90  2.29 26.19 
//                       2  0.680  9.05  3.18 29.37 
//                       3  0.680  9.06  3.18 32.55 
//                       4  0.617  9.98  3.32 35.87 
// RAW POINTS: 35.87
// TOTAL POINTS: 35 / 35
// -------REPLACE WITH YOUR RUN + TABLE --------

// You can write several different versions of your optimized function
// in this file and call one of them in the last function.

// context struct for thread workers to cooperate on column sums
typedef struct {
  int thread_id;              // logical id of thread, 0,1,2,...
  int thread_count;           // total threads working on summing
  matrix_t mat;               // matrix to sum
  vector_t avg;               // vector to place mean
  vector_t std;               // vector to place standard_dev
  pthread_mutex_t *vec_lock;  // mutex to lock the vec before adding on results
} colnorm_context_t;

void *col_norm_worker(void *arg) {
  // Extract the parameters / "context" via a cast
  colnorm_context_t ctx = *((colnorm_context_t *)arg);

  // Extract the matrix and vector for the parameter context struct
  matrix_t mat = ctx.mat;
  vector_t avg = ctx.avg;
  vector_t std = ctx.std;

  // Calculate how much work this thread should do and where its
  // begin/end columns are located
  int cols_per_thread = mat.cols / ctx.thread_count;
  int beg_col = cols_per_thread * ctx.thread_id;
  int end_col = cols_per_thread * (ctx.thread_id + 1);
  if(ctx.thread_id == ctx.thread_count - 1){
    end_col = mat.cols;
  }

  // Allocate local arrays just for the columns this thread handles
  int local_cols = end_col - beg_col;
  double *sums = calloc(local_cols, sizeof(double));
  double *avg_local = calloc(local_cols, sizeof(double));
  double *std_local = calloc(local_cols, sizeof(double));
  

  // Compute column sums in row-major order
  for (int i = 0; i < mat.rows; i++) {
    for (int j = beg_col; j < end_col; j++) {
      int local_idx = j - beg_col;
      sums[local_idx] += MGET(mat, i, j);
    }
  }
  
  // Calculate averages
  for (int j = 0; j < local_cols; j++) {
    avg_local[j] = sums[j] / mat.rows;
  }
  
  // Reset sums array to reuse for variance calculation
  memset(sums, 0, local_cols * sizeof(double));
  
  // Compute sum of squared differences in row-major order
  for (int i = 0; i < mat.rows; i++) {
    for (int j = beg_col; j < end_col; j++) {
      int local_idx = j - beg_col;
      double diff = MGET(mat, i, j) - avg_local[local_idx];
      sums[local_idx] += diff * diff;
    }
  }
  
  // Calculate standard deviations
  for (int j = 0; j < local_cols; j++) {
    std_local[j] = sqrt(sums[j] / mat.rows);
  }
  
  // Normalize matrix columns in row-major order
  for (int i = 0; i < mat.rows; i++) {
    for (int j = beg_col; j < end_col; j++) {
      int local_idx = j - beg_col;
      double normalized_val = (MGET(mat, i, j) - avg_local[local_idx]) / std_local[local_idx];
      MSET(mat, i, j, normalized_val);
    }
  }

  // Lock the mutex to safely update the shared vectors
  pthread_mutex_lock(ctx.vec_lock);

  // Copy results to the shared vectors
  for (int j = 0; j < local_cols; j++) {
    VSET(avg, beg_col + j, avg_local[j]);
    VSET(std, beg_col + j, std_local[j]);
  }

  // Unlock the mutex
  pthread_mutex_unlock(ctx.vec_lock);

  // Free local arrays before ending
  free(sums);
  free(avg_local);
  free(std_local);
  return NULL;
}

int cn_verA(matrix_t *mat_ptr, vector_t *avg_ptr, vector_t *std_ptr, int thread_count) {
  // OPTIMIZED CODE HERE

  // initialize the shared results vector
  for (int j = 0; j < mat_ptr->cols; j++) {
    VSET(*avg_ptr, j, 0);
    VSET(*std_ptr, j, 0);
  }

  if(thread_count < 1) thread_count = 1;  // Minimum 1 thread
  if(thread_count > 4) thread_count = 4;  // Your defined upper limit

  // init a mutex to be used for threads to add on their local results
  pthread_mutex_t vec_lock;
  pthread_mutex_init(&vec_lock, NULL);

  pthread_t threads[thread_count];       // track each thread
  colnorm_context_t ctxs[thread_count];  // context for each trhead

  // loop to create threads
  for (int i = 0; i < thread_count; i++) {
    // Fill field values for the context to pass to threads;
    // thread IDs are 0,1,2,... and are used to determine which part
    // of the matrix to operate on. The remaining data are copies of
    // local data that enable the thread to "see" the matrix and
    // vector as well as utilize the mutex for coordination.
    ctxs[i].thread_id = i;
    ctxs[i].thread_count = thread_count;
    ctxs[i].mat = *mat_ptr;
    ctxs[i].avg = *avg_ptr;
    ctxs[i].std = *std_ptr;
    ctxs[i].vec_lock = &vec_lock;

    // Loop to create the threads
    pthread_create(&threads[i], NULL,  // start worker thread to compute part of answer
                   col_norm_worker, &ctxs[i]);
  }

  // Use a loop to join the threads
  for (int i = 0; i < thread_count; i++) {
    pthread_join(threads[i], NULL);
  }

  pthread_mutex_destroy(&vec_lock);  // get rid of the lock to avoid a memory leak
  return 0;
}

int cn_verB(matrix_t *mat_ptr, vector_t *avg_ptr, vector_t *std_ptr, int thread_count) {
  // OPTIMIZED CODE HERE
  return 0;
}

int colnorm_OPTM(matrix_t *mat_ptr, vector_t *avg_ptr, vector_t *std_ptr, int thread_count) {
  // call your preferred version of the function
  return cn_verA(mat_ptr, avg_ptr, std_ptr, thread_count);
}