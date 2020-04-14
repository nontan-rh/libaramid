/**
 * @mainpage Public API
 * Public API is described in the header page @ref aramid/aramid.h
 */
/**
 * @file aramid/aramid.h
 * @brief libaramid public API header
 */

#ifndef ARAMID_ARAMID_H
#define ARAMID_ARAMID_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#else
#include <stddef.h>
#include <stdint.h>
#endif

#ifdef __cplusplus
#define ARMD_EXTERN_C extern "C"
#else
#define ARMD_EXTERN_C
#endif

/**
 * @brief Size type
 */
typedef size_t ARMD_Size;
/**
 * @brief Boolean type
 */
typedef int ARMD_Bool;
/**
 * @brief Floating point value type
 */
typedef double ARMD_Real;
/**
 * @brief Handle type
 */
typedef uint64_t ARMD_Handle;

/**
 * @brief Aramid Job
 * @details This is used when implementing @ref ARMD_Procedure
 */
typedef struct TAG_ARMD_Job ARMD_Job;
/**
 * @brief Aramid Procedure
 * @details This is something like functions but consists of many continuations
 * which are represented with many fragment functions. The procedure also has
 * meta infomation to execute in the thread pool.
 */
typedef struct TAG_ARMD_Procedure ARMD_Procedure;

/**
 * @brief Memory allocation function interface
 * @param context Memory allocator context. See @ref
 * ARMD_MemoryAllocator.context
 * @param size Allocation size in bytes. It can be zero but not recommended.
 * @return The pointer to allocated memory. You can return NULL if failed.
 */
typedef void *(*ARMD_MemoryAllocatorAllocateFunc)(void *context,
                                                  ARMD_Size size);
/**
 * @brief Memory free function interface
 * @param context Memory allocator context. See @ref
 * ARMD_MemoryAllocator.context
 * @param buf The pointer to free. It must be the same pointer returned by @ref
 * ARMD_MemoryAllocatorAllocateFunc,
 */
typedef void (*ARMD_MemoryAllocatorFreeFunc)(void *context, void *buf);

/**
 * @brief Abstract memory allocator
 * @details It provides the capability of switching malloc implementation
 * or injecting your own memory allocation algorithm. this struct is used not
 * like a reference but like a value. That is, the members of @ref
 * ARMD_MemoryAllocator are copied into another @ref ARMD_MemoryAllocator.
 */
typedef struct TAG_ARMD_MemoryAllocator {
    /**
     * @brief Memory allocator context
     * @details The pointer passed to @ref ARMD_MemoryAllocatorAllocateFunc @ref
     * allocate and @ref ARMD_MemoryAllocatorFreeFunc @ref free
     */
    void *context;
    /**
     * @brief Memory allocation function
     */
    ARMD_MemoryAllocatorAllocateFunc allocate;
    /**
     * @brief Memory free function
     */
    ARMD_MemoryAllocatorFreeFunc free;
} ARMD_MemoryAllocator;

/**
 * @brief Allocates memory area with @ref ARMD_MemoryAllocator
 * @param allocator The memory allocator
 * @param size Allocation size in bytes. It can be zero but not recommended.
 * @return The pointer to allocated memory area. NULL if failed.
 */
ARMD_EXTERN_C void *
armd_memory_allocator_allocate(const ARMD_MemoryAllocator *allocator,
                               ARMD_Size size);
/**
 * @brief Free memory area allocated with @ref ARMD_MemoryAllocator
 * @param allocator The memory allocator. It must be the same allocator used on
 * allocation.
 * @param buf The pointer to free. It must be the same pointer returned by @ref
 * armd_memory_allocator_allocate.
 */
ARMD_EXTERN_C void
armd_memory_allocator_free(const ARMD_MemoryAllocator *allocator, void *buf);

/**
 * @brief Initialize @ref ARMD_MemoryAllocator with default value
 * @details It initializes @ref ARMD_MemoryAllocator to use the system's malloc
 * and free from stdlib.h
 * @param memory_allocator The memory allocator to initialize
 */
ARMD_EXTERN_C void
armd_memory_allocator_init_default(ARMD_MemoryAllocator *memory_allocator);

/**
 * @brief Memory region
 * @details It provides region-based memory management functionality. It is also
 * called memory zone or arena in some projects. Memory areas allocated in the
 * region can be freed in a batch or it enables to detect memory leakage in fine
 * granularity. In contrast to @ref ARMD_MemoryAllocator, this struct is used as
 * a reference.
 */
typedef struct TAG_ARMD_MemoryRegion ARMD_MemoryRegion;

/**
 * @brief Create @ref ARMD_MemoryRegion
 * @param memory_allocator The memory allocator to use
 * @return The pointer to the new @ref ARMD_MemoryRegion. NULL if failed.
 */
