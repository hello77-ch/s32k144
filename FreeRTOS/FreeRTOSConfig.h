/*
 * FreeRTOSConfig.h —— S32K144 工程专用的 FreeRTOS 内核配置
 * 内核版本:FreeRTOS-Kernel V11.3.1(位于 FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel)
 * 移植层:portable/GCC/ARM_CM4F(S32K144 为 Cortex-M4F)
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ---------------- 硬件相关 ---------------- */
/* 本工程 SystemInit 不配时钟,复位后核心时钟 = SIRC 8MHz。
 * ★ 以后若加了时钟初始化(如 SPLL 80MHz),这里必须同步改,
 *   否则 tick 会按比例变快/变慢。 */
#define configCPU_CLOCK_HZ                    8000000U
#define configTICK_RATE_HZ                    1000U    /* 1ms 一拍 */

/* ---------------- 调度行为 ---------------- */
#define configUSE_PREEMPTION                  1
#define configUSE_TIME_SLICING                1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  1     /* CM4F 有 CLZ 指令 */
#define configMAX_PRIORITIES                  5
#define configMINIMAL_STACK_SIZE              128      /* 单位:字(4字节),Idle 任务栈 */
#define configMAX_TASK_NAME_LEN               16
#define configTICK_TYPE_WIDTH_IN_BITS         TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD               1
#define configUSE_TICKLESS_IDLE               0

/* ---------------- 内存管理(heap_4) ---------------- */
#define configSUPPORT_STATIC_ALLOCATION       0
#define configSUPPORT_DYNAMIC_ALLOCATION      1
#define configTOTAL_HEAP_SIZE                 ( 16 * 1024 )  /* ucHeap 落在 .bss */
#define configAPPLICATION_ALLOCATED_HEAP      0
#define configHEAP_CLEAR_MEMORY_ON_FREE       0
#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP 0

/* ---------------- 内核功能开关 ---------------- */
#define configUSE_TIMERS                      0   /* 要用软件定时器改 1,并补 TIMER_TASK_* 四项 */
#define configUSE_TASK_NOTIFICATIONS          1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES 1
#define configUSE_MUTEXES                     1
#define configUSE_RECURSIVE_MUTEXES           1
#define configUSE_COUNTING_SEMAPHORES         1
#define configUSE_QUEUE_SETS                  0
#define configQUEUE_REGISTRY_SIZE             0
#define configUSE_EVENT_GROUPS                1
#define configUSE_STREAM_BUFFERS              1
#define configUSE_CO_ROUTINES                 0
#define configUSE_NEWLIB_REENTRANT            0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0
#define configUSE_APPLICATION_TASK_TAG        0

/* ---------------- 中断优先级(S32K144 的 NVIC 实现 4 位 = 16 级) ---------------- */
/* 内核此版本把 PendSV/SysTick 固定在最低优先级,此项按惯例声明 */
#define configKERNEL_INTERRUPT_PRIORITY       15U
/* 数值 >= 5 的中断才允许调用 ...FromISR API;
 * 0~4 级(数值更小 = 更高)中断不被内核屏蔽,严禁在其中调 FreeRTOS API */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY  5U

/* ---------------- 调试辅助 ---------------- */
#define configCHECK_FOR_STACK_OVERFLOW        2   /* 需要在 main.c 实现 hook */
#define configASSERT( x )  if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ) { ; } }

/* ---------------- API 裁剪 ---------------- */
#define INCLUDE_vTaskDelay                    1
#define INCLUDE_xTaskDelayUntil               1
#define INCLUDE_vTaskDelete                   1
#define INCLUDE_vTaskSuspend                  1
#define INCLUDE_vTaskPrioritySet              1
#define INCLUDE_uxTaskPriorityGet             1
#define INCLUDE_xTaskGetSchedulerState        1
#define INCLUDE_xTaskGetCurrentTaskHandle     1
#define INCLUDE_xTaskResumeFromISR            1
#define INCLUDE_uxTaskGetStackHighWaterMark   1
#define INCLUDE_xTaskGetIdleTaskHandle        1
#define INCLUDE_eTaskGetState                 0
#define INCLUDE_xTimerPendFunctionCall        0
#define INCLUDE_xTaskAbortDelay               0
#define INCLUDE_xTaskGetHandle                0

/* ---------------- 中断入口接管(核心!) ----------------
 * 内核移植层的三个函数改名为向量表期待的符号。
 * startup_S32K144.S 里这三项是 .weak 别名到 DefaultISR,
 * port.c 编出同名强符号后自动覆盖,向量表一个字都不用改。 */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
