/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/linkage.h>
#include <asm/unistd.h>

	.text

	nop
ENTRY(__kernel_rt_sigreturn)
	mov	x8, #__NR_rt_sigreturn
	svc	#0
ENDPROC(__kernel_rt_sigreturn)
