#ifndef __WIN_THREAD_POOL_H__
#define __WIN_THREAD_POOL_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef void(*thread_task_func)(void* param);

struct Task {
    thread_task_func task_func;
    void* param;
    struct Task* next;
};

struct ThreadPool {
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE cond;
    HANDLE* workers;
    struct Task* work_queue_head;
    struct Task* work_queue_tail;
    int workers_num;
    int running;
};

int thread_pool_init(struct ThreadPool* pool, int workers_num);
int thread_pool_add(struct ThreadPool* pool, thread_task_func func, void* param);
void thread_pool_destroy(struct ThreadPool* pool);
void thread_pool_join_all(struct ThreadPool* pool);

#endif