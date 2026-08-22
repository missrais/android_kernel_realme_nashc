/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/linkage.h>

	.text

	nop
ENTRY(__kernel_rt_sigreturn)
	/* mov x8, #139 (__NR_rt_sigreturn) */
	.word	0xd2801168
	/* svc #0 */
	.word	0xd4000001
ENDPROC(__kernel_rt_sigreturn)
