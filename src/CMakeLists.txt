# ============================================================================
#  应用层 CMakeLists.txt —— 最终固件 s32k144.elf
# ----------------------------------------------------------------------------
#  这一层做四件事:定义可执行文件、链接启动库、挂链接脚本、
#  链接完成后自动生成烧录文件并报告体积。
# ============================================================================

# ----------------------------------------------------------------------------
# 最终可执行文件(对嵌入式来说 .elf 就是固件本体)
#   名字直接带 .elf 后缀,和 S32DS 的产物命名保持一致,
#   原有的 Segger .launch 调试配置改个路径就能继续用。
#   main.c 等源文件列在这里;以后加新文件就往后面追加。
# ----------------------------------------------------------------------------
add_executable(s32k144.elf main.c)

# ----------------------------------------------------------------------------
# 链接启动代码库 + FreeRTOS 内核
#   链接 startup:拿到向量表、Reset_Handler,以及 INTERFACE 传播的
#   include/ 路径(main.c 由此找到 device_registers.h)。
#   链接 freertos_kernel:拿到内核静态库;FreeRTOS.h/task.h 的内核路径
#   和 FreeRTOSConfig.h 的路径(freertos_config)会经 PUBLIC 链自动
#   传播过来,应用层无需任何 include_directories()。
# ----------------------------------------------------------------------------
target_link_libraries(s32k144.elf PRIVATE startup freertos_kernel)

# ----------------------------------------------------------------------------
# 选择链接脚本:描述 Flash/RAM 的地址和容量,指挥代码数据怎么摆放
#   默认 FLASH 版(代码烧进 0x0 起始的 Flash);
#   顶层开关 LINK_VARIANT=RAM 时换成 RAM 版,
#   供"整个程序加载进 RAM 跑"的调试场景使用。
# ----------------------------------------------------------------------------
set(LINKER_SCRIPT ${PROJECT_SOURCE_DIR}/Project_Settings/Linker_Files/S32K144_64_flash.ld)
if(LINK_VARIANT STREQUAL "RAM")
    set(LINKER_SCRIPT ${PROJECT_SOURCE_DIR}/Project_Settings/Linker_Files/S32K144_64_ram.ld)
endif()

# ----------------------------------------------------------------------------
# 链接选项(只作用于本 target)
#   -T<脚本>          :指定链接脚本
#   --specs=nano.specs:改用 newlib-nano(裁剪版 C 库),
#                      同样功能体积小一半以上,printf 默认还不带浮点
#   --specs=nosys.specs:裸机没有 OS,open/read/write 这些系统调用
#                      用空桩函数顶替,否则链接时找不到符号
#   -Wl,-Map=...      :生成 map 文件——"固件体检报告",
#                      每个函数/变量占了哪个地址、多大,查体积超标全靠它
# ----------------------------------------------------------------------------
target_link_options(s32k144.elf PRIVATE
    -T${LINKER_SCRIPT}
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/s32k144.map
)

# ----------------------------------------------------------------------------
# 链接成功后的自动后处理
#   add_custom_command(... POST_BUILD):挂一个"构建完成钩子",
#   elf 每次重新生成,下面三条命令就自动执行一次。
#
#   $<TARGET_FILE:s32k144.elf> 是"生成器表达式":
#   构建阶段才展开成 elf 的完整路径,比手写路径可靠
#   (输出目录变了也不用回来改这里)。
#
#   ihex 和 binary 两种格式:
#     - hex  带地址信息、文本格式,JLink/调试器直接认;
#     - bin  纯二进制镜像,给 DFU/sdcard 裸写 Flash 的场景用。
#   两个都生成,烧录工具要哪种用哪种。
#
#   size 输出的三列怎么读:
#     text = Flash 占用(代码 + 常量 + 已初始化数据)
#     data = 已初始化变量的 Flash 占用(会被 startup 搬到 RAM)
#     bss  = RAM 占用(零初始化变量,不占 Flash)
# ----------------------------------------------------------------------------
add_custom_command(TARGET s32k144.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:s32k144.elf> s32k144.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:s32k144.elf> s32k144.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:s32k144.elf>
    COMMENT "Generating hex/bin and printing size"
)
