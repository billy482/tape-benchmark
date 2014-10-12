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
*  Last modified: Sun, 12 Oct 2014 12:17:21 +0200                           *
\***************************************************************************/

#ifndef __TAPEBENCHMARK_SCSI_H__
#define __TAPEBENCHMARK_SCSI_H__

// bool
#include <stdbool.h>

struct tb_scsi_inquery {
	char vendor[9];
	char model[17];
	char revision[5];
	char serial_number[13];
};

struct tb_scsi_mam {
	char vendor[9];
	char serial_number[33];
	char manufactured_date[9];
	bool is_worm;
	unsigned long long int load_count;
};

int tb_scsi_do_inquery(int fd, struct tb_scsi_inquery * data);
int tb_scsi_do_read_mam(int fd, struct tb_scsi_mam * mam);
char * tb_scsi_lookup_scsi_file(const char * file);

#endif

