/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Sigreturn trampoline for returning from a signal when the SA_RESTORER
 * flag is not set.
 *
 * Copyright (C) 2012 ARM Limited
 */
#include <linux/linkage.h>
#include <asm/unistd.h>

	.text

	nop
ENTRY(__kernel_rt_sigreturn)
	.cfi_startproc
	.cfi_signal_frame
	.cfi_def_cfa	x29, 0
	.cfi_offset	x29, 0
	.cfi_offset	x30, 8
	mov	x8, #__NR_rt_sigreturn
	svc	#0
	.cfi_endproc
ENDPROC(__kernel_rt_sigreturn)
