/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Userspace implementations of gettimeofday() and friends.
 * Pure C implementation for LLVM IAS compatibility.
 */
#include <linux/types.h>
#include <linux/time.h>
#include <linux/errno.h>
#include <asm/vdso_datapage.h>
#include <asm/unistd.h>

#define NSEC_PER_SEC	1000000000LL

static notrace __always_inline const struct vdso_data *get_vdso_data(void)
{
	return _vdso_data;
}

static notrace __always_inline u32 vdso_read_begin(const struct vdso_data *vd)
{
	u32 seq;

	while ((seq = READ_ONCE(vd->tb_seq_count)) & 1)
		cpu_relax();
	smp_rmb();
	return seq;
}

static notrace __always_inline int vdso_read_retry(const struct vdso_data *vd,
						    u32 start)
{
	smp_rmb();
	return READ_ONCE(vd->tb_seq_count) != start;
}

static notrace __always_inline u64 read_vdso_syscall_flag(
					const struct vdso_data *vd)
{
	return READ_ONCE(vd->use_syscall);
}

static notrace __always_inline int do_realtime(const struct vdso_data *vd,
					       struct timespec *ts)
{
	u64 nsec;
	u32 seq;

	do {
		seq = vdso_read_begin(vd);
		if (read_vdso_syscall_flag(vd))
			return -1;
		ts->tv_sec = vd->xtime_clock_sec;
		nsec = vd->xtime_clock_nsec;
	} while (vdso_read_retry(vd, seq));

	ts->tv_nsec = nsec;
	return 0;
}

static notrace __always_inline int do_coarse(const struct vdso_data *vd,
					     struct timespec *ts)
{
	u32 seq;

	do {
		seq = vdso_read_begin(vd);
		ts->tv_sec  = vd->xtime_coarse_sec;
		ts->tv_nsec = vd->xtime_coarse_nsec;
	} while (vdso_read_retry(vd, seq));

	return 0;
}

notrace int __kernel_clock_gettime(clockid_t clock,
				   struct timespec *ts)
{
	const struct vdso_data *vd = get_vdso_data();
	int ret;

	switch (clock) {
	case CLOCK_REALTIME:
		ret = do_realtime(vd, ts);
		break;
	case CLOCK_REALTIME_COARSE:
	case CLOCK_MONOTONIC_COARSE:
		ret = do_coarse(vd, ts);
		break;
	default:
		goto fallback;
	}

	if (ret)
		goto fallback;
	return 0;

fallback:
	return clock_gettime(clock, ts);
}

notrace int __kernel_gettimeofday(struct timeval *tv,
				  struct timezone *tz)
{
	const struct vdso_data *vd = get_vdso_data();

	if (likely(tv != NULL)) {
		struct timespec ts;
		if (do_realtime(vd, &ts))
			goto fallback;
		tv->tv_sec  = ts.tv_sec;
		tv->tv_usec = ts.tv_nsec / 1000;
	}

	if (unlikely(tz != NULL)) {
		tz->tz_minuteswest = vd->tz_minuteswest;
		tz->tz_dsttime     = vd->tz_dsttime;
	}

	return 0;

fallback:
	return gettimeofday(tv, tz);
}

notrace int __kernel_clock_getres(clockid_t clock_id,
				  struct timespec *res)
{
	const struct vdso_data *vd = get_vdso_data();

	if (res == NULL)
		return 0;

	switch (clock_id) {
	case CLOCK_REALTIME:
	case CLOCK_MONOTONIC:
	case CLOCK_MONOTONIC_RAW:
		res->tv_sec  = 0;
		res->tv_nsec = vd->hrtimer_res;
		return 0;
	case CLOCK_REALTIME_COARSE:
	case CLOCK_MONOTONIC_COARSE:
		res->tv_sec  = 0;
		res->tv_nsec = CLOCK_COARSE_RES;
		return 0;
	default:
		return clock_getres(clock_id, res);
	}
}
