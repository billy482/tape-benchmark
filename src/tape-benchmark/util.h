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

#ifndef __TAPEBENCHMARK_UTIL_H__
#define __TAPEBENCHMARK_UTIL_H__

// bool
#include <stdbool.h>
// ssize_t
#include <sys/types.h>

/**
 * \brief Check if \a size is a power of two
 * \param size[in]: size in byte
 * \return \b true if \a size is a power of two
 */
bool tb_check_size(ssize_t size);

/**
 * \brief Convert size into string
 * \param str[out]: write \a size into it
 * \param str_len[in]: length of \a str
 * \param size[in]: \a size to be converted into string
 */
void tb_convert_size(char * str, unsigned int str_len, ssize_t size);

/**
 * \brief Convert size from string to integer
 * \param size[in]: string representing a size
 * \return \a size or \b -1 if failed
 */
ssize_t tb_parse_size(const char * size);

/**
 * \brief print_flush is a shortcut of printf(format, ...) then fflush(stdout)
 * \param format[in]: printf compatible format
 */
void tb_print_flush(const char * format, ...) __attribute__((format(printf, 1, 2)));

/**
 * \brief print current time to stdout
 */
void tb_print_time(void);

void tb_string_rtim(char * string, char chr);

#endif

