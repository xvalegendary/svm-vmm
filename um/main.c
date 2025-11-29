#include <Windows.h>
#include <intrin.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hypercall.h"

static uint64_t safe_vmcall(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    __try {
        return hv_vmcall(code, arg1, arg2, arg3);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("[!] vmmcall 0x%llx faulted with 0x%08X\n", code, GetExceptionCode());
        return 0;
    }
}

static void print_vendor_string(void) {
    int cpu_info[4] = {0};
    __cpuid(cpu_info, 0);

    char vendor[13] = {0};
    memcpy(vendor + 0, &cpu_info[1], 4); // ebx
    memcpy(vendor + 4, &cpu_info[3], 4); // edx
    memcpy(vendor + 8, &cpu_info[2], 4); // ecx

    printf("[+] cpuid vendor     : %s\n", vendor);
    printf("[+] cpuid max leaf   : 0x%x\n", cpu_info[0]);
}

static void dump_process_bases(void) {
    const uint32_t system_pid = 4;

    uint64_t current_base = safe_vmcall(hv_vmcall_query_current_process_base, 0, 0, 0);
    uint64_t system_base = safe_vmcall(hv_vmcall_query_process_base, system_pid, 0, 0);
    uint64_t system_cr3 = safe_vmcall(hv_vmcall_query_process_dirbase, system_pid, 0, 0);

    printf("[+] current image base : 0x%016llx\n", current_base);
    printf("[+] ntoskrnl.exe base  : 0x%016llx\n", system_base);
    printf("[+] system process cr3 : 0x%016llx\n", system_cr3);
}

static void dump_address_translations(void) {
    HMODULE self = GetModuleHandleW(NULL);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");

    uint64_t self_base = (uint64_t)self;
    uint64_t ntdll_base = (uint64_t)ntdll;

    uint64_t self_hpa = safe_vmcall(hv_vmcall_translate_gva_to_hpa, self_base, 0, 0);
    uint64_t ntdll_hpa = safe_vmcall(hv_vmcall_translate_gva_to_hpa, ntdll_base, 0, 0);

    printf("[+] image base gva -> hpa : 0x%016llx -> 0x%016llx\n", self_base, self_hpa);
    printf("[+] ntdll    gva -> hpa   : 0x%016llx -> 0x%016llx\n", ntdll_base, ntdll_hpa);
}

static void probe_mailbox_state(void) {
    uint64_t last_mailbox = safe_vmcall(hv_vmcall_last_mailbox, 0, 0, 0);
    uint64_t stealth = safe_vmcall(hv_vmcall_stealth_enable, 0, 0, 0);

    printf("[+] last mailbox token    : 0x%016llx\n", last_mailbox);
    printf("[+] stealth enable result : 0x%016llx\n", stealth);

    if (stealth != 0) {
        uint64_t disabled = safe_vmcall(hv_vmcall_stealth_disable, 0, 0, 0);
        printf("[+] stealth disable result: 0x%016llx\n", disabled);
    }
}

static void print_menu_hint(void) {
    puts("-------------------------------------------");
    puts("[~] svm playground usermode client");
    puts("[~] calls the hypervisor through vmmcall");
    puts("-------------------------------------------");
    puts("");
}

int main(void) {
    SetConsoleTitleA("svm hypercall playground [~]");

    print_menu_hint();
    printf("[+] make sure the svm driver is loaded first.\n\n");

    print_vendor_string();
    dump_process_bases();
    dump_address_translations();
    probe_mailbox_state();

    printf("\n[+] done.\n");
    return 0;
}

