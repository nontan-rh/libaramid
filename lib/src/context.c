#include <assert.h>

#include <aramid/aramid.h>

#include "context.h"

#include "condvar.h"
#include "executor.h"
#include "job.h"
#include "job_awaiter.h"
#include "memory_allocator.h"
#include "memory_region.h"
#include "mutex.h"
#include "procedure.h"
#include "promise.h"

ARMD_Context *armd_context_create(const ARMD_MemoryAllocator *memory_allocator,
                                  ARMD_Size num_executors) {
    assert(memory_allocator != NULL);
    assert(num_executors >= 1);

    int res = 0;
    (void)res;

    int context_initialized = 0;
    int memory_region_initialized = 0;
    int executor_mutex_initialized = 0;
    int promise_manager_mutex_initialized = 0;
    int executor_condvar_initialized = 0;
    int promise_manager_condvar_initialized = 0;
    int promise_manager_promises_initialized = 0;
    int executors_initialized = 0;

    ARMD_Context *context = NULL;

    context =
        armd_memory_allocator_allocate(memory_allocator, sizeof(ARMD_Context));
    if (context == NULL) {
        goto error;
    }
    context_initialized = 1;

    context->memory_allocator = *memory_allocator;

    context->memory_region = armd_memory_region_create(memory_allocator);
    if (context->memory_region == NULL) {
        goto error;
    }
    memory_region_initialized = 1;

    if (armd__mutex_init(&context->executor_mutex) != 0) {
        goto error;
    }
    executor_mutex_initialized = 1;

    if (armd__mutex_init(&context->promise_manager.mutex) != 0) {
        goto error;
    }
    promise_manager_mutex_initialized = 1;

    if (armd__condvar_init(&context->executor_condvar) != 0) {
        goto error;
    }
    executor_condvar_initialized = 1;

    if (armd__condvar_init(&context->promise_manager.condvar) != 0) {
        goto error;
    }
    promise_manager_condvar_initialized = 1;

    context->promise_manager.promises =
        armd__hash_table_create(context->memory_region, 16, 0.5f);
    if (context->promise_manager.promises == NULL) {
        goto error;
    }
    promise_manager_promises_initialized = 1;

    context->promise_manager.handle_counter = 0;

    context->num_executors = num_executors;
    context->executors = armd_memory_allocator_allocate(
        memory_allocator, num_executors * sizeof(ARMD__Executor *));
    if (context->executors == NULL) {
        goto error;
    }
    for (ARMD_Size i = 0; i < num_executors; i++) {
        context->executors[i] = NULL;
    }
    executors_initialized = 1;

    for (ARMD_Size i = 0; i < num_executors; i++) {
        context->executors[i] = armd__executor_create(context, i);

        if (context->executors[i] == NULL) {
            goto error;
        }
    }

    res = armd__mutex_lock(&context->executor_mutex);
    assert(res == 0);

    for (ARMD_Size i = 0; i < num_executors; i++) {
        context->executors[i]->context_ready = 1;
    }

    res = armd__condvar_broadcast(&context->executor_condvar);
    assert(res == 0);

    res = armd__mutex_unlock(&context->executor_mutex);
    assert(res == 0);

    return context;

error:
    if (executors_initialized) {
        for (ARMD_Size j = 0; j < num_executors; j++) {
            if (context->executors[j] == NULL) {
                continue;
            }
            armd__executor_stop(context->executors[j]);
        }

        for (ARMD_Size j = 0; j < num_executors; j++) {
            if (context->executors[j] == NULL) {
                continue;
            }
            res = armd__executor_destroy(context->executors[j]);
            assert(res == 0);
        }
        armd_memory_allocator_free(memory_allocator, context->executors);
    }

    if (executor_mutex_initialized) {
        res = armd__mutex_deinit(&context->executor_mutex);
        assert(res == 0);
    }

    if (promise_manager_mutex_initialized) {
        res = armd__mutex_deinit(&context->promise_manager.mutex);
        assert(res == 0);
    }

    if (executor_condvar_initialized) {
        res = armd__condvar_deinit(&context->executor_condvar);
        assert(res == 0);
    }

    if (promise_manager_condvar_initialized) {
        res = armd__condvar_deinit(&context->promise_manager.condvar);
        assert(res == 0);
    }

    if (promise_manager_promises_initialized) {
        res = armd__hash_table_destroy(context->promise_manager.promises);
        assert(res == 0);
    }

    if (memory_region_initialized) {
        armd_memory_region_destroy(context->memory_region);
    }

    if (context_initialized) {
        armd_memory_allocator_free(memory_allocator, context);
    }

    return NULL;
}

