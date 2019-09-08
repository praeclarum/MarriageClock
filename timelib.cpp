#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "timelib.h"

#define SECS_PER_ERA   12622780800LL
#define SECS_PER_DAY   86400
#define DAYS_PER_YEAR    365
#define DAYS_PER_LYEAR   366
/* 400*365 days + 97 leap days */
#define DAYS_PER_LYEAR_PERIOD 146097
#define YEARS_PER_LYEAR_PERIOD 400

#define TIMELIB_ZONETYPE_OFFSET 1
#define TIMELIB_ZONETYPE_ABBR   2
#define TIMELIB_ZONETYPE_ID     3

typedef signed long long timelib_sll;

typedef struct _ttinfo ttinfo;
typedef struct _tlinfo tlinfo;

typedef struct _tlocinfo
{
  char country_code[3];
  double latitude;
  double longitude;
  char *comments;
} tlocinfo;

typedef struct _timelib_tzinfo
{
  char    *name;
  struct {
    uint32_t ttisgmtcnt;
    uint32_t ttisstdcnt;
    uint32_t leapcnt;
    uint32_t timecnt;
    uint32_t typecnt;
    uint32_t charcnt;
  } _bit32;
  struct {
    uint64_t ttisgmtcnt;
    uint64_t ttisstdcnt;
    uint64_t leapcnt;
    uint64_t timecnt;
    uint64_t typecnt;
    uint64_t charcnt;
  } bit64;

  int64_t *trans;
  unsigned char *trans_idx;

  ttinfo  *type;
  char    *timezone_abbr;

  tlinfo  *leap_times;
  unsigned char bc;
  tlocinfo location;
} timelib_tzinfo;

typedef struct _timelib_rel_time {
  timelib_sll y, m, d; /* Years, Months and Days */
  timelib_sll h, i, s; /* Hours, mInutes and Seconds */
  timelib_sll us;      /* Microseconds */

  int weekday; /* Stores the day in 'next monday' */
  int weekday_behavior; /* 0: the current day should *not* be counted when advancing forwards; 1: the current day *should* be counted */

  int first_last_day_of;
  int invert; /* Whether the difference should be inverted */
  timelib_sll days; /* Contains the number of *days*, instead of Y-M-D differences */

  struct {
    unsigned int type;
    timelib_sll amount;
  } special;

  unsigned int   have_weekday_relative, have_special_relative;
} timelib_rel_time;

typedef struct _timelib_time {
  timelib_sll      y, m, d;     /* Year, Month, Day */
  timelib_sll      h, i, s;     /* Hour, mInute, Second */
  timelib_sll      us;          /* Microseconds */
  int              z;           /* UTC offset in seconds */
  char            *tz_abbr;     /* Timezone abbreviation (display only) */
  timelib_tzinfo  *tz_info;     /* Timezone structure */
  signed int       dst;         /* Flag if we were parsing a DST zone */
  timelib_rel_time relative;

  timelib_sll      sse;         /* Seconds since epoch */

  unsigned int   have_time, have_date, have_zone, have_relative, have_weeknr_day;

  unsigned int   sse_uptodate; /* !0 if the sse member is up to date with the date/time members */
  unsigned int   tim_uptodate; /* !0 if the date/time members are up to date with the sse member */
  unsigned int   is_localtime; /*  1 if the current struct represents localtime, 0 if it is in GMT */
  unsigned int   zone_type;    /*  1 time offset,
                                *  3 TimeZone identifier,
                                *  2 TimeZone abbreviation */
} timelib_time;

typedef struct _timelib_time_offset {
  int32_t      offset;
  unsigned int leap_secs;
  unsigned int is_dst;
  char        *abbr;
  timelib_sll  transition_time;
} timelib_time_offset;

#define timelib_is_leap(y) ((y) % 4 == 0 && ((y) % 100 != 0 || (y) % 400 == 0))

