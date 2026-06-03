/**
 * @file freertos_tasks.h
 * @brief FreeRTOS task initialization API
 */

#ifndef FREERTOS_TASKS_H
#define FREERTOS_TASKS_H

/**
 * @brief Create FreeRTOS objects (queues, event groups, mutexes)
 */
void freertos_objects_init(void);

/**
 * @brief Create and start all FreeRTOS tasks + scheduler
 */
void freertos_tasks_init(void);

#endif /* FREERTOS_TASKS_H */