int armd_context_destroy(ARMD_Context *context) {
    int status = 0;
    int res = 0;
    (void)res;

    ARMD_MemoryAllocator memory_allocator = context->memory_allocator;

    for (ARMD_Size i = 0; i < context->num_executors; i++) {
        armd__executor_stop(context->executors[i]);
    }

    for (ARMD_Size i = 0; i < context->num_executors; i++) {
        int executor_status = armd__executor_destroy(context->executors[i]);
        if (executor_status) {
            status = -1;
        }
        context->executors[i] = NULL;
    }
    armd_memory_allocator_free(&memory_allocator, context->executors);
    context->executors = NULL;

    if (armd__hash_table_get_num_entries(context->promise_manager.promises) !=
        0) {
        status = -1;
    }

    res = armd__hash_table_destroy(context->promise_manager.promises);
    assert(res == 0);

    res = armd__mutex_deinit(&context->executor_mutex);
    assert(res == 0);
    res = armd__mutex_deinit(&context->promise_manager.mutex);
    assert(res == 0);
    res = armd__condvar_deinit(&context->executor_condvar);
    assert(res == 0);
    res = armd__condvar_deinit(&context->promise_manager.condvar);
    assert(res == 0);

    armd_memory_region_destroy(context->memory_region);

    armd_memory_allocator_free(&memory_allocator, context);

    return status;
}

static int fork_with_executor(ARMD__Executor *executor, ARMD_Job *parent_job,
                              ARMD_Procedure *procedure, void *args) {
    int res = 0;
    (void)res;

    ARMD__JobAwaiter awaiter;
    awaiter.type = JobAwaiterType_ParentJob;
    awaiter.body.parent_job.parent_job = parent_job;

    ARMD_Job *job = armd__job_create(parent_job->memory_region, executor,
                                     procedure, &awaiter, args);
    if (job == NULL) {
        return -1;
    }

    res = armd__spinlock_lock(&parent_job->lock);
    assert(res == 0);
    ++parent_job->num_all_waiting_jobs;
    res = armd__spinlock_unlock(&parent_job->lock);
    assert(res == 0);

    res = armd__spinlock_lock(&executor->lock);
    assert(res == 0);
    int enqueue_res = armd__deque_enqueue_forward(executor->deque, job);
    res = armd__spinlock_unlock(&executor->lock);
    assert(res == 0);

    if (enqueue_res != 0) {
        armd__job_destroy(job);

        res = armd__spinlock_lock(&parent_job->lock);
        assert(res == 0);
        --parent_job->num_all_waiting_jobs;
        res = armd__spinlock_unlock(&parent_job->lock);
        assert(res == 0);
        return -1;
    }

    {
        res = armd__mutex_lock(&executor->context->executor_mutex);
        assert(res == 0);

        ++executor->context->free_job_count;
        res = armd__condvar_broadcast(&executor->context->executor_condvar);
        assert(res == 0);

        res = armd__mutex_unlock(&executor->context->executor_mutex);
        assert(res == 0);
    }

    return 0;
}

