cmake_minimum_required(VERSION 2.6)
set(UNIX "On") # set the C output file extension to .o instead of .obj
project(diaforus_final_code)

option(SIMULATION "Generate a Makefile suited for simulation or the real nodes" ON)

if (NOT APPLICATION)
	set (APPLICATION "reasoning")
endif(NOT APPLICATION)

if (NOT SIMULATION AND NOT WAVENIS_LINKER_SCRIPT)
	set (WAVENIS_LINKER_SCRIPT "link_ram_39k_flashD.x")
else (NOT SIMULATION AND NOT WAVENIS_LINKER_SCRIPT)
	set (WAVENIS_LINKER_SCRIPT "")
endif(NOT SIMULATION AND NOT WAVENIS_LINKER_SCRIPT)

if (NOT SIMULATION AND NOT WAVENIS_LIB)
	set (WAVENIS_LIB "waveliblgyC11_V1")
else (NOT SIMULATION AND NOT WAVENIS_LIB)
	set (WAVENIS_LIB "")
endif(NOT SIMULATION AND NOT WAVENIS_LIB)

include_directories(BEFORE "${CMAKE_BINARY_DIR}")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/config")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/framework")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/simulated_layers")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/rpl")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/uip")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/application")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/application/${APPLICATION}")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/coap")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/bsp/include")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/bsp/platform/mpw5")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/pubsub")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/plugins")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/plugins/plugin_example")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/pubsub")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/wavenis")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/drivers/include/")
include_directories(BEFORE "${CMAKE_SOURCE_DIR}/management/")
if(SIMULATION)
	include_directories(BEFORE "${CMAKE_SOURCE_DIR}/freertos_simulator/include")
	include_directories(BEFORE "${CMAKE_SOURCE_DIR}/freertos_simulator/portable/GCC/posix")
	include_directories(BEFORE "${CMAKE_SOURCE_DIR}/wavenis_simulator")
else(SIMULATION)
	include_directories(BEFORE "${CMAKE_SOURCE_DIR}/freertos/include")
	include_directories(BEFORE "${CMAKE_SOURCE_DIR}/freertos/portage")
	include_directories(BEFORE "${CMAKE_SOURCE_DIR}/freertos/portable/GCC/posix")
endif(SIMULATION)

#default values
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif(NOT CMAKE_BUILD_TYPE)

if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}")
endif(NOT CMAKE_INSTALL_PREFIX)

if (SIMULATION)
	find_package (Threads REQUIRED)
endif(SIMULATION)

file(GLOB diaforus_SOURCES
  "framework/*.c" "framework/*.h"
  "wavenis/*.c" "wavenis/*.h"
  "rpl/*.c" "rpl/*.h"
  "coap/*.c" "coap/*.h"
  "uip/*.c" "uip/*.h"
  "simulated_layers/*.c" "simulated_layers/*.h"
  "application/*.c" "application/*.h"
  "application/${APPLICATION}/*.c" "application/${APPLICATION}/*.h"
  "bsp/platform/mpw5/machine/*.c" "bsp/platform/mpw5/machine/*.h"
  "bsp/platform/mpw5/custom/*.c" "bsp/platform/mpw5/custom/*.h"
  "bsp/platform/mpw5/sys/*.c" "bsp/platform/mpw5/sys/*.h"
  "bsp/include/*.c" "bsp/include/*.h"
  "pubsub/*.c" "pubsub/*.h"
  "plugins/*.c" "plugins/*.h"
  "plugins/plugin_example/*.c" "plugins/plugin_example/*.h"
  "management/*.c" "management/*.h"
)

if(SIMULATION)
	file(GLOB other_SOURCES
		"wavenis_simulator/*.c" "wavenis_simulator/*.h"
		"freertos_simulator/*.c" "freertos_simulator/*.h"
		"freertos_simulator/portable/*.c" "freertos_simulator/portable/*.h"
		"freertos_simulator/portable/GCC/posix/*.c" "freertos_simulator/portable/GCC/posix/*.h"
		"freertos_simulator/portable/MemMang/*.c" "freertos_simulator/portable/MemMang/*.h"
		"freertos_simulator/include/*.c" "freertos_simulator/include/*.h"
	)
