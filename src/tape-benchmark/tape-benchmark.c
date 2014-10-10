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
*  Last modified: Fri, 10 Oct 2014 23:26:58 +0200                           *
\***************************************************************************/

// errno
#include <errno.h>
// open
#include <fcntl.h>
// getopt_long
#include <getopt.h>
// bindtextdomain, gettext, textdomain
#include <libintl.h>
// setlocale
#include <locale.h>
// poll
#include <poll.h>
// fflush, printf
#include <stdio.h>
// free, malloc
#include <stdlib.h>
// memset
#include <string.h>
// ioctl
#include <sys/ioctl.h>
// GMT_WR_PROT, MTIOCGET, MT_ST_BLKSIZE_MASK, MT_ST_BLKSIZE_SHIFT
#include <sys/mtio.h>
// open
#include <sys/stat.h>
// gettimeofday
#include <sys/time.h>
// open
#include <sys/types.h>
// clock_gettime, difftime
#include <time.h>
// close, read, sleep, write
#include <unistd.h>

#include "scsi.h"
#include "util.h"

#include "tape-benchmark.version"
#include "tape-benchmark.chcksum"

#define DEFAULT_SIZE 1073741824L
#define DEFAULT_DEVICE "/dev/nst0"
#define MIN_BUFFER_SIZE 16384L
#define MAX_BUFFER_SIZE 262144L

static bool rewind_tape(int fd);


