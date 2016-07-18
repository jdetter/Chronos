/**
 * Author: John Detter <john@detter.com>
 *
 * Driver for the Real Time Clock.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "kstdlib.h"
#include "drivers/cmos.h"
#include "stdlock.h"
#include "drivers/rtc.h"
#include "panic.h"

void rtc_update(struct rtc_t* dst)
{
	/* Check for update */
	while(cmos_read(0x0A) & 0x80);
	
	uchar mode		= cmos_read(0x0B);
	uchar seconds 		= cmos_read(0x00);
	uchar minutes 		= cmos_read(0x02);
	uchar hours 		= cmos_read(0x04);
	//uchar weekday 		= cmos_read(0x06);
	uchar dayOfMonth 	= cmos_read(0x07);
	uchar month 		= cmos_read(0x08);
	uchar year 		= cmos_read(0x09);
	uchar century 		= cmos_read(0x32);

	uchar hour24 = mode & 0x02;
	uchar binary = mode & 0x04;
	
	if(!binary)
	{
		/* Convert all to binary */
		seconds = bcdtobin(seconds);
		minutes = bcdtobin(minutes);
		if(!hour24)
		{
			/* Do both conversions here */
			uchar pm = hours & 0x80;
			if(pm) hours &= ~(0x80);
			hours = bcdtobin(hours);
			if(pm) hours += 12;
			if(hours >= 24) hours %= 24;
		} else hours = bcdtobin(hours);
		dayOfMonth = bcdtobin(dayOfMonth);
		month = bcdtobin(month);
		year = bcdtobin(year);
		century = bcdtobin(century);
	} else if(!hour24)
	{
		/* Do both conversions here */
		uchar pm = hours & 0x80;
		if(pm) hours &= ~(0x80);
		hours = bcdtobin(hours);
		if(pm) hours += 12;
		/* 12am is midnight in 12hour rtc */
		if(!pm && hours == 12) hours = 0;
		if(hours >= 24) hours = 0;
	}

	dst->seconds = seconds;
	dst->minutes = minutes;
	dst->hours = hours;
	dst->day = dayOfMonth;
	dst->month = month;
	dst->year = year + 2000; /* Year only tracks last 2 digits */
}

void rtc_str(char* dst, size_t sz, struct rtc_t* r)
{
	char* pm = "am";
	uchar hours = r->hours;
	if(hours >= 12)
	{
		pm = "pm";
		hours -= 12;
	} else if(hours == 0)
	{
		hours = 12;
	}

	char* leading_min = "0";
	char* leading_hour = "0";
	char* leading_day = "0";
	char* leading_month = "0";

	if(r->minutes > 9)
		leading_min = "";
	leading_hour = ""; /* Always disable leading hour */
	if(r->day > 9)
		leading_day = "";
	if(r->month > 9)
		leading_month = "";

	snprintf(dst, sz, "%s%d:%s%d%s %s%d/%s%d/%d", 
		leading_hour, hours, 
		leading_min, r->minutes, pm,
		leading_month, r->month, 
		leading_day, r->day, 
		r->year);
}
