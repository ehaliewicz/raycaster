#ifndef THREAD_H 
#define THREAD_H

#ifndef PLATFORM_WEB 
#include <windows.h>
#include <synchapi.h>
#include <winternl.h>

typedef struct thread_pool_
{
    TP_CALLBACK_ENVIRON callback_environ;
    PTP_CLEANUP_GROUP cleanup_group;
    PTP_POOL pool;
} thread_pool;


#define thread_pool_function(function_name, arg_var_name) \
    void CALLBACK function_name(PTP_CALLBACK_INSTANCE instance, PVOID arg_var_name, PTP_WORK work)


thread_pool* thread_pool_create(int cpu_threads);

void thread_pool_add_work(thread_pool* tp, PTP_WORK_CALLBACK function, void* arg_var);

void thread_pool_destroy(thread_pool* tp);
#endif
#endif 