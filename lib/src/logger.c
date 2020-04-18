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

ARMD_Logger *armd_logger_create(ARMD_MemoryRegion *memory_region) {
    int res = 0;
    (void)res;

    int ring_initialized = 0;
    int mutex_initialized = 0;
    int condvar_initialized = 0;

    ARMD_Logger *logger =
        armd_memory_region_allocate(memory_region, sizeof(ARMD_Logger));
    if (logger == NULL) {
        return NULL;
    }

    logger->memory_region = memory_region;

    logger->is_destroying = 0;
    logger->awaiter_count = 0;
    logger->ring =
        armd_memory_region_allocate(memory_region, sizeof(ARMD__LogNode));
    if (logger->ring == NULL) {
        goto error;
    }
    logger->ring->next = logger->ring;
    logger->ring->prev = logger->ring;
    logger->ring->log_element = NULL;
    ring_initialized = 1;

    res = armd__mutex_init(&logger->mutex);
    if (res != 0) {
        goto error;
    }
    mutex_initialized = 1;

    res = armd__condvar_init(&logger->condvar);
    if (res != 0) {
        goto error;
    }
    condvar_initialized = 1;

    return logger;

error:
    if (condvar_initialized) {
        armd__condvar_deinit(&logger->condvar);
    }

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

void armd_logger_destroy(ARMD_Logger *logger) {
    int res = 0;
    (void)res;

    assert(logger != NULL);

    /* Notify all awaiters to stop awaiting */

    ARMD_MemoryRegion *memory_region = logger->memory_region;

    res = armd__mutex_lock(&logger->mutex);
    assert(res == 0);

    logger->is_destroying = 1;
    armd__condvar_broadcast(&logger->condvar);

    while (logger->awaiter_count != 0) {
        res = armd__condvar_wait(&logger->condvar, &logger->mutex);
        assert(res == 0);
    }

    res = armd__mutex_unlock(&logger->mutex);
    assert(res == 0);

    /* Destroy */

    res = armd__condvar_deinit(&logger->condvar);
    assert(res == 0);

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

ARMD_MemoryRegion *armd_logger_get_memory_region(ARMD_Logger *logger) {
    assert(!logger->is_destroying);

    return logger->memory_region;
}

int armd_logger_get_log_element(ARMD_Logger *logger,
                                ARMD_LogElement **log_element) {
    int res = 0;
    (void)res;

    assert(!logger->is_destroying);

    res = armd__mutex_lock(&logger->mutex);
    assert(res == 0);

    ++logger->awaiter_count;

    ARMD_Bool is_destroying = 0;
    ARMD__LogNode *node = NULL;
    while (1) {
        if (logger->is_destroying) {
            is_destroying = 1;

            break;
        }

        if (logger->ring->prev != logger->ring) {
            node = logger->ring->prev;

            ARMD__LogNode *next = node->next;
            ARMD__LogNode *prev = node->prev;

            node->prev->next = next;
            node->next->prev = prev;

            node->next = NULL;
            node->prev = NULL;

            break;
        }

        armd__condvar_wait(&logger->condvar, &logger->mutex);
    }

    if (node != NULL) {
        assert(!is_destroying);
        assert(node->log_element != NULL);

        *log_element = node->log_element;
        armd_memory_region_free(logger->memory_region, node);
    } else {
        assert(is_destroying);

        *log_element = NULL;
    }

    --logger->awaiter_count;

    res = armd__mutex_unlock(&logger->mutex);
    assert(res == 0);

    if (is_destroying) {
        return -1;
    } else {
        return 0;
    }
}

void armd_logger_destroy_log_element(ARMD_Logger *logger,
                                     ARMD_LogElement *log_element) {
    assert(logger != NULL);
    assert(log_element != NULL);
    assert(!logger->is_destroying);

    destroy_element(logger->memory_region, log_element);
}

void armd_logger_log(ARMD_Logger *logger, ARMD_LogLevel level, char *message) {
    int res = 0;
    (void)res;

    assert(!logger->is_destroying);

    res = armd__mutex_lock(&logger->mutex);
    assert(res == 0);

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