int main(int argc, char ** argv) {
	static char * buffer[32];
	static char buffer_size[16];
	const char * device = DEFAULT_DEVICE;
	ssize_t size = DEFAULT_SIZE;
	bool no_rewind = false;
	bool rewind = false;

	setlocale(LC_ALL, "");
	bindtextdomain("tape-benchmark", "/usr/share/locale/");
	textdomain("tape-benchmark");

	ssize_t max_buffer_size = MAX_BUFFER_SIZE;
	ssize_t min_buffer_size = MIN_BUFFER_SIZE;
	ssize_t tmp_size = 0;

	enum {
		OPT_DEVICE     = 'd',
		OPT_HELP       = 'h',
		OPT_MAX_BUFFER = 'M',
		OPT_MIN_BUFFER = 'm',
		OPT_NO_REWIND  = 'r',
		OPT_REWIND     = 'R',
		OPT_SIZE       = 's',
		OPT_VERSION    = 'V',
	};

	static struct option op[] = {
		{ "device",          1, 0, OPT_DEVICE },
		{ "help",            0, 0, OPT_HELP },
		{ "max-buffer-size", 1, 0, OPT_MAX_BUFFER },
		{ "min-buffer-size", 1, 0, OPT_MIN_BUFFER },
		{ "no-rewind",       0, 0, OPT_NO_REWIND },
		{ "size",            1, 0, OPT_SIZE },
		{ "rewind-at-start", 0, 0, OPT_REWIND },
		{ "version",         0, 0, OPT_VERSION },

		{ 0, 0, 0, 0 },
	};

	static int lo;
	for (;;) {
		int c = getopt_long(argc, argv, "d:hm:M:s:rRV?", op, &lo);

		if (c == -1)
			break;

		switch (c) {
			case OPT_DEVICE:
				device = optarg;
				break;

			case OPT_HELP:
			case '?':
				printf("tape-benchmark (" TAPEBENCHMARK_VERSION ")\n");
				printf(gettext("  -d, --device=DEV           : use this device DEV instead of \"%s\"\n"), DEFAULT_DEVICE);
				printf(gettext("  -h, --help                 : show this and quit\n"));
				tb_convert_size(buffer_size, 16, MAX_BUFFER_SIZE);
				printf(gettext("  -M, --max-buffer-size=SIZE : maximum block size (instead of %s)\n"), buffer_size);
				tb_convert_size(buffer_size, 16, MIN_BUFFER_SIZE);
				printf(gettext("  -m, --min-buffer-size=SIZE : minimum block size (instead of %s)\n"), buffer_size);
				tb_convert_size(buffer_size, 16, DEFAULT_SIZE);
				printf(gettext("  -s, --size=SIZE            : size of file (default: %s)\n"), buffer_size);
				printf(gettext("  -r, --no-rewind            : no rewind tape between step (default: rewind between step)\n"));
				printf(gettext("  -R, --rewind-at-start      : rewind tape before writing on tape, (default: no rewind at start)\n\n"));

				printf(gettext("SIZE can be specified with (BKGT)\n"));
				printf(gettext("  1B => 1 byte, 1K => 1024B, 1M => 1024K, 1G => 1024M, 1T => 1024G\n"));
				printf(gettext("Another way to set the size is by specifying an integer which will be interpreted as a power of two.\n"));
				printf(gettext("  10 => 2^10 bytes (= 1K), 16 => 2^16 bytes (= 64K), 24 => 2^24 bytes (= 16M), and so on\n"));
				printf(gettext("Constraint: min-buffer-size and max-buffer-size should be a power of two\n\n"));

				printf(gettext("Notice: this programme will allocate 32 buffers of max-buffer-size\n"));

				return 0;

			case OPT_MAX_BUFFER:
				tmp_size = tb_parse_size(optarg);
				if (tmp_size == -1) {
					printf(gettext("Error: invalid block size for max-buffer-size\n"));
					return 1;
				} else if (tmp_size > 0 && tb_check_size(tmp_size)) {
					max_buffer_size = tmp_size;
				} else {
					printf(gettext("Error: max-buffer-size should be positive and a power of two\n"));
					return 1;
				}
				break;

			case OPT_MIN_BUFFER:
				tmp_size = tb_parse_size(optarg);
				if (tmp_size == -1) {
					printf(gettext("Error: invalid block size for min-buffer-size\n"));
					return 1;
				} else if (tmp_size > 0 && tb_check_size(tmp_size)) {
					min_buffer_size = tmp_size;
				} else {
					printf(gettext("Error: min-buffer-size should be positive and a power of two\n"));
					return 1;
				}
				break;

			case OPT_NO_REWIND:
				no_rewind = true;
				break;

			case OPT_SIZE:
				tmp_size = tb_parse_size(optarg);
				if (tmp_size == -1) {
					printf(gettext("Error: invalid file size\n"));
					return 1;
				} else if (tmp_size > 0) {
					size = tmp_size;
				} else {
					printf(gettext("Error: size should be positive\n"));
					return 1;
				}
				break;

			case OPT_REWIND:
				rewind = true;
				break;

			case OPT_VERSION:
				printf(gettext("tape-benchmark\n"));
				printf(gettext("  version: %s\n"), TAPEBENCHMARK_VERSION);
				printf(gettext("  build: %s\n"), __DATE__ " " __TIME__);
				printf(gettext("  based on git commit: %s\n"), TAPEBENCHMARK_GIT_COMMIT);
				printf(gettext("  checksum of source code: %s\n"), TAPEBENCHMARK_SRCSUM);
				return 0;
		}
	}

	tb_print_time();
	tb_print_flush(gettext("Openning \"%s\"... "), device);
	int fd_tape = open(device, O_RDONLY);
	if (fd_tape < 0) {
		printf(gettext("failed!!!, because %m\n"));
		return 2;
	}

	struct mtget mt;
	int failed = ioctl(fd_tape, MTIOCGET, &mt);
	if (failed != 0) {
		close(fd_tape);
		printf(gettext("Oops: seem not to be a valid tape device\n"));
		return 2;
	}

	if (GMT_WR_PROT(mt.mt_gstat)) {
		close(fd_tape);
		printf(gettext("Oops: Tape has its write lock enabled\n"));
		return 2;
	}

	failed = close(fd_tape);

	fd_tape = open(device, O_WRONLY);
	if (fd_tape < 0) {
		printf(gettext("failed!!!, because %m\n"));
		return 2;
	} else {
		printf(gettext("fd: %d\n"), fd_tape);
	}

	if (rewind && !rewind_tape(fd_tape))
		return 2;

	char * scsi_file = tb_scsi_lookup_scsi_file(device);
	if (scsi_file != NULL) {
		int scsi_fd = open(scsi_file, O_RDONLY);
		if (scsi_fd < 0) {
			printf(gettext("Failed to open scsi device because %m\n"));
		} else {
			static struct tb_scsi_inquery info;
			failed = tb_scsi_do_inquery(scsi_fd, &info);
			close(scsi_fd);

			printf(gettext("Tape vendor: %s\n"), info.vendor);
			printf(gettext("Model: %s\n"), info.model);
			printf(gettext("Revision: %s\n"), info.revision);
		}

		free(scsi_file);
	}

	ssize_t current_block_size = (mt.mt_dsreg & MT_ST_BLKSIZE_MASK) >> MT_ST_BLKSIZE_SHIFT;

	tb_print_time();
	tb_print_flush(gettext("Generate random data from \"/dev/urandom\"... "));
	int fd_ran = open("/dev/urandom", O_RDONLY);
	if (fd_ran < 0) {
		printf(gettext("Failed to open because %m\n"));
		close(fd_tape);
		return 2;
	}

	int j;
	for (j = 0; j < 32; j++) {
		buffer[j] = malloc(max_buffer_size);
		if (buffer[j] == NULL) {
			printf(gettext("Error: failed to allocate memory (size: %zd) because %m\n"), max_buffer_size);
			close(fd_tape);
			close(fd_ran);
			return 3;
		}

		ssize_t nb_read = read(fd_ran, buffer[j], max_buffer_size);
		if (nb_read < 0)
			printf(gettext("\nWarning: failed to read from \"/dev/urandom\" because %m\n"));
		else if (nb_read < max_buffer_size)
			printf(gettext("\nWarning: read less than expected, %zd instead of %zd\n"), nb_read, max_buffer_size);
	}
	close(fd_ran);
	printf(gettext("done\n"));

	static char clean_line[64];
	memset(clean_line, ' ', 64);

	ssize_t write_size;
	for (write_size = min_buffer_size; write_size <= max_buffer_size; write_size <<= 1) {
		if (current_block_size > 0) {
			write_size = current_block_size;
			printf(gettext("Warning: block size is defined to %zd instead of %zd\n"), current_block_size, write_size);
		}

		struct pollfd plfd = { fd_tape, POLLOUT, 0 };

		int pll_rslt = poll(&plfd, 1, 100);
		int poll_retry = 0;

		while (pll_rslt < 1) {
			if (poll_retry == 0)
				printf(gettext("Device is no ready, so we wait until"));
			else
				printf(".");
			fflush(stdout);
			poll_retry++;

			pll_rslt = poll(&plfd, 1, 6000);

			if (pll_rslt > 0)
				printf("\n");
		}

		struct timeval time_start;
		gettimeofday(&time_start, NULL);

		ssize_t nb_loop = size / write_size;
		tb_convert_size(buffer_size, 16, write_size);

		tb_print_time();
		printf(gettext("Starting, nb loop: %zd, block size: %s\n"), nb_loop, buffer_size);

		struct timespec start, last, current;
		clock_gettime(CLOCK_MONOTONIC, &start);
		last = start;

		int write_error = 0;
		long long int i;
		static int last_width = 64;
		for (i = 0; i < nb_loop; i++) {
			ssize_t nb_write = write(fd_tape, buffer[i & 0x1F], write_size);
			if (nb_write < 0) {
				if (last_width > 0)
					printf("\r%*s\r", last_width, clean_line);

				switch (errno) {
					case EINVAL:
						tb_convert_size(buffer_size, 16, write_size >> 1);
						printf(gettext("It seems that you cannot use a block size greater than %s\n"), buffer_size);
						break;

					case EBUSY:
						printf(gettext("Device is busy, so we wait a few seconds before restarting\n"));
						sleep(8);

						tb_print_time();
						printf(gettext("Restarting, nb loop: %zd, block size: %s\n"), nb_loop, buffer_size);
						i = -1;

						clock_gettime(CLOCK_MONOTONIC, &start);
						break;

					default:
						printf(gettext("Oops: an error occured => (%d) %m\n"), errno);
						printf(gettext("fd: %d, buffer: %p, count: %zd\n"), fd_tape, buffer[i & 0x1F], write_size);
						break;
				}
				write_error = 1;
				break;
			}

			clock_gettime(CLOCK_MONOTONIC, &current);

			if (last.tv_sec + 5 <= current.tv_sec) {
				float pct = 100 * i;
				double time_spent = difftime(current.tv_sec, start.tv_sec);
				double speed = i * write_size;
				speed /= time_spent;
				tb_convert_size(buffer_size, 16, speed);

				printf("\r%*s\r", last_width, clean_line);
				printf(gettext("loop: %lld, current speed %s, done: %.2f%%%n"), i, buffer_size, pct / nb_loop, &last_width);
				fflush(stdout);

				last = current;
			}
		}

		printf("\r%*s\r", last_width, clean_line);

		struct timeval end;
		gettimeofday(&end, 0);

		clock_gettime(CLOCK_MONOTONIC, &current);

		double time_spent = difftime(current.tv_sec, start.tv_sec);
		double speed = i * write_size;
		speed /= time_spent;
		tb_convert_size(buffer_size, 16, speed);

		tb_print_time();
		printf(gettext("Finished, current speed %s\n"), buffer_size);

		struct mtget mt2;
		failed = ioctl(fd_tape, MTIOCGET, &mt2);
		if (failed != 0) {
			printf(gettext("MTIOCGET failed => %m\n"));
			break;
		}

		struct mtop eof = { MTWEOF, 1 };
		failed = ioctl(fd_tape, MTIOCTOP, &eof);
		if (failed != 0) {
			printf(gettext("Weof failed => %m\n"));
			break;
		}

		struct mtop nop = { MTNOP, 1 };
		failed = ioctl(fd_tape, MTIOCTOP, &nop);
		if (failed != 0) {
			printf(gettext("Nop failed => %m\n"));
			break;
		}

		failed = ioctl(fd_tape, MTIOCGET, &mt2);
		if (failed != 0) {
			printf(gettext("MTIOCGET failed => %m\n"));
			break;
		}

		if (!no_rewind) {
			if (mt.mt_fileno < 2) {
				rewind_tape(fd_tape);
			} else {
				tb_print_time();
				tb_print_flush(gettext("Moving backward space 1 file... "));

				static struct mtop rewind = { MTBSFM, 2 };
				failed = ioctl(fd_tape, MTIOCTOP, &rewind);
				if (failed != 0)
					printf(gettext("Failed => %m\n"));
				else
					printf(gettext("done\n"));
			}
		}

		failed = ioctl(fd_tape, MTIOCGET, &mt2);
		if (failed)
			printf(gettext("MTIOCGET failed => %m\n"));

		if (current_block_size > 0 || write_error)
			break;
	}

	close(fd_tape);

	for (j = 0; j < 32; j++)
		free(buffer[j]);

	return 0;
}

/**
 * \brief rewind tape
 * \param fd[in]: valid file descriptor
 * \return true if success
 */
static bool rewind_tape(int fd) {
	static struct mtop rewind = { MTREW, 1 };

	tb_print_flush(gettext("Rewinding tape... "));

	int failed = ioctl(fd, MTIOCTOP, &rewind);

	if (failed != 0)
		printf(gettext("failed => %m\n"));
	else
		printf(gettext(", done\n"));

	return failed == 0;
}

