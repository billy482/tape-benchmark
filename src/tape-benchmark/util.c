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

// strlen
#include <string.h>

#include "util.h"

void tb_string_rtim(char * str, char trim) {
	size_t length = strlen(str);

	char * ptr;
	for (ptr = str + (length - 1); *ptr == trim && ptr > str; ptr--);

	if (ptr[1] != '\0')
		ptr[1] = '\0';
}