int armd_fork_with_id(ARMD_Size executor_id, ARMD_Job *parent_job,
                      ARMD_Procedure *procedure, void *args) {
    ARMD_Context *context = parent_job->executor->context;
    if (executor_id >= context->num_executors) {
        return -1;
    }

    ARMD__Executor *executor = context->executors[executor_id];
    return fork_with_executor(executor, parent_job, procedure, args);
}

int armd_fork(ARMD_Job *parent_job, ARMD_Procedure *procedure, void *args) {
    ARMD__Executor *executor = parent_job->executor;
    return fork_with_executor(executor, parent_job, procedure, args);
}

/* Cleanup dependency graph for error handling */
static void cleanup_dependency_graph(ARMD_Context *context,
                                     ARMD_Size num_dependencies,
                                     const ARMD_Handle *dependencies,
                                     ARMD_Handle target) {
    int res;

    for (ARMD_Size i = 0; i < num_dependencies; i++) {
        ARMD_Handle dependency = dependencies[i];
        if (dependency == 0 ||
            dependency > context->promise_manager.handle_counter) {
            continue;
        }

        ARMD__Promise *promise;
        res = armd__hash_table_get(context->promise_manager.promises,
                                   dependency, (void **)&promise);
        if (res != 0) {
            continue;
        }
        assert(promise != NULL);

        ARMD_Size num_removed =
            armd__promise_remove_continuation_promise(promise, target);
        (void)num_removed;
        assert(num_removed <= 1);
    }
}

static int check_and_build_dependency_graph(ARMD_Context *context,
                                            ARMD_Size num_dependencies,
                                            const ARMD_Handle *dependencies,
                                            ARMD_Handle target,
                                            int *dependency_has_error) {
    int res;

    int num_waiting_promises = 0;

    *dependency_has_error = 0;
    for (ARMD_Size i = 0; i < num_dependencies; i++) {
        ARMD_Handle dependency = dependencies[i];
        if (dependency == 0) {
            continue;
        }

        ARMD__Promise *promise;
        res = armd__hash_table_get(context->promise_manager.promises,
                                   dependency, (void **)&promise);
        if (res != 0) {
            cleanup_dependency_graph(context, num_dependencies, dependencies,
                                     target);
            return -1;
        }
        assert(promise != NULL);

        if (promise->detached) {
            cleanup_dependency_graph(context, num_dependencies, dependencies,
                                     target);
            return -1;
        }

        switch (promise->status) {
        case ARMD__PromiseStatus_NotFinished:
            break;
        case ARMD__PromiseStatus_Success:
            continue;
        case ARMD__PromiseStatus_Error:
            *dependency_has_error = 1;
            continue;
        default:
            assert(0);
            break;
        }

        if (promise->status != ARMD__PromiseStatus_NotFinished) {
            continue;
        }

        res = armd__promise_add_continuation_promise(promise, target);
        if (res != 0) {
            cleanup_dependency_graph(context, num_dependencies, dependencies,
                                     target);
            return -1;
        }

        ++num_waiting_promises;
    }

    return num_waiting_promises;
}

