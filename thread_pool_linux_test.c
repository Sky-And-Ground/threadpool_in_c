#include <stdio.h>
#include "thread_pool_linux.h"

void test_worker_func(void* param) {
    printf("%d, yes, just for test\n", pthread_self());
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

    thread_pool_wait_all(&pool);
    thread_pool_destroy(&pool);
    return 0;
}
