#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include "win_thread_pool.h"

void test_worker_func(void* param) {
    printf("%d, yes, just for test\n", GetCurrentThreadId());
}

int main(void) {
    struct ThreadPool pool;

    thread_pool_init(&pool, 4);

    thread_pool_add(&pool, test_worker_func, NULL);
    thread_pool_add(&pool, test_worker_func, NULL);
    thread_pool_add(&pool, test_worker_func, NULL);
    thread_pool_add(&pool, test_worker_func, NULL);
    thread_pool_add(&pool, test_worker_func, NULL);
    thread_pool_add(&pool, test_worker_func, NULL);
    thread_pool_add(&pool, test_worker_func, NULL);
    thread_pool_add(&pool, test_worker_func, NULL);

    thread_pool_join_all(&pool);
    thread_pool_destroy(&pool);
    return 0;
}
