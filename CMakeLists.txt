cmake_minimum_required(VERSION 3.10)

# Variables regarding the AVR chip
set(MCU atmega328p)
set(F_CPU 16000000)
set(BAUD 9600)
set(PROG_TYPE arduino)

#AVR applications
set(AVRC avr-gcc)
set(AVRDUDE avrdude)
set(AVRSTRIP avr-strip)
set(OBJCOPY  avr-objcopy)

#Define compilers
set(CMAKE_C_COMPILER ${AVRC})

set(CMAKE_C_COMPILER  ${AVRC})
project(MinecraftCounter)

set(AVR_LIB_PATH "avr-libc-2.1.0")
set(DEVICE_NAME)

#Add source paths
set(SRC_PATH "source")
file(GLOB_RECURSE SRC_PATH_TARGET "${SRC_PATH}/*.c")

#Add include paths
set(INCLUDE_PATH "header")
set(AVR_LIB_INCLUDE_PATH "${AVR_LIB_PATH}/include")

#Compiler defines
add_definitions(-DF_CPU=${F_CPU} -DBAUD={$BAUD})

#Compiler flags
add_compile_options(-mmcu=atmega328p)
add_compile_options(-std=gnu99 -c -g -Os -Wall)
add_compile_options(-fno-exceptions -ffunction-sections -fdata-sections -fpack-struct)

#Linker flags
set(CMAKE_EXE_LINKER_FLAGS -mmcu=${MCU})

# Compiling targets
add_custom_target(strip ALL ${AVRSTRIP} "${PROJECT_NAME}.elf" DEPENDS ${PROJECT_NAME})
add_custom_target(hex ALL ${OBJCOPY} -R .eeprom -O ihex "${PROJECT_NAME}.elf" "${PROJECT_NAME}.hex" DEPENDS strip)
add_custom_target(eeprom ${OBJCOPY} -j .eeprom --change-section-lma .eeprom=0 -O ihex "${PROJECT_NAME}.elf" "${PROJECT_NAME}.eeprom" DEPENDS strip)
add_custom_target(flash ${AVRDUDE} -c ${PROG_TYPE} -p ${MCU} -P ${DEVICE_NAME} -U flash:w:${PROJECT_NAME}.hex DEPENDS hex)

#Project core options
include_directories(${INCLUDE_PATH} ${AVR_LIB_INCLUDE_PATH})
add_executable(${PROJECT_NAME} ${SRC_PATH_TARGET})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}.elf")