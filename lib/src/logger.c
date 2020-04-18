#include <assert.h>

#include <aramid/aramid.h>

#include "logger.h"

static void destroy_element(ARMD_MemoryRegion *memory_region,
                            ARMD_LogElement *element) {
    armd_memory_region_free(memory_region, element->message);
    element->message = NULL;

    armd_memory_region_free(memory_region, element);
}

static void destroy_node(ARMD_MemoryRegion *memory_region,
                         ARMD__LogNode *node) {
    destroy_element(memory_region, node->log_element);
    node->log_element = NULL;

    armd_memory_region_free(memory_region, node);
}

ARMD_Logger *armd_logger_create(ARMD_MemoryRegion *memory_region,
                                ARMD_LogLevel level) {
    int res = 0;
    (void)res;

    int ring_initialized = 0;
    int mutex_initialized = 0;

    ARMD_Logger *logger =
        armd_memory_region_allocate(memory_region, sizeof(ARMD_Logger));
    if (logger == NULL) {
        return NULL;
    }

    logger->memory_region = memory_region;
    logger->level = level;

    logger->reference_count = 1;
    logger->ring =
        armd_memory_region_allocate(memory_region, sizeof(ARMD__LogNode));
    if (logger->ring == NULL) {
        goto error;
    }
    logger->ring->next = logger->ring;
    logger->ring->prev = logger->ring;
    logger->ring->log_element = NULL;
    ring_initialized = 1;

    logger->callback.func = NULL;
    logger->callback.context = NULL;

    res = armd__mutex_init(&logger->mutex);
    if (res != 0) {
        goto error;
    }
    mutex_initialized = 1;

    return logger;

error:

    if (mutex_initialized) {
        armd__mutex_deinit(&logger->mutex);
    }

    if (ring_initialized) {
        armd_memory_region_free(memory_region, logger->ring);
        logger->ring = NULL;
    }

    armd_memory_region_free(memory_region, logger);

    return NULL;
}

static void destroy_logger(ARMD_Logger *logger) {
    int res = 0;
    (void)res;

    assert(logger != NULL);
    assert(logger->reference_count == 0);

    /* Notify all awaiters to stop awaiting */

    ARMD_MemoryRegion *memory_region = logger->memory_region;

    /* Destroy */

    res = armd__mutex_deinit(&logger->mutex);
    assert(res == 0);

    ARMD__LogNode *node = logger->ring->next;
    while (node != logger->ring) {
        ARMD__LogNode *next = node->next;
        destroy_node(memory_region, node);
        node = next;
    }

    armd_memory_region_free(memory_region, logger->ring);
    logger->ring = NULL;

    armd_memory_region_free(memory_region, logger);
}

void armd_logger_increment_reference_count(ARMD_Logger *logger) {
    int res = 0;
    (void)res;

    res = armd__mutex_lock(&logger->mutex);
    assert(res == 0);

    assert(logger->reference_count >= 1);
    ++logger->reference_count;

    res = armd__mutex_unlock(&logger->mutex);
    assert(res == 0);
}

ARMD_Bool armd_logger_decrement_reference_count(ARMD_Logger *logger) {
    int res = 0;
    (void)res;

    res = armd__mutex_lock(&logger->mutex);
    assert(res == 0);

    assert(logger->reference_count >= 1);
    --logger->reference_count;

    ARMD_Bool to_destroy = logger->reference_count == 0;

    res = armd__mutex_unlock(&logger->mutex);
    assert(res == 0);

    if (to_destroy) {
        destroy_logger(logger);
    }

    return to_destroy;
}

void armd_logger_set_callback(ARMD_Logger *logger,
                              ARMD_LoggerCallbackFunc callback_func,
                              void *callback_context) {
    int res = 0;
    (void)res;

    res = armd__mutex_lock(&logger->mutex);
    assert(res == 0);

    assert(logger->reference_count >= 1);
    assert(logger->callback.func == NULL);
    assert(logger->callback.context == NULL);

    logger->callback.func = callback_func;
    logger->callback.context = callback_context;

    res = armd__mutex_unlock(&logger->mutex);
    assert(res == 0);
}

