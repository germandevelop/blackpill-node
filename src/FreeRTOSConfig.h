#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "board.config.h"

// FreeRTOS Kernel V11.0.1

// Exception handlers
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

// Clock
extern uint32_t SystemCoreClock;
#define configCPU_CLOCK_HZ (SystemCoreClock)
#define configTICK_RATE_HZ ((TickType_t)1000)
#define configUSE_16_BIT_TICKS 0

// ISR priorities
#define LOWEST_INTERRUPT_PRIORITY       15  // 0 - highest, 15 -lowest
#define MAX_SYSCALL_INTERRUPT_PRIORITY  5   // The highest interrupt priority to use 'fromISR' functions

#define configPRIO_BITS                       4   // __NVIC_PRIO_BITS (2^4=16 - number of priorities)
#define configMAX_SYSCALL_INTERRUPT_PRIORITY  (MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configKERNEL_INTERRUPT_PRIORITY       (LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

// Scheduler
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configIDLE_SHOULD_YIELD                 1

// Memory
#define configAPPLICATION_ALLOCATED_HEAP          0
#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP 0
#define configSUPPORT_STATIC_ALLOCATION           0
#define configSUPPORT_DYNAMIC_ALLOCATION          1
#define configTOTAL_HEAP_SIZE                     CONFIG_RTOS_HEAP_SIZE
#define configSTACK_DEPTH_TYPE                    uint16_t
#define configMINIMAL_STACK_SIZE                  ((uint16_t)128)
#define configTIMER_TASK_STACK_DEPTH              ((uint16_t)1024) // 1024 * 4 = 4096 bytes

// Hooks
#define configUSE_IDLE_HOOK                 0
#define configUSE_TICK_HOOK                 0
#define configCHECK_FOR_STACK_OVERFLOW      0
#define configUSE_MALLOC_FAILED_HOOK        0
#define configUSE_DAEMON_TASK_STARTUP_HOOK  0

// Power consumption
#define configUSE_TICKLESS_IDLE                 1
#define configEXPECTED_IDLE_TIME_BEFORE_SLEEP   5

// Debug
#define configGENERATE_RUN_TIME_STATS         0
#define configUSE_TRACE_FACILITY              0
#define configUSE_STATS_FORMATTING_FUNCTIONS  0
#define configQUEUE_REGISTRY_SIZE             0

#ifndef NDEBUG
#include <assert.h>
#define configASSERT(x) assert((x) != 0)
#endif

// Tasks
#define configMAX_PRIORITIES    (5) // 0 - lowest, 4 - highest
#define configMAX_TASK_NAME_LEN (16)

// Sync
#define configUSE_TASK_NOTIFICATIONS      1
#define configUSE_MUTEXES                 1
#define configUSE_RECURSIVE_MUTEXES       0
#define configUSE_COUNTING_SEMAPHORES     0
#define configUSE_QUEUE_SETS              1
#define configMESSAGE_BUFFER_LENGTH_TYPE  size_t

// Timers
#define configUSE_TIMERS              1
#define configTIMER_TASK_PRIORITY     (0)
#define configTIMER_QUEUE_LENGTH      32

// Co-routines
#define configUSE_CO_ROUTINES           0
#define configMAX_CO_ROUTINE_PRIORITIES (2)

// Compatibility
#define configUSE_ALTERNATIVE_API           0
#define configENABLE_BACKWARD_COMPATIBILITY 0
#define configUSE_NEWLIB_REENTRANT          0

// Optional functions - most linkers will remove unused functions anyway
#define INCLUDE_vTaskPrioritySet 1
#define INCLUDE_uxTaskPriorityGet 1
#define INCLUDE_vTaskDelete 0
#define INCLUDE_vTaskCleanUpResources 0
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskGetSchedulerState 0
#define INCLUDE_xTimerPendFunctionCall 0
#define INCLUDE_xQueueGetMutexHolder 0
#define INCLUDE_xTaskGetCurrentTaskHandle 0
#define INCLUDE_eTaskGetState 0
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_uxTaskGetStackHighWaterMark 0

#endif // FREERTOS_CONFIG_H
