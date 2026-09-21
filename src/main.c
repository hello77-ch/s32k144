/*
 * main.c —— FreeRTOS 演示入口
 * ----------------------------------------------------------------------------
 * 移植 FreeRTOS 后的第一个应用:创建两个周期任务并启动调度器。
 *
 * 时钟说明:本工程 SystemInit() 不配置时钟,复位后核心时钟为
 * SIRC 8MHz,FreeRTOSConfig.h 的 configCPU_CLOCK_HZ 与之对应(1ms 一拍)。
 * 以后若加了时钟初始化,必须同步修改 configCPU_CLOCK_HZ。
 *
 * 观察方法:本工程尚未接 LED/串口驱动,两个任务各自维护一个计数器,
 * 调试时把它们加入 watch 窗口(或周期性挂起查看),可见计数随各自的
 * 周期独立增长,即说明任务切换与 SysTick 均已正常工作。
 *
 * ★ 原裸机示例中的 SVC_Handler 已删除:它与 FreeRTOS 移植层的
 *   vPortSVCHandler(经 FreeRTOSConfig.h 改名为 SVC_Handler)撞名,
 *   保留会导致链接报 multiple definition。
 */
#include "FreeRTOS.h"
#include "task.h"

#include "S32K144.h"

/* 任务 A:每 500ms 计数一次 */
static volatile uint32_t taskA_counter = 0;

/* 任务 B:每 200ms 计数一次 */
static volatile uint32_t taskB_counter = 0;

/* ----------------------------------------------------------------------------
 * 任务函数的统一原型:void f(void *参数)
 * 任务体内绝不允许 return——要结束就删任务或死循环。
 * ---------------------------------------------------------------------------- */
static void vTaskA(void *pvParameters)
{
    (void)pvParameters;
    for (;;) {
        taskA_counter++;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void vTaskB(void *pvParameters)
{
    (void)pvParameters;
    for (;;) {
        taskB_counter++;
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/* ----------------------------------------------------------------------------
 * 主函数:创建任务 → 启动调度器(之后 CPU 永远在任务间切换,不再返回)
 * ---------------------------------------------------------------------------- */
int main(void)
{
    BaseType_t ret;

    /* 栈大小单位是"字"(4 字节):128*2 = 1KB,对这种空转任务绰绰有余。
     * 优先级 1 = 高于 Idle(0),同级任务间由时间片轮转。 */
    ret = xTaskCreate(vTaskA, "TaskA", configMINIMAL_STACK_SIZE * 2,
                      NULL, 1, NULL);
    configASSERT(ret == pdPASS);        /* 创建失败通常是堆不够 */

    ret = xTaskCreate(vTaskB, "TaskB", configMINIMAL_STACK_SIZE * 2,
                      NULL, 1, NULL);
    configASSERT(ret == pdPASS);

    vTaskStartScheduler();

    /* 正常情况下永远执行不到这里;能到,说明调度器启动失败
     * (典型原因:configTOTAL_HEAP_SIZE 不够创建 Idle 任务)。 */
    for (;;) {
        ;
    }
}

/* ----------------------------------------------------------------------------
 * 栈溢出钩子:configCHECK_FOR_STACK_OVERFLOW = 2 时内核强制要求提供。
 * 每次上下文切换时内核检查任务栈尾的填充图案是否被破坏,
 * 溢出则带出"哪个任务"调用本函数。此处关中断停机,便于接调试器定位。
 * ---------------------------------------------------------------------------- */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for (;;) {
        ;
    }
}