/*                                    jan  feb  mrt  apr  may  jun  jul  aug  sep  oct  nov  dec */
static int month_tab_leap[12]     = {  -1,  30,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334 };
static int month_tab[12]          = {   0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334 };

/*                                    dec  jan  feb  mrt  apr  may  jun  jul  aug  sep  oct  nov  dec */
static int days_in_month_leap[13] = {  31,  31,  29,  31,  30,  31,  30,  31,  31,  30,  31,  30,  31 };
static int days_in_month[13]      = {  31,  31,  28,  31,  30,  31,  30,  31,  31,  30,  31,  30,  31 };

//static int month_tab_leap[12] = { -1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
//static int month_tab[12] =      {  0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

static void do_range_limit_fraction(timelib_sll *fraction, timelib_sll *seconds)
{
  if (*fraction < 0) {
    *fraction += 1000000;
    *seconds -= 1;
  }
  if (*fraction >= 1000000) {
    *fraction -= 1000000;
    *seconds += 1;
  }
}

static void do_range_limit(timelib_sll start, timelib_sll end, timelib_sll adj, timelib_sll *a, timelib_sll *b)
{
  if (*a < start) {
    *b -= (start - *a - 1) / adj + 1;
    *a += adj * ((start - *a - 1) / adj + 1);
  }
  if (*a >= end) {
    *b += *a / adj;
    *a -= adj * (*a / adj);
  }
}

static void inc_month(timelib_sll *y, timelib_sll *m)
{
  (*m)++;
  if (*m > 12) {
    *m -= 12;
    (*y)++;
  }
}

static void dec_month(timelib_sll *y, timelib_sll *m)
{
  (*m)--;
  if (*m < 1) {
    *m += 12;
    (*y)--;
  }
}

static void do_range_limit_days_relative(timelib_sll *base_y, timelib_sll *base_m, timelib_sll *y, timelib_sll *m, timelib_sll *d, timelib_sll invert)
{
  timelib_sll leapyear;
  timelib_sll month, year;
  timelib_sll days;

  do_range_limit(1, 13, 12, base_m, base_y);

  year = *base_y;
  month = *base_m;

/*
  printf( "S: Y%d M%d   %d %d %d   %d\n", year, month, *y, *m, *d, days);
*/
  if (!invert) {
    while (*d < 0) {
      dec_month(&year, &month);
      leapyear = timelib_is_leap(year);
      days = leapyear ? days_in_month_leap[month] : days_in_month[month];

      /* printf( "I  Y%d M%d   %d %d %d   %d\n", year, month, *y, *m, *d, days); */

      *d += days;
      (*m)--;
    }
  } else {
    while (*d < 0) {
      leapyear = timelib_is_leap(year);
      days = leapyear ? days_in_month_leap[month] : days_in_month[month];

      /* printf( "I  Y%d M%d   %d %d %d   %d\n", year, month, *y, *m, *d, days); */

      *d += days;
      (*m)--;
      inc_month(&year, &month);
    }
  }
  /*
  printf( "E: Y%d M%d   %d %d %d   %d\n", year, month, *y, *m, *d, days);
  */
}

static void timelib_do_rel_normalize(timelib_time *base, timelib_rel_time *rt)
{
  do_range_limit_fraction(&rt->us, &rt->s);
  do_range_limit(0, 60, 60, &rt->s, &rt->i);
  do_range_limit(0, 60, 60, &rt->i, &rt->h);
  do_range_limit(0, 24, 24, &rt->h, &rt->d);
  do_range_limit(0, 12, 12, &rt->m, &rt->y);

  do_range_limit_days_relative(&base->y, &base->m, &rt->y, &rt->m, &rt->d, rt->invert);
  do_range_limit(0, 12, 12, &rt->m, &rt->y);
}

timelib_rel_time* timelib_rel_time_ctor(void)
{
  timelib_rel_time *t;  
  t = (timelib_rel_time *)calloc(1, sizeof(timelib_rel_time));

  return t;
}

