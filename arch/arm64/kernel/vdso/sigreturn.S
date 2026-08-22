/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/linkage.h>
#include <asm/unistd.h>

	.text

	nop
ENTRY(__kernel_rt_sigreturn)
	.inst	0xd2801168	/* mov x8, #__NR_rt_sigreturn (139) */
	.inst	0xd4000001	/* svc #0 */
ENDPROC(__kernel_rt_sigreturn)
