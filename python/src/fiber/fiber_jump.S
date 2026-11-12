/* The jump_fcontext of boostorg/context for this platform (thirdparty/boost_context, docs/tasks/0457) */
#if defined(__x86_64__) && defined(__ELF__)
#include "jump_x86_64_sysv_elf_gas.S"
#elif defined(__x86_64__) && defined(__APPLE__)
#include "jump_x86_64_sysv_macho_gas.S"
#elif defined(__aarch64__) && defined(__ELF__)
#include "jump_arm64_aapcs_elf_gas.S"
#elif defined(__aarch64__) && defined(__APPLE__)
#include "jump_arm64_aapcs_macho_gas.S"
#else
#error "fiber.cmake builds the fcontext switch only for x86-64 and AArch64 on ELF and Mach-O"
#endif