ARMD_EXTERN_C ARMD_MemoryRegion *
armd_memory_region_create(const ARMD_MemoryAllocator *memory_allocator);
/**
 * @brief Destroy @ref ARMD_MemoryRegion
 * @param memory_region The memory region to destroy
 * @return the count of freed memory areas
 */
ARMD_EXTERN_C ARMD_Size
armd_memory_region_destroy(ARMD_MemoryRegion *memory_region);

/**
 * @brief Allocates memory area with @ref ARMD_MemoryRegion
 * @param allocator The memory region
 * @param size Allocation size in bytes. It can be zero but not recommended.
 * @return The pointer to allocated memory area. NULL if failed.
 */
ARMD_EXTERN_C void *
armd_memory_region_allocate(ARMD_MemoryRegion *memory_region, ARMD_Size size);
/**
 * @brief Free memory area allocated with @ref ARMD_MemoryRegion
 * @param allocator The memory region. It must be the same region used on
 * allocation.
 * @param buf The pointer to free. It must be the same pointer returned by @ref
 * armd_memory_region_allocate.
 */
ARMD_EXTERN_C void armd_memory_region_free(ARMD_MemoryRegion *memory_region,
                                           void *buf);

/**
 * @brief The execution engine
 * @details The global state for execution engine. It contains native threads,
 * job deque, promise manager and so on. Most your API calls are done against
 * this structure.
 */
typedef struct TAG_ARMD_Context ARMD_Context;

/**
 * @brief Create @ref ARMD_Context
 * @param memory_allocator The memory allocator used everywhere related to this
 * context
 * @param num_executors The number of executors, in other words, concurrency
 * @return The new ARMD_Context. NULL if failed.
 */
ARMD_EXTERN_C ARMD_Context *
armd_context_create(const ARMD_MemoryAllocator *memory_allocator,
                    ARMD_Size num_executors);
/**
 * @brief Destroy @ref ARMD_Context
 * @param context The @ref  ARMD_Context to destroy
 * @return Status code, 0 if succeeded, non-zero if otherwise
 */
ARMD_EXTERN_C int armd_context_destroy(ARMD_Context *context);

/**
 * @brief Invoke procedure
 * @param context The @ref ARMD_Context to run the @ref procedure in
 * @param procedure The @ref ARMD_Procedure to run
 * @param args The arguments to pass into @ref ARMD_Procedure
 * @param num_dependencies The number of elements in @ref dependencies
 * @param dependencies The array of handles of dependent promises, this should
 * be a valid pointer even if @ref num_dependencies == 0
 * @return The new handle of promise, 0 if failure
 */
ARMD_EXTERN_C ARMD_Handle armd_invoke(ARMD_Context *context,
                                      ARMD_Procedure *procedure, void *args,
                                      ARMD_Size num_dependencies,
                                      const ARMD_Handle *dependencies);
/**
 * @brief Await promise
 * @details This function locks the caller thread and it will not return until
 * the promise is completed.
 * @param context The @ref ARMD_Context which promise belongs to
 * @param handle The @ref ARMD_Handle of the promise to be awaited
 * @return Status code, 0 if succeeded, non-zero if otherwise
 */
ARMD_EXTERN_C int armd_await(ARMD_Context *context, ARMD_Handle handle);

/**
 * @brief Detach promise
 * @details This function detaches the job.
 * @param context The @ref ARMD_Context which promise belongs to
 * @param handle The @ref ARMD_Handle of the promise to be detached
 * @return Status code, 0 if succeeded, non-zero if otherwise
 */
ARMD_EXTERN_C int armd_detach(ARMD_Context *context, ARMD_Handle handle);

/**
 * @brief Await all promise
 */
ARMD_EXTERN_C int armd_await_all(ARMD_Context *context);

/**
 * @brief Promise callback
 * @details The function called when promise resolved. See @ref
 * armd_add_promise_callback.
 */
typedef void (*ARMD_PromiseCallbackFunc)(ARMD_Handle handle,
                                         void *callback_context, int has_error);
/**
 * @brief Add callback to the promise
 * @details Add a function to be called when the promise being resolved.
 * Note that the thread which invokes the callback is not indeterminate and the
 * promise_callback may be invoked immediately. Many API in libaramid is not
 * reentrant. If you calls any API of libaramid in this callback, a deadlock may
 * occur.
 */
ARMD_EXTERN_C
int armd_add_promise_callback(ARMD_Context *context, ARMD_Handle handle,
                              void *callback_context,
                              ARMD_PromiseCallbackFunc callback_func);

/**
 * @brief Get the number of executors in the @ref ARMD_Context via @ref
 * ARMD_Job
 * @details This function returns the number of executors in the @ref
 * ARMD_Context which the @ref job belongs to. This is useful for
 * implementing your @ref ARMD_Procedure.
 * @param job The current @ref ARMD_Job
 * @return The number of executors
 */