//static int month_tab_leap[12] = { -1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
//static int month_tab[12] =      { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };


/* Converts a Unix timestamp value into broken down time, in GMT */
void timelib_unixtime2gmt(timelib_time* tm, timelib_sll ts)
{
  timelib_sll days, remainder, tmp_days;
  timelib_sll cur_year = 1970;
  timelib_sll i;
  timelib_sll hours, minutes, seconds;
  int *months;

  days = ts / SECS_PER_DAY;
  remainder = ts - (days * SECS_PER_DAY);
  if (ts < 0 && remainder == 0) {
    days++;
    remainder -= SECS_PER_DAY;
  }
//  TIMELIB_DEBUG(printf("days=%lld, rem=%lld\n", days, remainder););

  if (ts >= 0) {
    tmp_days = days + 1;
  } else {
    tmp_days = days;
  }

  if (tmp_days > DAYS_PER_LYEAR_PERIOD || tmp_days <= -DAYS_PER_LYEAR_PERIOD) {
    cur_year += YEARS_PER_LYEAR_PERIOD * (tmp_days / DAYS_PER_LYEAR_PERIOD);
    tmp_days -= DAYS_PER_LYEAR_PERIOD * (tmp_days / DAYS_PER_LYEAR_PERIOD);
  }
//  TIMELIB_DEBUG(printf("tmp_days=%lld, year=%lld\n", tmp_days, cur_year););

  if (ts >= 0) {
    while (tmp_days >= DAYS_PER_LYEAR) {
      cur_year++;
      if (timelib_is_leap(cur_year)) {
        tmp_days -= DAYS_PER_LYEAR;
      } else {
        tmp_days -= DAYS_PER_YEAR;
      }
//      TIMELIB_DEBUG(printf("tmp_days=%lld, year=%lld\n", tmp_days, cur_year););
    }
  } else {
    while (tmp_days <= 0) {
      cur_year--;
      if (timelib_is_leap(cur_year)) {
        tmp_days += DAYS_PER_LYEAR;
      } else {
        tmp_days += DAYS_PER_YEAR;
      }
//      TIMELIB_DEBUG(printf("tmp_days=%lld, year=%lld\n", tmp_days, cur_year););
    }
    remainder += SECS_PER_DAY;
  }
//  TIMELIB_DEBUG(printf("tmp_days=%lld, year=%lld\n", tmp_days, cur_year););

  months = timelib_is_leap(cur_year) ? month_tab_leap : month_tab;
  if (timelib_is_leap(cur_year) && cur_year < 1970) {
    tmp_days--;
  }
  i = 11;
  while (i > 0) {
//    TIMELIB_DEBUG(printf("month=%lld (%d)\n", i, months[i]););
    if (tmp_days > months[i]) {
      break;
    }
    i--;
  }
//  TIMELIB_DEBUG(printf("A: ts=%lld, year=%lld, month=%lld, day=%lld,", ts, cur_year, i + 1, tmp_days - months[i]););

  /* That was the date, now we do the time */
  hours = remainder / 3600;
  minutes = (remainder - hours * 3600) / 60;
  seconds = remainder % 60;
//  TIMELIB_DEBUG(printf(" hour=%lld, minute=%lld, second=%lld\n", hours, minutes, seconds););

  tm->y = cur_year;
  tm->m = i + 1;
  tm->d = tmp_days - months[i];
  tm->h = hours;
  tm->i = minutes;
  tm->s = seconds;
  tm->z = 0;
  tm->dst = 0;
  tm->sse = ts;
  tm->sse_uptodate = 1;
  tm->tim_uptodate = 1;
  tm->is_localtime = 0;
}