ARMD_Handle armd_invoke(ARMD_Context *context, ARMD_Procedure *procedure,
                        void *args, ARMD_Size num_dependencies,
                        const ARMD_Handle *dependencies) {
    assert(context != NULL);
    assert(procedure != NULL);
    assert(num_dependencies == 0 || dependencies != NULL);

    int res = 0;
    (void)res;

    int promise_manager_mutex_locked = 0;
    int promise_initialized = 0;
    int job_initialized = 0;
    int dependency_graph_initialized = 0;
    int hash_table_inserted = 0;

    ARMD__Promise *promise = NULL;
    ARMD_Job *job = NULL;
    ARMD_Handle new_handle = 0;

    res = armd__mutex_lock(&context->promise_manager.mutex);
    assert(res == 0);
    promise_manager_mutex_locked = 1;

    /* handle */

    new_handle = context->promise_manager.handle_counter + 1;

    /* dependency graph */

    int ended_dependency_has_error = 0;
    int dependency_graph_res;
    if (num_dependencies == 0) {
        dependency_graph_res = 0;
    } else {
        dependency_graph_res = check_and_build_dependency_graph(
            context, num_dependencies, dependencies, new_handle,
            &ended_dependency_has_error);
    }

    if (dependency_graph_res < 0) {
        goto error;
    }
    dependency_graph_initialized = 1;

    /* awaiter */

    ARMD__JobAwaiter awaiter;
    awaiter.type = JobAwaiterType_Promise;
    awaiter.body.promise.handle = new_handle;

    /* job */

    assert(context->num_executors >= 1);
    ARMD__Executor *executor = context->executors[0];
    job = armd__job_create(context->memory_region, executor, procedure,
                           &awaiter, args);
    if (job == NULL) {
        goto error;
    }
    job_initialized = 1;

    if (ended_dependency_has_error) {
        job->dependency_has_error = 1;
    }

    /* promise */

    if (dependency_graph_res == 0) {
        promise = armd__promise_create_no_pending_job(context->memory_region);
    } else {
        promise = armd__promise_create_with_pending_job(
            context->memory_region, dependency_graph_res, job);
        armd__promise_add_reference_count(
            promise, dependency_graph_res); // For dependency graph
    }
    if (promise == NULL) {
        goto error;
    }
    promise_initialized = 1;

    armd__promise_increment_reference_count(promise); // For internal job
    // The reference count is 2 here

    if (ended_dependency_has_error) {
        promise->error_in_waiting_promises = 1;
    }

    int insert_res = armd__hash_table_insert(context->promise_manager.promises,
                                             new_handle, promise);
    if (insert_res != 0) {
        goto error;
    }
    hash_table_inserted = 1;

    /* queueing */

    if (dependency_graph_res == 0) {
        res = armd__spinlock_lock(&executor->lock);
        assert(res == 0);
        int enqueue_res = armd__deque_enqueue_back(executor->deque, job);
        res = armd__spinlock_unlock(&executor->lock);
        assert(res == 0);

        if (enqueue_res != 0) {
            goto error;
        }
    }

    /* apply and notify */

    context->promise_manager.handle_counter = new_handle;

    res = armd__mutex_unlock(&context->promise_manager.mutex);
    assert(res == 0);
    promise_manager_mutex_locked =
        0; // NOLINT(clang-analyzer-deadcode.DeadStores)

    {
        res = armd__mutex_lock(&context->executor_mutex);
        assert(res == 0);

        ++context->free_job_count;
        res = armd__condvar_broadcast(&context->executor_condvar);
        assert(res == 0);

        res = armd__mutex_unlock(&context->executor_mutex);
        assert(res == 0);
    }

    return new_handle;

error:
    if (hash_table_inserted) {
        res = armd__hash_table_remove(context->promise_manager.promises,
                                      new_handle);
        assert(res == 0);
    }

    if (dependency_graph_initialized) {
        if (num_dependencies != 0) {
            cleanup_dependency_graph(context, num_dependencies, dependencies,
                                     new_handle);
        }
    }

    if (promise_initialized) {
        res = armd__promise_destroy(promise);
        assert(res == 0);
    }

    if (job_initialized) {
        res = armd__job_destroy(job);
        assert(res == 0);
    }

    if (promise_manager_mutex_locked) {
        res = armd__mutex_unlock(&context->promise_manager.mutex);
        assert(res == 0);
    }

    return 0;
}