else(SIMULATION)
	file(GLOB other_SOURCES
		"freertos/src/*.c" "freertos/src/*.h"
		"freertos/portage/*.c" "freertos/portage/*.h"
		"freertos/portage/GCC/posix/*.c" "freertos/portage/GCC/posix/*.h"
		"freertos/portage/MemMang/*.c" "freertos/portage/MemMang/*.h"
		"freertos/include/*.c" "freertos/include/*.h"
		"drivers/src/i2c/arch/culprit/*.c" "drivers/src/i2c/arch/culprit/*.h"
		"drivers/src/gpio/arch/culprit/*.c" "drivers/src/gpio/arch/culprit/*.h"
		"drivers/src/timer/arch/culprit/*.c" "drivers/src/timer/arch/culprit/*.h"
		"drivers/src/mcp23018/arch/culprit/*.c" "drivers/src/mcp23018/arch/culprit/*.h"
		"drivers/src/power/arch/culprit/*.c" "drivers/src/power/arch/culprit/*.h"
		"drivers/src/uart/arch/culprit/*.c" "drivers/src/uart/arch/culprit/*.h"
		"drivers/src/devices/mcp23018/*.c" "drivers/src/devices/mcp23018/*.h"
		"drivers/src/devices/spirit/*.c" "drivers/src/devices/spirit/*.h"
	)
endif(SIMULATION)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --save-temps -fverbose-asm -Os -Wall -Wno-strict-aliasing -Wcast-align -Wno-cast-align -fdata-sections -ffunction-sections -fno-align-loops -fno-move-loop-invariants -fno-peel-loops -fno-prefetch-loop-arrays -fno-rerun-cse-after-loop -fno-reschedule-modulo-scheduled-loops -fno-tree-loop-im -fno-tree-loop-ivcanon -fno-tree-loop-linear -fno-tree-loop-optimize -fno-tree-vect-loop-version -fno-unroll-all-loops -fno-unroll-loops -fno-loop-optimize -fshort-enums") # -Wpadded
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3")
else(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmul -mshift -fdata-sections -ffunction-sections")
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

add_definitions("-DUIP_CONF_UDP_CONNS=2")

if(NOT SIMULATION)
	set(CMAKE_EXE_LINKER_FLAGS "-T ${WAVENIS_LINKER_SCRIPT} -mmul -mshift -nostartfiles -Wl,--no-undefined -Wl,--noinhibit-exec -Wl,-Map,diaforus_final_code.map")
	link_directories("${CMAKE_SOURCE_DIR}/lib")
	link_directories("${CMAKE_SOURCE_DIR}/bsp/platform/mpw5")
	set(TARGET diaforus_final_code.jtag)
else(NOT SIMULATION)
	set(TARGET diaforus_final_code)
endif(NOT SIMULATION)

add_executable(${TARGET} ${diaforus_SOURCES} ${other_SOURCES})
target_link_libraries(${TARGET} ${CMAKE_THREAD_LIBS_INIT} m ${WAVENIS_LIB})

if(NOT SIMULATION)
	add_custom_command(	TARGET ${TARGET}
		POST_BUILD
		COMMAND aps3-objcopy --output-target=srec --srec-forceS3 --strip-debug ${CMAKE_BINARY_DIR}/diaforus_final_code.jtag ${CMAKE_BINARY_DIR}/diaforus_final_code.jtag.s3)
endif(NOT SIMULATION)

MESSAGE("")
MESSAGE("----- Summary -----")
MESSAGE("")
MESSAGE("Target Name                                   : ${TARGET} ")
MESSAGE("Install prefix(-DCMAKE_INSTALL_PREFIX=...)    : ${CMAKE_INSTALL_PREFIX} ")
MESSAGE("Build Type(-DCMAKE_BUILD_TYPE=(Release|Debug)): ${CMAKE_BUILD_TYPE}")
MESSAGE("C-Flags(-DCMAKE_C_FLAGS=...)                  : ${CMAKE_C_FLAGS} ")
MESSAGE("Application                                   : ${APPLICATION} ")
MESSAGE("")
MESSAGE("C Compiler (-DCMAKE_C_COMPILER=...)           : ${CMAKE_C_COMPILER} ")
MESSAGE("Cxx Compiler (-DCMAKE_CXX_COMPILER=...)       : ${CMAKE_CXX_COMPILER} ")
MESSAGE("Simulation                                    : ${SIMULATION} ")
if(NOT SIMULATION)
MESSAGE("Linker Script (-DWAVENIS_LINKER_SCRIPT=...)   : ${WAVENIS_LINKER_SCRIPT} ")
MESSAGE("Wavenis lib (-DWAVENIS_LIB=...)               : ${WAVENIS_LIB} ")
endif(NOT SIMULATION)
MESSAGE("")
MESSAGE("----- /Summary -----")
MESSAGE("")