int timelib_apply_localtime(timelib_time *t, unsigned int localtime)
{
  if (localtime) {
    return -1;
    /* Converting from GMT time to local time */
//    TIMELIB_DEBUG(printf("Converting from GMT time to local time\n"););

    /* Check if TZ is set */
//    if (!t->tz_info) {
//      TIMELIB_DEBUG(printf("E: No timezone configured, can't switch to local time\n"););
//      return -1;
//    }

//    timelib_unixtime2local(t, t->sse);
  } else {
    /* Converting from local time to GMT time */
//    TIMELIB_DEBUG(printf("Converting from local time to GMT time\n"););

    timelib_unixtime2gmt(t, t->sse);
  }
  return 0;
}

timelib_rel_time *timelib_diff(timelib_time *one, timelib_time *two)
{
	timelib_rel_time *rt;
	timelib_time *swp;
	timelib_sll dst_corr = 0 ,dst_h_corr = 0, dst_m_corr = 0;
	timelib_time one_backup, two_backup;

	rt = timelib_rel_time_ctor();
	rt->invert = 0;
	if (
		(one->sse > two->sse) ||
		(one->sse == two->sse && one->us > two->us)
	) {
		swp = two;
		two = one;
		one = swp;
		rt->invert = 1;
	}

	/* Calculate correction for DST change over, but only if the TZ type is ID
	 * and it's the same */
	if (one->zone_type == 3 && two->zone_type == 3
		&& (strcmp(one->tz_info->name, two->tz_info->name) == 0)
		&& (one->z != two->z))
	{
		dst_corr = two->z - one->z;
		dst_h_corr = dst_corr / 3600;
		dst_m_corr = (dst_corr % 3600) / 60;
	}

	/* Save old TZ info */
	memcpy(&one_backup, one, sizeof(one_backup));
	memcpy(&two_backup, two, sizeof(two_backup));

    timelib_apply_localtime(one, 0);
    timelib_apply_localtime(two, 0);

	rt->y = two->y - one->y;
	rt->m = two->m - one->m;
	rt->d = two->d - one->d;
	rt->h = two->h - one->h;
	rt->i = two->i - one->i;
	rt->s = two->s - one->s;
	rt->us = two->us - one->us;
	if (one_backup.dst == 0 && two_backup.dst == 1 && two->sse >= one->sse + 86400 - dst_corr) {
		rt->h += dst_h_corr;
		rt->i += dst_m_corr;
	}

	rt->days = fabs(floor((one->sse - two->sse - (dst_h_corr * 3600) - (dst_m_corr * 60)) / 86400));

	timelib_do_rel_normalize(rt->invert ? one : two, rt);

	/* We need to do this after normalisation otherwise we can't get "24H" */
	if (one_backup.dst == 1 && two_backup.dst == 0 && two->sse >= one->sse + 86400) {
		if (two->sse < one->sse + 86400 - dst_corr) {
			rt->d--;
			rt->h = 24;
		} else {
			rt->h += dst_h_corr;
			rt->i += dst_m_corr;
		}
	}

	/* Restore old TZ info */
	memcpy(one, &one_backup, sizeof(one_backup));
	memcpy(two, &two_backup, sizeof(two_backup));

	return rt;
}

void diff_times(signed long long startSeconds, signed long long endSeconds, reltime* rel)
{
  timelib_time startTime={0};
  timelib_time endTime={0};
  
  timelib_unixtime2gmt(&startTime, startSeconds);
  timelib_unixtime2gmt(&endTime, endSeconds);

  timelib_rel_time *r = timelib_diff(&startTime, &endTime);
  Serial.printf("r->y = %d\n", (int)r->y);
  Serial.printf("r->m = %d\n", (int)r->m);
  Serial.printf("r->d = %d\n", (int)r->d);
  Serial.printf("r->h = %d\n", (int)r->h);
  Serial.printf("r->i = %d\n", (int)r->i);
  Serial.printf("r->s = %d\n", (int)r->s);
  Serial.printf("r->invert = %d\n", (int)r->invert);
  rel->y = r->y;
  rel->m = r->m;
  rel->d = r->d;
  rel->h = r->h;
  rel->i = r->i;
  rel->s = r->s;
  rel->invert = r->invert;
  free(r);
}