ARMD_MemoryRegion *armd_logger_get_memory_region(ARMD_Logger *logger) {
    return logger->memory_region;
}

int armd_logger_get_log_element(ARMD_Logger *logger,
                                ARMD_LogElement **log_element) {
    int res = 0;
    (void)res;

    res = armd__mutex_lock(&logger->mutex);
    assert(res == 0);

    assert(logger->reference_count >= 1);

    ARMD__LogNode *node = NULL;

    if (logger->ring->prev != logger->ring) {
        node = logger->ring->prev;

        ARMD__LogNode *next = node->next;
        ARMD__LogNode *prev = node->prev;

        node->prev->next = next;
        node->next->prev = prev;

        node->next = NULL;
        node->prev = NULL;
    }

    if (node != NULL) {
        assert(node->log_element != NULL);

        *log_element = node->log_element;
        armd_memory_region_free(logger->memory_region, node);
    } else {
        *log_element = NULL;
    }

    res = armd__mutex_unlock(&logger->mutex);
    assert(res == 0);

    if (node == NULL) {
        return -1;
    } else {
        return 0;
    }
}

void armd_logger_destroy_log_element(ARMD_Logger *logger,
                                     ARMD_LogElement *log_element) {
    assert(logger != NULL);
    assert(log_element != NULL);

    destroy_element(logger->memory_region, log_element);
}

void armd_logger_log(ARMD_Logger *logger, ARMD_LogLevel level, char *message) {
    int res = 0;
    (void)res;

    res = armd__mutex_lock(&logger->mutex);
    assert(res == 0);

    assert(logger->reference_count >= 1);

    if (level > logger->level) {
        armd_memory_region_free(logger->memory_region, message);

        res = armd__mutex_unlock(&logger->mutex);
        assert(res == 0);
        return;
    }

    ARMD_LoggerCallbackFunc callback_func = logger->callback.func;
    void *callback_context = logger->callback.context;

    if (callback_func != NULL) {
        ++logger->reference_count; // For callback
    }

    ARMD_LogElement *log_element = armd_memory_region_allocate(
        logger->memory_region, sizeof(ARMD_LogElement));

    armd_get_time(&log_element->timespec);
    log_element->level = level;
    log_element->message = message;

    ARMD__LogNode *log_node = armd_memory_region_allocate(
        logger->memory_region, sizeof(ARMD__LogNode));
    log_node->log_element = log_element;

    ARMD__LogNode *next = logger->ring->next;
    logger->ring->next = log_node;
    next->prev = log_node;
    log_node->next = next;
    log_node->prev = logger->ring;

    res = armd__mutex_unlock(&logger->mutex);
    assert(res == 0);

    if (callback_func != NULL) {
        callback_func(callback_context, logger);
        armd_logger_decrement_reference_count(logger); // For callback
    }
}

void armd_log_fatal(ARMD_Logger *logger, char *message) {
    armd_logger_log(logger, ARMD_LogLevel_Fatal, message);
}

void armd_log_error(ARMD_Logger *logger, char *message) {
    armd_logger_log(logger, ARMD_LogLevel_Error, message);
}

void armd_log_warn(ARMD_Logger *logger, char *message) {
    armd_logger_log(logger, ARMD_LogLevel_Warn, message);
}

void armd_log_info(ARMD_Logger *logger, char *message) {
    armd_logger_log(logger, ARMD_LogLevel_Info, message);
}

void armd_log_debug(ARMD_Logger *logger, char *message) {
    armd_logger_log(logger, ARMD_LogLevel_Debug, message);
}

void armd_log_trace(ARMD_Logger *logger, char *message) {
    armd_logger_log(logger, ARMD_LogLevel_Trace, message);
}
