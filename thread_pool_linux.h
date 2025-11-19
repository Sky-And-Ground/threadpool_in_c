#ifndef __LINUX_THREAD_POOL_H__
#define __LINUX_THREAD_POOL_H__

#include <errno.h>
#include <pthread.h>

typedef void(*thread_task_func)(void* param);

struct Task {
    thread_task_func task_func;
    void* param;
    struct Task* next;
};

struct ThreadPool {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_t* workers;
    struct Task* work_queue_head;
    struct Task* work_queue_tail;
    int workers_num;
    int running;
};

/* if success, return 0, else return a errno. */
int thread_pool_init(struct ThreadPool* pool, int workers_num);

/* if success, return 0, else return a errno. */
int thread_pool_add(struct ThreadPool* pool, thread_task_func func, void* param);

/* destroy the thread pool. */
void thread_pool_destroy(struct ThreadPool* pool);

/* join all worker threads. if thread has task, then it will finish that before it exits. */
void thread_pool_join_all(struct ThreadPool* pool);

#endif