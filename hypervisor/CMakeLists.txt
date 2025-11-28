cmake_minimum_required(VERSION 3.10)

project(hypervisor LANGUAGES C ASM_MASM)

if(NOT MSVC)
    message(FATAL_ERROR "MSVC compiler required.")
endif()

# -----------------------------------------
#   FIXED WINDOWS SDK PATH (YOUR SYSTEM)
# -----------------------------------------

# Your SDK version:
set(SDK_VERSION "10.0.26100.0")

# Your SDK root:
set(SDK_ROOT "C:/Program Files (x86)/Windows Kits/10")

set(WDK_INCLUDE_DIR "${SDK_ROOT}/Include/${SDK_VERSION}")
set(WDK_LIB_DIR     "${SDK_ROOT}/Lib/${SDK_VERSION}")

if(NOT EXISTS "${WDK_INCLUDE_DIR}/km")
    message(FATAL_ERROR "WDK headers not found in: ${WDK_INCLUDE_DIR}/km")
endif()

message(STATUS "Using SDK version: ${SDK_VERSION}")
message(STATUS "Using WDK include: ${WDK_INCLUDE_DIR}")
message(STATUS "Using WDK lib: ${WDK_LIB_DIR}")

# -----------------------------------------
#   ARCHITECTURE
# -----------------------------------------

if(CMAKE_VS_PLATFORM_NAME)
    set(DRIVER_ARCH "${CMAKE_VS_PLATFORM_NAME}")
else()
    set(DRIVER_ARCH "x64")
endif()
string(TOLOWER "${DRIVER_ARCH}" DRIVER_ARCH)

# -----------------------------------------
#   SOURCE FILES
# -----------------------------------------

set(SOURCES
    driver.c
    guest_mem.c
    hooks.c
    hypervisor.c
    layers.c
    npt.c
    shadow_idt.c
    smp.c
    stealth.c
    svm.c
    vmrun.asm
)

# -----------------------------------------
#   DRIVER BUILD (.sys)
# -----------------------------------------

add_library(hypervisor SHARED ${SOURCES})



set_target_properties(hypervisor PROPERTIES
    OUTPUT_NAME "hypervisor"
    SUFFIX ".sys"
)

target_include_directories(hypervisor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${WDK_INCLUDE_DIR}/km"
    "${WDK_INCLUDE_DIR}/shared"
)

target_compile_definitions(hypervisor PRIVATE
    _AMD64_ _X64_ WIN64
)

target_link_directories(hypervisor PRIVATE
    "${WDK_LIB_DIR}/km/${DRIVER_ARCH}"
)

target_link_libraries(hypervisor
    ntoskrnl.lib
    hal.lib
)

target_link_options(hypervisor PRIVATE
    /DRIVER
    /SUBSYSTEM:NATIVE
    /ENTRY:DriverEntry
    /NODEFAULTLIB
    /MANIFEST:NO
)
