/***************************************************************************\
*       ______              ___               __                  __        *
*      /_  __/__ ____  ___ / _ )___ ___  ____/ /  __ _  ___ _____/ /__      *
*       / / / _ `/ _ \/ -_) _  / -_) _ \/ __/ _ \/  ' \/ _ `/ __/  '_/      *
*      /_/  \_,_/ .__/\__/____/\__/_//_/\__/_//_/_/_/_/\_,_/_/ /_/\_\       *
*              /_/                                                          *
*  -----------------------------------------------------------------------  *
*  This file is a part of TapeBenchmark                                     *
*                                                                           *
*  TapeBenchmark is free software; you can redistribute it and/or           *
*  modify it under the terms of the GNU General Public License              *
*  as published by the Free Software Foundation; either version 3           *
*  of the License, or (at your option) any later version.                   *
*                                                                           *
*  This program is distributed in the hope that it will be useful,          *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*  GNU General Public License for more details.                             *
*                                                                           *
*  You should have received a copy of the GNU General Public License        *
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  Copyright (C) 2014, Clercin guillaume <gclercin@intellique.com>          *
*  Last modified: Fri, 10 Oct 2014 22:32:11 +0200                           *
\***************************************************************************/

// gettext
#include <libintl.h>
// localeconv
#include <locale.h>
// va_end, va_start
#include <stdarg.h>
// sscanf, snprintf, vprintf
#include <stdio.h>
// strlen
#include <string.h>
// clock_gettime, localtime, strftime
#include <time.h>

#include "util.h"

bool tb_check_size(ssize_t size) {
	if (size < 0)
		return false;

	int i = 0;
	ssize_t tsize = size;

	while (tsize > 1) {
		tsize >>= 1;
		i++;
	}

	if (size != (tsize << i))
		return false;

	return true;
}

void tb_convert_size(char * str, unsigned int str_len, ssize_t size) {
	unsigned short mult = 0;
	double tsize = size;

	while (tsize >= 8192 && mult < 4) {
		tsize /= 1024;
		mult++;
	}

	int fixed = 0;
	if (tsize < 10)
		fixed = 2;
	else if (tsize < 100)
		fixed = 1;

	switch (mult) {
		case 0:
			snprintf(str, str_len, gettext("%zd Bytes"), size);
			break;
		case 1:
			snprintf(str, str_len, gettext("%.*f KBytes"), fixed, tsize);
			break;
		case 2:
			snprintf(str, str_len, gettext("%.*f MBytes"), fixed, tsize);
			break;
		case 3:
			snprintf(str, str_len, gettext("%.*f GBytes"), fixed, tsize);
			break;
		default:
			snprintf(str, str_len, gettext("%.*f TBytes"), fixed, tsize);
	}

	struct lconv * locale_info = localeconv();
	if (strstr(str, locale_info->decimal_point) != NULL) {
		char * ptrEnd = strchr(str, ' ');
		char * ptrBegin = ptrEnd - 1;
		while (*ptrBegin == '0')
			ptrBegin--;
		if (strncmp(ptrBegin, locale_info->decimal_point, strlen(locale_info->decimal_point)) == 0)
			ptrBegin--;

		if (ptrBegin + 1 < ptrEnd)
			memmove(ptrBegin + 1, ptrEnd, strlen(ptrEnd) + 1);
	}
}

void tb_string_rtim(char * str, char trim) {
	size_t length = strlen(str);

	char * ptr;
	for (ptr = str + (length - 1); *ptr == trim && ptr > str; ptr--);

	if (ptr[1] != '\0')
		ptr[1] = '\0';
}

ssize_t tb_parse_size(const char * size) {
	double dsize = 0;
	ssize_t lsize = 0;
	char mult = '\0';

	static const struct {
		char * pat;
		short nbElt;
	} pattern[] = {
		{ "%lg %c", 2 },
		{ "%lg%c",  2 },
		{ "%llu",   1 },

		{ 0, 0},
	};

	int i;
	for (i = 0; pattern[i].pat; i++) {
		if (pattern[i].nbElt == 2 && sscanf(size, pattern[i].pat, &dsize, &mult) == 2) {
			if (dsize < 0)
				return -1;
			if (mult == ' ')
				continue;

			switch (mult) {
				case 'k':
				case 'K':
					dsize *= 1024;
					break;

				case 'm':
				case 'M':
					dsize *= 1048576;
					break;

				case 'g':
				case 'G':
					dsize *= 1073741824;
					break;
			}
			return (unsigned long long) dsize;
		} else if (pattern[i].nbElt == 1 && sscanf(size, pattern[i].pat, &lsize) == 1) {
			if (mult == ' ')
				continue;

			return lsize > 0 ? 1L << lsize : -1;
		}
	}

	return -1;
}

void tb_print_flush(const char * format, ...) {
	va_list va;
	va_start(va, format);
	vprintf(format, va);
	va_end(va);

	fflush(stdout);
}

void tb_print_time() {
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);

	char buf_time[32];
	strftime(buf_time, 32, gettext("[ %T ] "), localtime(&now.tv_sec));

	printf("%s", buf_time);
}

