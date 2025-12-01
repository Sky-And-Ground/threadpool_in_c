#include "thread_pool_linux.h"
#include <stdlib.h>
#include <errno.h>

static void thread_pool_clean_when_init_failed(struct ThreadPool* pool, int workers_num) {
    int i;

    /* stop the pool. */
    pthread_mutex_lock(&(pool->lock));

    pool->running = 0;
    pthread_cond_broadcast(&(pool->cond));

    pthread_mutex_unlock(&(pool->lock));

    /* join all theads. */
    for (i = 0; i < workers_num; ++i) {
        pthread_join(pool->workers[i], NULL);
    }

    /* destroy mutex and condition_variable. */
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->cond));

    /* destroy workers. */
    free(pool->workers);
}

static void* thread_pool_worker_do_work(void* param) {
    struct ThreadPool* pool = (struct ThreadPool*)param;
    struct Task* task;

    while (1) {
        pthread_mutex_lock(&(pool->lock));

        while (pool->work_queue_head == NULL && pool->running) {
            pthread_cond_wait(&(pool->cond), &(pool->lock));
        }

        task = pool->work_queue_head;
        if (task) {
            pool->work_queue_head = pool->work_queue_head->next;

            if (pool->work_queue_head == NULL) {
                pool->work_queue_tail = NULL;
            }

            pool->task_count -= 1;

            if (pool->task_count == 0) {
                pthread_cond_signal(&(pool->cond));
            }
        }

        pthread_mutex_unlock(&(pool->lock));

        if (task) {
            task->task_func(task->param);
            free(task);
        }

        if (!pool->running) {
            pthread_exit(NULL);
        }
    }
}

int thread_pool_init(struct ThreadPool* pool, int workers_num) {
    int i, err;

    pool->task_count = 0;
    pool->workers_num = workers_num;
    pool->running = 1;
    pool->work_queue_head = pool->work_queue_tail = NULL;

    pool->workers = (pthread_t*)malloc(workers_num * sizeof(pthread_t));
    if (pool->workers == NULL) {
        return ENOMEM;
    }

    err = pthread_mutex_init(&(pool->lock), NULL);
    if (err != 0) {
        free(pool->workers);
        return err;
    }

    err = pthread_cond_init(&(pool->cond), NULL);
    if (err != 0) {
        free(pool->workers);
        pthread_mutex_destroy(&(pool->lock));
        return err;
    }

    for (i = 0; i < workers_num; ++i) {
        err = pthread_create(&(pool->workers[i]), NULL, thread_pool_worker_do_work, pool);
        if (err != 0) {
            thread_pool_clean_when_init_failed(pool, i - 1);
            return err;
        }
    }

    return 0;
}

int thread_pool_add(struct ThreadPool* pool, thread_task_func func, void* param) {
    if (func == NULL) {
        return EINVAL;
    }
    else {
        struct Task* task = (struct Task*)malloc(sizeof(struct Task));

        if (task == NULL) {
            return ENOMEM;
        }

        task->task_func = func;
        task->param = param;
        task->next = NULL;

        pthread_mutex_lock(&(pool->lock));

        if (pool->work_queue_head == NULL) {
            pool->work_queue_head = task;
            pool->work_queue_tail = task;
        }
        else {
            pool->work_queue_tail->next = task;
            pool->work_queue_tail = task;
        }

        pool->task_count += 1;
        pthread_cond_signal(&(pool->cond));

        pthread_mutex_unlock(&(pool->lock));
        return 0;
    }
}

void thread_pool_wait_all(struct ThreadPool* pool) {
    pthread_mutex_lock(&(pool->lock));
    
    while (pool->task_count > 0) {
        pthread_cond_wait(&(pool->cond), &(pool->lock));
    }
    
    pthread_mutex_unlock(&(pool->lock));
}

void thread_pool_destroy(struct ThreadPool* pool) {
    int i;

    /* stop the pool. */
    pthread_mutex_lock(&(pool->lock));

    pool->running = 0;
    pthread_cond_broadcast(&(pool->cond));

    pthread_mutex_unlock(&(pool->lock));

    /* join all theads. */
    for (i = 0; i < pool->workers_num; ++i) {
        pthread_join(pool->workers[i], NULL);
    }

    /* no need to execute the remaining tasks. */
    while (pool->work_queue_head) {
        struct Task* t = pool->work_queue_head;
        pool->work_queue_head = pool->work_queue_head->next;
        free(t);
    }

    /* destroy mutex and condition_variable. */
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->cond));

    /* destroy workers. */
    free(pool->workers);
}
