# user-mode hypercall playground

expanded windows user-mode program that exercises the vmmcall interface
implemented by `HookVmmcallDispatch` in the svm hypervisor. the sample
queries kernel base addresses, directory table base values, and basic
address translations while keeping all naming lower-case for readability.

## building

1. open `SeCodeIntegrityQueryInformation.sln` in visual studio.
2. choose the x64 configuration.
3. build the solution. the hypervisor driver and the `um` console target
   will be produced side-by-side.

if you prefer the command line, run from a visual studio x64 native tools
prompt:

```cmd
msbuild SeCodeIntegrityQueryInformation.sln /p:Configuration=Release /p:Platform=x64
```

run the resulting `um_demo.exe` (from the `um` project) after the
hypervisor driver has been loaded so the `vmmcall` instruction is
intercepted.

## what it does

this demo issues several hypercalls:

- fetches the cpuid vendor string for a quick sanity check.
- queries the current process image base.
- queries the system (pid 4) process image base, which maps to
  `ntoskrnl.exe` in typical windows builds.
- queries the system process directory-table base / cr3 value.
- translates the image base of the current process and `ntdll.dll` from
  guest virtual address to host physical address.
- probes the mailbox and stealth toggles exposed by the hypervisor.

each request uses the same `hv_vmcall` entry point defined in
`hypercall.asm` to map the windows x64 calling convention to the register
layout the hypervisor expects.
