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
*  Last modified: Sun, 28 Sep 2014 13:28:26 +0200                           *
\***************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mtio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "tape-benchmark.version"
#include "tape-benchmark.chcksum"

#define DEFAULT_SIZE 1073741824L
#define DEFAULT_DEVICE "/dev/nst0"
#define MIN_BUFFER_SIZE 16384L
#define MAX_BUFFER_SIZE 262144L

static int check_size(ssize_t size);
static void convert_size(char * str, unsigned int str_len, ssize_t size);
static ssize_t parse_size(const char * size);

static int check_size(ssize_t size) {
	int i = 0;
	ssize_t tsize = size;

	while (tsize > 1) {
		tsize >>= 1;
		i++;
	}

	if (size != (tsize << i))
		return 0;

	return 1;
}

static void convert_size(char * str, unsigned int str_len, ssize_t size) {
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
			snprintf(str, str_len, "%zd Bytes", size);
			break;
		case 1:
			snprintf(str, str_len, "%.*f KBytes", fixed, tsize);
			break;
		case 2:
			snprintf(str, str_len, "%.*f MBytes", fixed, tsize);
			break;
		case 3:
			snprintf(str, str_len, "%.*f GBytes", fixed, tsize);
			break;
		default:
			snprintf(str, str_len, "%.*f TBytes", fixed, tsize);
	}

	if (strchr(str, '.') != NULL) {
		char * ptrEnd = strchr(str, ' ');
		char * ptrBegin = ptrEnd - 1;
		while (*ptrBegin == '0')
			ptrBegin--;
		if (*ptrBegin == '.')
			ptrBegin--;

		if (ptrBegin + 1 < ptrEnd)
			memmove(ptrBegin + 1, ptrEnd, strlen(ptrEnd) + 1);
	}
}

static ssize_t parse_size(const char * size) {
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

			return 1L << lsize;
		}
	}

	return 0;
}

