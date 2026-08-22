/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/types.h>
#include <linux/time.h>
#include <linux/errno.h>
#include <asm/vdso_datapage.h>
#include <asm/unistd.h>
#include <asm/barrier.h>

#define VDSO_CLOCK_COARSE_RES	4000000

extern struct vdso_data _vdso_data;

static notrace __always_inline const struct vdso_data *get_vdso_data(void)
{
	return &_vdso_data;
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

static notrace __always_inline int do_realtime(const struct vdso_data *vd,
					       struct timespec *ts)
{
	u32 seq;
	do {
		seq = vdso_read_begin(vd);
		if (READ_ONCE(vd->use_syscall))
			return -1;
		ts->tv_sec  = vd->xtime_clock_sec;
		ts->tv_nsec = vd->xtime_clock_nsec;
	} while (vdso_read_retry(vd, seq));
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

static notrace __always_inline long
clock_gettime_fallback(clockid_t clock, struct timespec *ts)
{
	register long ret    __asm__("x0");
	register long _clk   __asm__("x0") = (long)clock;
	register struct timespec *_ts __asm__("x1") = ts;
	register long _nr    __asm__("x8") = __NR_clock_gettime;

	__asm__ volatile("svc #0"
		: "=r" (ret)
		: "0" (_clk), "r" (_ts), "r" (_nr)
		: "memory");
	return ret;
}

static notrace __always_inline long
gettimeofday_fallback(struct timeval *tv, struct timezone *tz)
{
	register long ret    __asm__("x0");
	register struct timeval *_tv __asm__("x0") = tv;
	register struct timezone *_tz __asm__("x1") = tz;
	register long _nr    __asm__("x8") = __NR_gettimeofday;

	__asm__ volatile("svc #0"
		: "=r" (ret)
		: "0" (_tv), "r" (_tz), "r" (_nr)
		: "memory");
	return ret;
}

static notrace __always_inline long
clock_getres_fallback(clockid_t clock_id, struct timespec *ts)
{
	register long ret     __asm__("x0");
	register long _clkid  __asm__("x0") = (long)clock_id;
	register struct timespec *_ts __asm__("x1") = ts;
	register long _nr     __asm__("x8") = __NR_clock_getres;

	__asm__ volatile("svc #0"
		: "=r" (ret)
		: "0" (_clkid), "r" (_ts), "r" (_nr)
		: "memory");
	return ret;
}

notrace int __kernel_clock_gettime(clockid_t clock, struct timespec *ts)
{
	const struct vdso_data *vd = get_vdso_data();

	switch (clock) {
	case CLOCK_REALTIME:
		if (!do_realtime(vd, ts))
			return 0;
		break;
	case CLOCK_REALTIME_COARSE:
	case CLOCK_MONOTONIC_COARSE:
		return do_coarse(vd, ts);
	default:
		break;
	}
	return clock_gettime_fallback(clock, ts);
}

notrace int __kernel_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	const struct vdso_data *vd = get_vdso_data();

	if (likely(tv != NULL)) {
		struct timespec ts;
		if (do_realtime(vd, &ts))
			return gettimeofday_fallback(tv, tz);
		tv->tv_sec  = ts.tv_sec;
		tv->tv_usec = ts.tv_nsec / 1000;
	}
	if (unlikely(tz != NULL)) {
		tz->tz_minuteswest = vd->tz_minuteswest;
		tz->tz_dsttime     = vd->tz_dsttime;
	}
	return 0;
}

notrace int __kernel_clock_getres(clockid_t clock_id, struct timespec *res)
{
	if (res == NULL)
		return 0;
	switch (clock_id) {
	case CLOCK_REALTIME:
	case CLOCK_MONOTONIC:
	case CLOCK_MONOTONIC_RAW:
		res->tv_sec  = 0;
		res->tv_nsec = get_vdso_data()->hrtimer_res;
		return 0;
	case CLOCK_REALTIME_COARSE:
	case CLOCK_MONOTONIC_COARSE:
		res->tv_sec  = 0;
		res->tv_nsec = VDSO_CLOCK_COARSE_RES;
		return 0;
	default:
		return clock_getres_fallback(clock_id, res);
	}
}
