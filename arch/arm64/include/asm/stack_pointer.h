/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_STACK_POINTER_H
#define __ASM_STACK_POINTER_H

#ifndef __clang__
register unsigned long current_stack_pointer asm ("sp");
#else
static __always_inline unsigned long current_stack_pointer(void)
{
	unsigned long sp;
	asm("mov %0, sp" : "=r"(sp));
	return sp;
}
#define current_stack_pointer current_stack_pointer()
#endif

#endif /* __ASM_STACK_POINTER_H */