int main(int argc, char ** argv) {
	char * buffer[32];
	char buffer_size[16];
	char * device = DEFAULT_DEVICE;
	ssize_t size = DEFAULT_SIZE;

	ssize_t max_buffer_size = MAX_BUFFER_SIZE;
	ssize_t min_buffer_size = MIN_BUFFER_SIZE;
	ssize_t tmp_size = 0;

	enum {
		OPT_DEVICE     = 'd',
		OPT_HELP       = 'h',
		OPT_MAX_BUFFER = 'M',
		OPT_MIN_BUFFER = 'm',
		OPT_SIZE       = 's',
		OPT_VERSION    = 'V',
	};

	static struct option op[] = {
		{"device",          1, 0, OPT_DEVICE},
		{"help",            0, 0, OPT_HELP},
		{"max-buffer-size", 1, 0, OPT_MAX_BUFFER},
		{"min-buffer-size", 1, 0, OPT_MIN_BUFFER},
		{"size",            1, 0, OPT_SIZE},
		{"version",         0, 0, OPT_VERSION},

		{0, 0, 0, 0},
	};

	static int lo;
	for (;;) {
		int c = getopt_long(argc, argv, "d:hm:M:s:V?", op, &lo);

		if (c == -1)
			break;

		switch (c) {
			case OPT_DEVICE:
				device = optarg;
				break;

			case OPT_HELP:
			case '?':
				printf("tape-benchmark (%s)\n", TAPEBENCHMARK_VERSION);
				printf("  -d, --device=DEV           : use this device DEV instead of \"%s\"\n", DEFAULT_DEVICE);
				printf("  -h, --help                 : show this and quit\n");
				convert_size(buffer_size, 16, MAX_BUFFER_SIZE);
				printf("  -M, --max-buffer-size=SIZE : maximum buffer size (instead of %s)\n", buffer_size);
				convert_size(buffer_size, 16, MIN_BUFFER_SIZE);
				printf("  -m, --min-buffer-size=SIZE : minimum buffer size (instead of %s)\n", buffer_size);
				convert_size(buffer_size, 16, DEFAULT_SIZE);
				printf("  -s, --size=SIZE            : size of file (default: %s)\n\n", buffer_size);

				printf("SIZE can be specified with (BKGT)\n");
				printf("  1B => 1 byte, 1K => 1024B, 1M => 1024K, 1G => 1024M, 1T => 1024G\n");
				printf("Another way to set the size is by specifying an integer which will be interpreted as a power of two.\n");
				printf("  10 => 2^10 bytes (= 1K), 16 => 2^16 bytes (= 64K), 24 => 2^24 bytes (= 16M), and so on\n");
				printf("Constraint: min-buffer-size and max-buffer-size should be a power of two\n\n");

				printf("Note: this programme will allocate 32 buffers of max-buffer-size\n");

				return 0;

			case OPT_MAX_BUFFER:
				tmp_size = parse_size(optarg);
				if (check_size(tmp_size)) {
					max_buffer_size = tmp_size;
				} else {
					printf("Error: max-buffer-size should be a power of two\n");
					return 3;
				}
				break;

			case OPT_MIN_BUFFER:
				tmp_size = parse_size(optarg);
				if (check_size(tmp_size)) {
					min_buffer_size = tmp_size;
				} else {
					printf("Error: min-buffer-size should be a power of two\n");
					return 3;
				}
				break;

			case OPT_SIZE:
				size = parse_size(optarg);
				break;

			case OPT_VERSION:
				printf("tape-benchmark (%s, date and time : %s %s)\n", TAPEBENCHMARK_VERSION, __DATE__, __TIME__);
				printf("checksum of source code: %s\n", TAPEBENCHMARK_SRCSUM);
				printf("git commit: %s\n", TAPEBENCHMARK_GIT_COMMIT);
				return 0;
		}
	}

	printf("Openning \"%s\"", device);
	fflush(stdout);
	int fd_tape = open(device, O_WRONLY);
	if (fd_tape < 0) {
		printf(", failed!!!");
		return 1;
	} else {
		printf(", fd: %d\n", fd_tape);
	}

	struct mtget mt;
	int failed = ioctl(fd_tape, MTIOCGET, &mt);

	if (failed) {
		close(fd_tape);
		printf("Oops: \"%s\" seem to be a not valid tape device\n", device);
		return 2;
	}

	ssize_t current_block_size = (mt.mt_dsreg & MT_ST_BLKSIZE_MASK) >> MT_ST_BLKSIZE_SHIFT;

	printf("Generate random data from \"/dev/urandom\"");
	fflush(stdout);
	int fd_ran = open("/dev/urandom", O_RDONLY);
	long long int i;
	for (i = 0; i < 32; i++) {
		buffer[i] = malloc(max_buffer_size);
		read(fd_ran, buffer[i], max_buffer_size);
	}
	close(fd_ran);
	printf(", done\n");

	static char clean_line[64];
	memset(clean_line, ' ', 64);

	ssize_t write_size;
	for (write_size = min_buffer_size; write_size <= max_buffer_size; write_size <<= 1) {
		if (current_block_size > 0) {
			write_size = current_block_size;
			printf("Warning: block size is defined to %zd instead of %zd\n", current_block_size, write_size);
		}

		struct pollfd plfd = { fd_tape, POLLOUT, 0 };

		int pll_rslt = poll(&plfd, 1, 100);
		int poll_retry = 0;

		while (pll_rslt < 1) {
			if (poll_retry == 0)
				printf("Device is no ready, so we wait until");
			else
				printf(".");
			fflush(stdout);
			poll_retry++;

			pll_rslt = poll(&plfd, 1, 6000);

			if (pll_rslt > 0)
				printf("\n");
		}

		struct timeval time_start;
		gettimeofday(&time_start, 0);
		struct tm * tv = localtime(&time_start.tv_sec);
		char buf[32];
		strftime(buf, 32, "%F %T", tv);
		ssize_t nb_loop = size / write_size;
		convert_size(buffer_size, 16, write_size);
		printf("Starting at %s, nb loop: %zd, block size: %s\n", buf, nb_loop, buffer_size);

		struct timespec start, last, current;
		clock_gettime(CLOCK_MONOTONIC, &start);
		last = start;

		int write_error = 0;
		for (i = 0; i < nb_loop; i++) {
			ssize_t nb_write = write(fd_tape, buffer[i & 0x1F], write_size);
			if (nb_write < 0) {
				switch (errno) {
					case EINVAL:
						convert_size(buffer_size, 16, write_size >> 1);
						printf("\rIt seems that you cannot use a block size greater than %s\n", buffer_size);
						break;

					case EBUSY:
						printf("\rDevice is busy, so we wait a few seconds before restarting\n");
						sleep(8);

						gettimeofday(&time_start, 0);
						clock_gettime(CLOCK_MONOTONIC, &start);
						strftime(buf, 32, "%F %T", tv);
						printf("\rRestarting at %s, nb loop: %zd, block size: %s\n", buf, nb_loop, buffer_size);
						i = -1;
						break;

					default:
						printf("\rOops: an error occured => (%d) %m\n", errno);
						printf("fd: %d, buffer: %p, count: %zd\n", fd_tape, buffer[i & 0x1F], write_size);
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

				static int last_width = 64;
				printf("\r%*s\r", last_width, clean_line);

				convert_size(buffer_size, 16, speed);
				printf("loop: %lld, current speed %s, done: %.2f%%%n", i, buffer_size, pct / nb_loop, &last_width);
				fflush(stdout);
				clock_gettime(CLOCK_MONOTONIC, &last);
			}
		}

		struct timeval end;
		gettimeofday(&end, 0);
		tv = localtime(&end.tv_sec);
		strftime(buf, 32, "%F %T", tv);

		clock_gettime(CLOCK_MONOTONIC, &current);

		double time_spent = difftime(current.tv_sec, start.tv_sec);
		double speed = i * write_size;
		speed /= time_spent;

		convert_size(buffer_size, 16, speed);
		printf("\rFinished at %s, current speed %s\n", buf, buffer_size);

		struct mtget mt2;
		failed = ioctl(fd_tape, MTIOCGET, &mt2);
		if (failed)
			printf("MTIOCGET failed => %m\n");

		struct mtop eof = { MTWEOF, 1 };
		failed = ioctl(fd_tape, MTIOCTOP, &eof);
		if (failed)
			printf("Weof failed => %m\n");

		struct mtop nop = { MTNOP, 1 };
		failed = ioctl(fd_tape, MTIOCTOP, &nop);
		if (failed)
			printf("Nop failed => %m\n");

		failed = ioctl(fd_tape, MTIOCGET, &mt2);
		if (failed)
			printf("MTIOCGET failed => %m\n");


		struct mtop rewind = { MTBSFM, 2 };
		if (mt.mt_fileno < 2) {
			rewind.mt_op = MTREW;
			rewind.mt_count = 1;
			printf("Rewinding tape");
		} else {
			printf("Moving backward space 1 file");
		}
		fflush(stdout);

		failed = ioctl(fd_tape, MTIOCTOP, &rewind);
		if (failed)
			printf("Rewind failed => %m\n");
		else
			printf(", done\n");

		failed = ioctl(fd_tape, MTIOCGET, &mt2);
		if (failed)
			printf("MTIOCGET failed => %m\n");

		if (current_block_size > 0 || write_error)
			break;
	}

	close(fd_tape);

	return 0;
}