int armd__context_complete_promise(ARMD_Context *context,
                                   ARMD_Handle promise_handle, int has_error) {
    int res = 0;
    int mutex_locked = 0;
    int promise_to_destroy = 0;

    res = armd__mutex_lock(&context->promise_manager.mutex);
    assert(res == 0);
    mutex_locked = 1;

    ARMD__Promise *promise = NULL;
    res = armd__hash_table_get(context->promise_manager.promises,
                               promise_handle, (void **)&promise);
    if (res != 0) {
        goto error;
    }

    if (has_error) {
        promise->status = ARMD__PromiseStatus_Error;
    } else {
        promise->status = ARMD__PromiseStatus_Success;
    }

    promise_to_destroy |=
        armd__promise_decrement_reference_count(promise); // For internal job

    for (ARMD_Size i = 0; i < promise->num_promise_callbacks; i++) {
        ARMD__PromiseCallback *promise_callback =
            &promise->promise_callbacks[i];
        promise_callback->func(promise_handle, promise_callback->context,
                               has_error);
        promise_to_destroy |= armd__promise_decrement_reference_count(promise);
    }

    for (ARMD_Size i = 0; i < promise->num_continuation_promises; i++) {
        ARMD_Handle continuation_promise_handle =
            promise->continuation_promises[i];
        if (continuation_promise_handle == 0) {
            continue;
        }
        ARMD__Promise *continuation_promise;
        res = armd__hash_table_get(context->promise_manager.promises,
                                   continuation_promise_handle,
                                   (void **)&continuation_promise);
        assert(res == 0);

        assert(continuation_promise->pending_job != NULL);

        ++continuation_promise->num_ended_waiting_promises;
        assert(continuation_promise->num_ended_waiting_promises <=
               continuation_promise->num_all_waiting_promises);

        if (has_error) {
            continuation_promise->error_in_waiting_promises = 1;
        }

        if (continuation_promise->num_ended_waiting_promises >=
            continuation_promise->num_all_waiting_promises) {
            ARMD_Job *job = continuation_promise->pending_job;

            if (continuation_promise->error_in_waiting_promises) {
                job->dependency_has_error = 1;
            }

            res = armd__spinlock_lock(&job->executor->lock);
            assert(res == 0);
            int enqueue_res =
                armd__deque_enqueue_back(job->executor->deque, job);
            res = armd__spinlock_unlock(&job->executor->lock);
            assert(res == 0);

            if (enqueue_res != 0) {
                assert(0); // FIXME: Handle this error
            }

            {
                res = armd__mutex_lock(&context->executor_mutex);
                assert(res == 0);

                ++context->free_job_count;
                res = armd__condvar_broadcast(&context->executor_condvar);
                assert(res == 0);

                res = armd__mutex_unlock(&context->executor_mutex);
                assert(res == 0);
            }

            continuation_promise->pending_job = NULL;
        }

        if (armd__promise_decrement_reference_count(continuation_promise)) {
            armd__hash_table_remove(context->promise_manager.promises,
                                    continuation_promise_handle);
            armd__promise_destroy(continuation_promise);
        }
    }

    if (promise_to_destroy) {
        res = armd__hash_table_remove(context->promise_manager.promises,
                                      promise_handle);
        assert(res == 0);
        armd__promise_destroy(promise);
    }

    res = armd__condvar_broadcast(&context->promise_manager.condvar);
    assert(res == 0);

    res = armd__mutex_unlock(&context->promise_manager.mutex);
    assert(res == 0);
    mutex_locked = 0; // NOLINT(clang-analyzer-deadcode.DeadStores)

    return 0;

error:
    if (mutex_locked) {
        res = armd__mutex_unlock(&context->promise_manager.mutex);
        assert(res == 0);
    }

    return -1;
}