ARMD_EXTERN_C ARMD_Size armd_job_get_num_executors(ARMD_Job *job);
/**
 * @brief Get the executor id via @ref ARMD_Job
 * @details This function returns the id of executors which is running the @ref
 * job. This is useful for implementing your @ref ARMD_Procedure.
 * @param job The current @ref ARMD_Job
 * @return The executor id
 */
ARMD_EXTERN_C ARMD_Size armd_job_get_executor_id(ARMD_Job *job);

/**
 * @brief Fork and invoke procedure
 * @details This function forks the thread of execution and invoke the @ref
 * procedure in it. The new job is queued into the current executor. Note that
 * the job created by this API is reassigned to another executor by
 * work-stealing system.
 * @param parent_job The current @ref ARMD_Job
 * @param procedure The @ref ARMD_Procedure to run
 * @param args The arguments to pass into @ref ARMD_Procedure
 * @return Status code, 0 if succeeded, non-zero if otherwise
 */
ARMD_EXTERN_C int armd_fork(ARMD_Job *parent_job, ARMD_Procedure *procedure,
                            void *args);
/**
 * @brief Fork and invoke procedure specifying the executor
 * @details This function forks the thread of execution and invoke the @ref
 * procedure in it. The new job is queued into the executor specified with @ref
 * executor_id. Note that the job created by this API is reassigned to another
 * executor by work-stealing system.
 * @param executor_id The id of executor to run @ref procedure in
 * @param parent_job The current @ref ARMD_Job
 * @param procedure The @ref ARMD_Procedure to run
 * @param args The arguments to pass into @ref ARMD_Procedure
 * @return Status code, 0 if succeeded, non-zero if otherwise
 */
ARMD_EXTERN_C int armd_fork_with_id(ARMD_Size executor_id, ARMD_Job *parent_job,
                                    ARMD_Procedure *procedure, void *args);

/**
 * @brief Continuation Status
 */
typedef enum TAG_ARMD_ContinuationResult {
    ARMD_ContinuationResult_Error,
    ARMD_ContinuationResult_Ended,
    ARMD_ContinuationResult_Repeat,
} ARMD_ContinuationResult;

/**
 * @brief Continuation Function
 * @details This is something like functions but consists of many continuations
 * which are represented with many fragment functions. The procedure also has
 * meta infomation to execute in the thread pool.
 */
typedef ARMD_ContinuationResult (*ARMD_ContinuationFunc)(
    ARMD_Job *job, const void *constants, void *args, void *frame,
    const void *continuation_constants, void *continuation_frame);

/**
 * @brief Error Trap Function
 */
typedef ARMD_ContinuationResult (*ARMD_ErrorTrapFunc)(
    ARMD_Job *job, const void *constants, void *args, void *frame,
    const void *continuation_constants, void *continuation_frame);

/**
 * @brief Continuation Frame Creator
 * @details This function allocates and initializes continuation frame.
 * @param memory_region The memory region for the continuation frame to live in
 * @return The new continuation frame. You can return NULL if failed.
 */
typedef void *(*ARMD_ContinuationFrameCreator)(
    ARMD_MemoryRegion *memory_region);
/**
 * @brief Continuation Frame Destroyer
 * @details This function allocates and initializes continuation frame.
 * @param memory_region The memory region in which the continuation frame lives
 * @param continuation_frame The continuation frame to be freed
 */
typedef void (*ARMD_ContinuationFrameDestroyer)(
    ARMD_MemoryRegion *memory_region, void *continuation_frame);

/**
 * @brief Single Continuation Function
 * @details The simplified continuation function to be used in @ref
 * armd_then_single. It omits continuation_constants, continuation_frame, and
 * the ability to run repeatedly.
 */
typedef int (*ARMD_SingleContinuationFunc)(ARMD_Job *job, const void *constants,
                                           void *args, void *frame);

/**
 * @brief Sequential-For Count Function
 * @details The delegate used to get the number of times to run repeatedly
 */
typedef ARMD_Size (*ARMD_SequentialForCountFunc)(void *args, void *frame);
/**
 * @brief Sequential-For Continuation Function
 * @details The simplified continuation function to be used in @ref
 * armd_then_sequential_for. It omits continuation_constants and
 * continuation_frame and it receives the repetition counter as @ref index.
 */
typedef int (*ARMD_SequentialForContinuationFunc)(ARMD_Job *job,
                                                  const void *constants,
                                                  void *args, void *frame,
                                                  ARMD_Size index);

/**
 * @brief Unwind Function
 * @details The simplified continuation function to be used in @ref
 * armd_then_sequential_for. It omits continuation_constants and
 * continuation_frame and it receives the repetition counter as @ref index.
 */
typedef void (*ARMD_UnwindFunc)(ARMD_Job *job, const void *constants,
                                void *args, void *frame);

