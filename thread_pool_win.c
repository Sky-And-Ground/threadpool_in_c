#include "win_thread_pool.h"

static void thread_pool_clean_when_init_failed(struct ThreadPool* pool, int workers_num) {
    int i;

    /* stop the pool. */
    EnterCriticalSection(&(pool->lock));

    pool->running = 0;
    WakeAllConditionVariable(&(pool->cond));

    LeaveCriticalSection(&(pool->lock));

    /* wait for all threads. */
    for (i = 0; i < workers_num; ++i) {
        WaitForSingleObject(pool->workers[i], INFINITE);
        CloseHandle(pool->workers[i]);
    }

    /* destroy mutex. */
    DeleteCriticalSection(&(pool->lock));

    /* destroy workers. */
    free(pool->workers);
}

static DWORD WINAPI thread_pool_worker_do_work(LPVOID param) {
    struct ThreadPool* pool = (struct ThreadPool*)param;
    struct Task* task;

    while (1) {
        EnterCriticalSection(&(pool->lock));

        while (pool->work_queue_head == NULL && pool->running) {
            SleepConditionVariableCS(&(pool->cond), &(pool->lock), INFINITE);
        }

        task = pool->work_queue_head;
        if (task) {
            pool->work_queue_head = pool->work_queue_head->next;

            if (pool->work_queue_head == NULL) {
                pool->work_queue_tail = NULL;
            }
        }

        LeaveCriticalSection(&(pool->lock));

        if (task) {
            task->task_func(task->param);
            free(task);
        }

        if (!pool->running) {
            break;
        }
    }

    return 0;
}

int thread_pool_init(struct ThreadPool* pool, int workers_num) {
    int i, err;
    DWORD threadId;

    pool->workers_num = workers_num;
    pool->running = 1;
    pool->work_queue_head = pool->work_queue_tail = NULL;

    pool->workers = (HANDLE*)malloc(workers_num * sizeof(HANDLE));
    if (pool->workers == NULL) {
        return ERROR_OUTOFMEMORY;
    }

    InitializeCriticalSection(&(pool->lock));
    InitializeConditionVariable(&(pool->cond));

    for (i = 0; i < workers_num; ++i) {
        pool->workers[i] = CreateThread(NULL, 0, thread_pool_worker_do_work, pool, 0, &threadId);

        if (pool->workers[i] == NULL) {
            err = GetLastError();
            thread_pool_clean_when_init_failed(pool, i);
            return err;
        }
    }

    return ERROR_SUCCESS;
}

int thread_pool_add(struct ThreadPool* pool, thread_task_func func, void* param) {
    if (func == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    else {
        struct Task* task = (struct Task*)malloc(sizeof(struct Task));

        if (task == NULL) {
            return ERROR_OUTOFMEMORY;
        }

        task->task_func = func;
        task->param = param;
        task->next = NULL;

        EnterCriticalSection(&(pool->lock));

        if (pool->work_queue_head == NULL) {
            pool->work_queue_head = task;
            pool->work_queue_tail = task;
        }
        else {
            pool->work_queue_tail->next = task;
            pool->work_queue_tail = task;
        }

        WakeConditionVariable(&(pool->cond));

        LeaveCriticalSection(&(pool->lock));
        return 0;
    }
}

void thread_pool_destroy(struct ThreadPool* pool) {
    int i;

    /* stop the pool. */
    EnterCriticalSection(&(pool->lock));

    pool->running = 0;
    WakeAllConditionVariable(&(pool->cond));

    LeaveCriticalSection(&(pool->lock));

    /* wait for all threads. */
    for (i = 0; i < pool->workers_num; ++i) {
        WaitForSingleObject(pool->workers[i], INFINITE);
        CloseHandle(pool->workers[i]);
    }

    /* no need to execute the remaining tasks. */
    while (pool->work_queue_head) {
        struct Task* t = pool->work_queue_head;
        pool->work_queue_head = pool->work_queue_head->next;
        free(t);
    }

    /* destroy critical section */
    DeleteCriticalSection(&(pool->lock));

    /* destroy workers. */
    free(pool->workers);
}

void thread_pool_join_all(struct ThreadPool* pool) {
    int i;

    EnterCriticalSection(&(pool->lock));

    pool->running = 0;
    WakeAllConditionVariable(&(pool->cond));

    LeaveCriticalSection(&(pool->lock));

    for (i = 0; i < pool->workers_num; ++i) {
        WaitForSingleObject(pool->workers[i], INFINITE);
    }
}
