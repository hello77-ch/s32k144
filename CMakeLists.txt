# ============================================================================
#  顶层 CMakeLists.txt —— 总控文件
# ----------------------------------------------------------------------------
#  大型项目的分层套路:顶层只做"总控",不写具体的编译细节。
#  它的职责只有三件事:
#    1. 声明工程(名字、语言、版本)
#    2. 定义全局的编译参数和开关
#    3. 把各个子目录(层)加进构建
#  编译细节全部下沉到子目录各自的 CMakeLists.txt 里。
# ============================================================================

# ----------------------------------------------------------------------------
# 指定最低 CMake 版本要求
#   - 必须是文件的第一条命令,它的作用不是"检查版本"那么简单:
#     它决定了后面所有命令按哪套"行为规范(policy)"来解释。
#   - 写 3.16 是因为我们用到了 target_link_options / add_link_options
#     (需要 3.13+),Ubuntu 20.04 自带的 3.16 刚好满足。
# ----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.16)

# ----------------------------------------------------------------------------
# 声明工程
#   - project(s32k144 ...):工程名,后面 ${PROJECT_NAME} 就引用它
#   - VERSION 1.0.0:版本号,存进变量 PROJECT_VERSION,可注入到源码
#   - LANGUAGES C ASM:启用 C 和汇编两种语言
#
#  ★ 裸机工程最容易踩的坑在这里:startup_S32K144.S 是汇编启动文件,
#    如果不写 ASM,这个文件会被 CMake 直接无视,
#    链接时报"找不到中断向量表/Reset_Handler"之类的错误。
#    (默认不加 LANGUAGES 时会启用 C+CXX,反而把用不到的 C++ 编译器检查跑一遍)
# ----------------------------------------------------------------------------
project(s32k144
    VERSION 1.0.0
    LANGUAGES C ASM
)

# ============================================================================
#  全局选项(用户可配置的开关)
# ============================================================================

# ----------------------------------------------------------------------------
# 链接脚本变体:程序最终放哪
#   set(... CACHE STRING ...):定义一个"缓存变量"——
#   它会存进 build 目录的 CMakeCache.txt,可以在命令行覆盖:
#       cmake -DLINK_VARIANT=RAM ..
#   set_property(... STRINGS ...):让 cmake-gui 里显示成下拉框,
#   纯命令行用户则当作提示文档。
# ----------------------------------------------------------------------------
set(LINK_VARIANT "FLASH" CACHE STRING "Linker script variant: FLASH or RAM")
set_property(CACHE LINK_VARIANT PROPERTY STRINGS FLASH RAM)

# ============================================================================
#  公共编译参数(对本工程所有 target、C 和汇编都生效)
# ============================================================================

# ----------------------------------------------------------------------------
# CPU 架构参数,一次性定义成变量,后面编译和链接两处复用
#   -mcpu=cortex-m4      :目标内核,S32K144 是 Cortex-M4
#   -mthumb              :Thumb/Thumb-2 指令集(Cortex-M 只能跑 Thumb,
#                         不写的话个别工具链默认 ARM 模式,链接必炸)
#
#  ★ 注意:这里故意没有 -mfpu/-mfloat-abi。
#    原因:S32DS 原工程(见 Debug_FLASH/*.args)就是软浮点配置,
#    新旧产物浮点 ABI 必须一致,否则链接时报
#    "xxx.o uses VFP register arguments"。
#    以后想启用硬件 FPU,把下面两个参数加回来:
#        -mfpu=fpv4-sp-d16 -mfloat-abi=hard
# ----------------------------------------------------------------------------
set(CPU_FLAGS -mcpu=cortex-m4 -mthumb)

# ----------------------------------------------------------------------------
# 芯片型号宏
#   S32 SDK 的头文件(device_registers.h -> S32K144.h)靠这个宏
#   识别具体型号,进而定义外设寄存器结构体和中断号。
#   没有它,编译第一行 #include 就会失败。
#   (名字来源:S32K144HFT0VLLT 是 NXP 的完整订料编号)
# ----------------------------------------------------------------------------
add_compile_definitions(CPU_S32K144HFT0VLLT)