int armd_await(ARMD_Context *context, ARMD_Handle handle) {
    int res = 0;
    (void)res;

    res = armd__mutex_lock(&context->promise_manager.mutex);
    assert(res == 0);

    ARMD__Promise *promise;
    if (armd__hash_table_get(context->promise_manager.promises, handle,
                             (void **)&promise)) {
        res = armd__mutex_unlock(&context->promise_manager.mutex);
        assert(res == 0);
        return -1;
    }

    if (promise->detached) {
        res = armd__mutex_unlock(&context->promise_manager.mutex);
        assert(res == 0);
        return -1;
    }

    ARMD__PromiseStatus status;
    while (1) {
        status = promise->status;
        if (status != ARMD__PromiseStatus_NotFinished) {
            break;
        }

        res = armd__condvar_wait(&context->promise_manager.condvar,
                                 &context->promise_manager.mutex);
        assert(res == 0);
    }

    if (armd__promise_decrement_reference_count(promise)) {
        armd__hash_table_remove(context->promise_manager.promises, handle);
        armd__promise_destroy(promise);
    }

    res = armd__mutex_unlock(&context->promise_manager.mutex);
    assert(res == 0);

    if (status == ARMD__PromiseStatus_Success) {
        return 0;
    } else {
        return -2;
    }
}

int armd_detach(ARMD_Context *context, ARMD_Handle handle) {
    int res = 0;
    (void)res;

    res = armd__mutex_lock(&context->promise_manager.mutex);
    assert(res == 0);

    ARMD__Promise *promise;
    if (armd__hash_table_get(context->promise_manager.promises, handle,
                             (void **)&promise)) {
        res = armd__mutex_unlock(&context->promise_manager.mutex);
        assert(res == 0);
        return -1;
    }

    if (promise->detached) {
        res = armd__mutex_unlock(&context->promise_manager.mutex);
        assert(res == 0);
        return -1;
    }

    armd__promise_detach(promise);

    if (armd__promise_decrement_reference_count(promise)) {
        armd__hash_table_remove(context->promise_manager.promises, handle);
        armd__promise_destroy(promise);
    }

    res = armd__mutex_unlock(&context->promise_manager.mutex);
    assert(res == 0);

    return 0;
}

int armd_await_all(ARMD_Context *context) {
    int res = 0;
    (void)res;

    res = armd__mutex_lock(&context->promise_manager.mutex);
    assert(res == 0);

    while (armd__hash_table_get_num_entries(
               context->promise_manager.promises) != 0) {
        res = armd__condvar_wait(&context->promise_manager.condvar,
                                 &context->promise_manager.mutex);
        assert(res == 0);
    }

    res = armd__mutex_unlock(&context->promise_manager.mutex);
    assert(res == 0);

    return 0;
}

int armd_add_promise_callback(ARMD_Context *context, ARMD_Handle handle,
                              void *callback_context,
                              ARMD_PromiseCallbackFunc callback_func) {
    int res = 0;
    (void)res;

    res = armd__mutex_lock(&context->promise_manager.mutex);
    assert(res == 0);

    if (context->promise_manager.handle_counter < handle) {
        res = armd__mutex_unlock(&context->promise_manager.mutex);
        assert(res == 0);
        return -1;
    }

    ARMD__Promise *promise;
    if (armd__hash_table_get(context->promise_manager.promises, handle,
                             (void **)&promise) != 0) {
        // Promise is not found
        res = armd__mutex_unlock(&context->promise_manager.mutex);
        assert(res == 0);
        return -1;
    }

    if (promise->detached) {
        // Promise is already detached
        res = armd__mutex_unlock(&context->promise_manager.mutex);
        assert(res == 0);
        return -1;
    }

    if (promise->status == ARMD__PromiseStatus_NotFinished) {
        ARMD__PromiseCallback promise_callback;
        promise_callback.func = callback_func;
        promise_callback.context = callback_context;
        res = armd__promise_add_promise_callback(promise, &promise_callback);
        if (res != 0) {
            res = armd__mutex_unlock(&context->promise_manager.mutex);
            assert(res == 0);
            return -1;
        }
        armd__promise_increment_reference_count(promise);
    } else {
        callback_func(handle, callback_context,
                      promise->status != ARMD__PromiseStatus_Success);
    }

    res = armd__mutex_unlock(&context->promise_manager.mutex);
    assert(res == 0);

    return 0;
}
