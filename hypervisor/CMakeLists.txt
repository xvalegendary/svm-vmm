cmake_minimum_required(VERSION 3.27)

if(NOT MSVC)
    message(FATAL_ERROR "This project builds a Windows kernel driver and requires the MSVC toolchain.")
endif()

enable_language(ASM_MASM)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Infer the SDK/WDK layout from the generator settings.
if(NOT DEFINED CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
    message(FATAL_ERROR "A Windows 10 SDK is required. Set CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION when configuring.")
endif()

set(WDK_VERSION "${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
# The VS variable sometimes ends with a trailing dot. Strip it so the path resolves correctly.
if(WDK_VERSION MATCHES "\\.$")
    string(SUBSTRING "${WDK_VERSION}" 0 -1 WDK_VERSION)
endif()

set(WDK_ROOT "$ENV{WDKContentRoot}")
if(NOT WDK_ROOT)
    set(WDK_ROOT "$ENV{ProgramFiles(x86)}/Windows Kits/10")
endif()

set(WDK_INCLUDE_DIR "${WDK_ROOT}/Include/${WDK_VERSION}")
set(WDK_LIB_DIR "${WDK_ROOT}/Lib/${WDK_VERSION}")

if(NOT EXISTS "${WDK_INCLUDE_DIR}" OR NOT EXISTS "${WDK_LIB_DIR}")
    message(FATAL_ERROR "Could not find WDK/SDK headers and libraries under ${WDK_ROOT} for version ${WDK_VERSION}.")
endif()

# Choose the correct library architecture folder (x64 or ARM64).
set(DRIVER_ARCH "${CMAKE_VS_PLATFORM_NAME}")
if(NOT DRIVER_ARCH)
    set(DRIVER_ARCH "x64")
endif()
string(TOLOWER "${DRIVER_ARCH}" DRIVER_ARCH)

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

target_compile_definitions(hypervisor PRIVATE _AMD64_ _X64_ WIN64)

target_link_directories(hypervisor PRIVATE "${WDK_LIB_DIR}/km/${DRIVER_ARCH}")

target_link_libraries(hypervisor
    ntoskrnl.lib
    hal.lib
)

target_link_options(hypervisor PRIVATE
    /SUBSYSTEM:NATIVE
    /DRIVER
    /ENTRY:DriverEntry
    /NODEFAULTLIB
    /MANIFEST:NO
)