/**
 * @brief Builder object for @ref ARMD_Procedure
 */
typedef struct TAG_ARMD_ProcedureBuilder ARMD_ProcedureBuilder;

/**
 * @brief Create @ref ARMD_ProcedureBuilder
 * @param memory_allocator The memory allocator to use
 * @param constant_size The size of constant table
 * @param frame_size The size of frame
 * @return The pointer to the new @ref ARMD_ProcedureBuilder. NULL if failed.
 */
ARMD_EXTERN_C ARMD_ProcedureBuilder *
armd_procedure_builder_create(const ARMD_MemoryAllocator *memory_allocator,
                              ARMD_Size constant_size, ARMD_Size frame_size);
/**
 * @brief Destroy @ref ARMD_ProcedureBuilder without building @ref
 * ARMD_Procedure
 * @param procedure_builder The procedure builder to destroy
 * @return Status code, 0 if succeeded, non-zero if otherwise
 */
ARMD_EXTERN_C int
armd_procedure_builder_destroy(ARMD_ProcedureBuilder *procedure_builder);
/**
 * @brief Build @ref ARMD_Procedure and destroy @ref ARMD_ProcedureBuilder
 * @param procedure_builder The procedure builder to build and destroy
 * @return The pointer to the new @ref ARMD_Procedure. NULL if failed.
 */
ARMD_EXTERN_C ARMD_Procedure *armd_procedure_builder_build_and_destroy(
    ARMD_ProcedureBuilder *procedure_builder);
/**
 * @brief Get the pointer to constant table
 * @param procedure_builder The procedure builder
 * @return The pointer to constant table. Always non-NULL.
 */
ARMD_EXTERN_C void *
armd_procedure_builder_get_constants(ARMD_ProcedureBuilder *procedure_builder);
/**
 * @brief Get the memory allocator used in @ref ARMD_ProcedureBuilder
 * @param procedure_builder The procedure builder
 * @return The memory allocator used in @ref ARMD_ProcedureBuilder. Always
 * non-NULL.
 */
ARMD_EXTERN_C ARMD_MemoryAllocator armd_procedure_builder_get_memory_allocator(
    ARMD_ProcedureBuilder *procedure_builder);

/**
 * @brief Appends a versatile continuation to the procedure
 * @details The versatile API to add the new continuation at last of the
 * continuation sequence in @ref ARMD_ProcedureBuilder. You should not use this
 * function unless you are implementing scheduling function. @ref
 * armd_then_single and @ref armd_then_sequential_for is much simpler and
 * enough.
 */
ARMD_EXTERN_C int
armd_then(ARMD_ProcedureBuilder *procedure_builder,
          ARMD_ContinuationFunc continuation_func, void *continuation_constants,
          ARMD_ErrorTrapFunc error_trap_func,
          ARMD_ContinuationFrameCreator continuation_frame_creator,
          ARMD_ContinuationFrameDestroyer continuation_frame_destroyer);

/**
 * @brief Appends a single continuation to the procedure
 * @details Adds a single continuation at last of the continuation sequence in
 * @ref ARMD_ProcedureBuilder. A single continuation is simply called once per
 * invocation.
 */
ARMD_EXTERN_C int
armd_then_single(ARMD_ProcedureBuilder *procedure_builder,
                 ARMD_SingleContinuationFunc single_continuation_func);

/**
 * @brief Appends a sequential-for continuation to the procedure
 * @details Adds a sequential-for continuation at last of the continuation
 * sequence in @ref ARMD_ProcedureBuilder. A sequential-for continuation @ref
 * sequential_for_continuation_func is called repeatedly several times specified
 * by @ref sequential_for_count_func.
 */
ARMD_EXTERN_C int armd_then_sequential_for(
    ARMD_ProcedureBuilder *procedure_builder,
    ARMD_SequentialForCountFunc sequential_for_count_func,
    ARMD_SequentialForContinuationFunc sequential_for_continuation_func);

/**
 * @brief Add unwind callback to the procedure
 * @details Adds the unwind callback to the procedure. The unwind callback is
 * called when procedures exit independent of the execution status.
 */
ARMD_EXTERN_C int armd_unwind(ARMD_ProcedureBuilder *procedure_builder,
                              ARMD_UnwindFunc unwind_func);

/**
 * @brief Destroy @ref ARMD_Procedure
 * @param memory_region The procedure to destroy
 * @return Status code, 0 if succeeded, non-zero if otherwise
 */
ARMD_EXTERN_C int armd_procedure_destroy(ARMD_Procedure *procedure);

/**
 * @brief Get the pointer to constant table
 * @param procedure The procedure
 * @return The pointer to constant table. Always non-NULL.
 */
ARMD_EXTERN_C void *armd_procedure_get_constants(ARMD_Procedure *procedure);

#endif