# ----------------------------------------------------------------------------
# 编译选项
#   -Wall -Wextra          :打开大部分警告,嵌入式代码值得保持高警告级别
#   -ffunction-sections    :每个函数放进独立的段(section)
#   -fdata-sections        :每个全局变量放进独立的段
#        ↑ 这两个本身不减小体积,它们是给链接器的 --gc-sections 铺路:
#          只有"分了节",链接器才能识别出哪些节没人引用并裁掉,
#          最终把没用到的库函数(比如整个 printf 家族)从固件里剔除。
#   -fmessage-length=0     :报错信息不折行,复制粘贴更方便(抄 S32DS 的配置)
#
#  设计取舍说明:这里用的是"目录级"的 add_compile_options(),
#  因为 CPU 参数对工程内所有 target 都是同一套,放顶层写一次最省事。
#  如果某个参数只属于个别 target,就应该改用
#  target_compile_options(<某个target> PRIVATE ...) 单独指定。
# ----------------------------------------------------------------------------
add_compile_options(
    ${CPU_FLAGS}
    -ffunction-sections
    -fdata-sections
    -fmessage-length=0
    -Wall -Wextra
)

# ----------------------------------------------------------------------------
# 默认构建类型 = Debug(与 S32DS 的 Debug_FLASH 行为一致,-O0 -g)
#   - if(NOT CMAKE_BUILD_TYPE):用户没在命令行指定时才补默认值
#   - CACHE ... FORCE:写进缓存并强制覆盖旧值。
#     不加 FORCE 的话,换过一次配置后缓存里的旧值会赖着不走。
#   发布版构建:cmake -DCMAKE_BUILD_TYPE=Release ..
#     (Release 自动加 -O3 -DNDEBUG,不需要我们手写)
# ----------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ----------------------------------------------------------------------------
# 链接选项(对所有 target 生效)
#   ${CPU_FLAGS}     :链接器也要知道 CPU 型号——它要据此选择
#                     libgcc 的多库变体、决定浮点参数怎么传
#   -Wl,--gc-sections:垃圾回收未引用的段。
#                     -Wl, 的意思是"把逗号后面的参数转交给链接器 ld"。
#                     它必须和上面的 -ffunction-sections/-fdata-sections
#                     成对出现才有裁剪效果。
# ----------------------------------------------------------------------------
add_link_options(
    ${CPU_FLAGS}
    -Wl,--gc-sections
)

# ============================================================================
#  加入各层(顺序就是执行顺序)
# ============================================================================
# 启动层:把 startup.c / startup_S32K144.S / system_S32K144.c 收进
#         startup 静态库,并向外 INTERFACE 传播 include/ 头文件路径。
#         必须先加它,后面 src 层才能链接到这个库。
add_subdirectory(Project_Settings/Startup_Code)

# ============================================================================
#  FreeRTOS 内核层(使用内核自带的官方 CMakeLists.txt)
# ----------------------------------------------------------------------------
#  源码直接引用工程内已下载的 FreeRTOS-LTS,不复制文件。
#  内核的 CMake 有两个约定,由调用方(本文件)负责:
#    1. 提供变量 FREERTOS_PORT / FREERTOS_HEAP;
#    2. 把 FreeRTOSConfig.h 所在目录包成名为 freertos_config 的接口库。
# ============================================================================

# 移植层:S32K144 是带 FPU 的 Cortex-M4 → GCC(编译器)_ARM_CM4F(内核)。
# ★ 必须显式指定:不设的话在 Windows/MinGW 环境下会被内核自动识别成
#   MSVC_MINGW(PC 本机端口),交叉编译直接乱套。
#   (CACHE 变量:命令行可用 -DFREERTOS_PORT=xxx 覆盖)
set(FREERTOS_PORT GCC_ARM_CM4F CACHE STRING "FreeRTOS port")

# 堆管理方案:4 = 可释放 + 相邻空闲块合并(portable/MemMang/heap_4.c)。
# 不设的话内核不编任何堆文件,链接时报 pvPortMalloc 未定义。
set(FREERTOS_HEAP 4 CACHE STRING "FreeRTOS heap impl")

# 官方约定:配置文件目录包成 freertos_config 接口库。
# INTERFACE 库 = 零源文件、不产 .a 的"配置载体",只把头文件路径
# 传播给链接它的人;内核编 tasks.c 和我们编 main.c 都靠它
# 才能找到 FreeRTOSConfig.h。
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config
    INTERFACE ${PROJECT_SOURCE_DIR}/FreeRTOS)

# 第二个参数给子目录指定构建产物目录名,
# 避免在 FreeRTOS-LTS 源码树(带 .git)里生成任何文件。
add_subdirectory(FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel FreeRTOS-Kernel)

# 应用层:定义最终固件 s32k144.elf,链接 startup,挂链接脚本,
#         链接后自动生成 hex/bin 并打印体积。
add_subdirectory(src)
