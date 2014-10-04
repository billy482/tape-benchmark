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
*  Last modified: Sat, 04 Oct 2014 12:06:34 +0200                           *
\***************************************************************************/

// errno
#include <errno.h>
// open
#include <fcntl.h>
// getopt_long
#include <getopt.h>
// poll
#include <poll.h>
// va_end, va_start
#include <stdarg.h>
// bool
#include <stdbool.h>
// fflush, printf, sscanf, snprintf, vprintf
#include <stdio.h>
// malloc
#include <stdlib.h>
// memmove, memset, strlen, strrchr
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
// clock_gettime, difftime, localtime, strftime
#include <time.h>
// close, read, sleep, write
#include <unistd.h>

#include "tape-benchmark.version"
#include "tape-benchmark.chcksum"

#define DEFAULT_SIZE 1073741824L
#define DEFAULT_DEVICE "/dev/nst0"
#define MIN_BUFFER_SIZE 16384L
#define MAX_BUFFER_SIZE 262144L

static bool check_size(ssize_t size);
static void convert_size(char * str, unsigned int str_len, ssize_t size);
static ssize_t parse_size(const char * size);
static void print_flush(const char * format, ...) __attribute__((format(printf, 1, 2)));
static void print_time(void);
static bool rewind_tape(int fd);

/**
 * \brief Check if \a size is a power of two
 * \param size[in]: size in byte
 * \return \b true if \a size is a power of two
 */
static bool check_size(ssize_t size) {
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

/**
 * \brief Convert size into string
 * \param str[out]: write \a size into it
 * \param str_len[in]: length of \a str
 * \param size[in]: \a size to be converted into string
 */
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

int main(int argc, char ** argv) {
	static char * buffer[32];
	static char buffer_size[16];
	const char * device = DEFAULT_DEVICE;
	ssize_t size = DEFAULT_SIZE;
	bool no_rewind = false;
	bool rewind = false;

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
				printf("  -d, --device=DEV           : use this device DEV instead of \"" DEFAULT_DEVICE "\"\n");
				printf("  -h, --help                 : show this and quit\n");
				convert_size(buffer_size, 16, MAX_BUFFER_SIZE);
				printf("  -M, --max-buffer-size=SIZE : maximum buffer size (instead of %s)\n", buffer_size);
				convert_size(buffer_size, 16, MIN_BUFFER_SIZE);
				printf("  -m, --min-buffer-size=SIZE : minimum buffer size (instead of %s)\n", buffer_size);
				convert_size(buffer_size, 16, DEFAULT_SIZE);
				printf("  -s, --size=SIZE            : size of file (default: %s)\n", buffer_size);
				printf("  -r, --no-rewind            : no rewind tape between step (default: rewind between step)\n");
				printf("  -R, --rewind-at-start      : rewind tape before writing on tape, (default: no rewind at start)\n\n");

				printf("SIZE can be specified with (BKGT)\n");
				printf("  1B => 1 byte, 1K => 1024B, 1M => 1024K, 1G => 1024M, 1T => 1024G\n");
				printf("Another way to set the size is by specifying an integer which will be interpreted as a power of two.\n");
				printf("  10 => 2^10 bytes (= 1K), 16 => 2^16 bytes (= 64K), 24 => 2^24 bytes (= 16M), and so on\n");
				printf("Constraint: min-buffer-size and max-buffer-size should be a power of two\n\n");

				printf("Note: this programme will allocate 32 buffers of max-buffer-size\n");

				return 0;

			case OPT_MAX_BUFFER:
				tmp_size = parse_size(optarg);
				if (tmp_size == -1) {
					printf("Error: invalid size for max-buffer-size\n");
					return 1;
				} else if (tmp_size > 0 && check_size(tmp_size)) {
					max_buffer_size = tmp_size;
				} else {
					printf("Error: max-buffer-size should be positive and a power of two\n");
					return 1;
				}
				break;

			case OPT_MIN_BUFFER:
				tmp_size = parse_size(optarg);
				if (tmp_size == -1) {
					printf("Error: invalid size for min-buffer-size\n");
					return 1;
				} else if (tmp_size > 0 && check_size(tmp_size)) {
					min_buffer_size = tmp_size;
				} else {
					printf("Error: min-buffer-size should be positive and a power of two\n");
					return 1;
				}
				break;

			case OPT_NO_REWIND:
				no_rewind = true;
				break;

			case OPT_SIZE:
				tmp_size = parse_size(optarg);
				if (tmp_size == -1) {
					printf("Error: invalid size\n");
					return 1;
				} else if (tmp_size > 0) {
					size = tmp_size;
				} else {
					printf("Error: size should be positive\n");
					return 1;
				}
				break;

			case OPT_REWIND:
				rewind = true;
				break;

			case OPT_VERSION:
				printf("tape-benchmark (" TAPEBENCHMARK_VERSION ", date and time : " __DATE__ " " __TIME__ ")\n");
				printf("checksum of source code: " TAPEBENCHMARK_SRCSUM "\n");
				printf("git commit: " TAPEBENCHMARK_GIT_COMMIT "\n");
				return 0;
		}
	}

	print_time();
	print_flush("Openning \"%s\"... ", device);
	int fd_tape = open(device, O_RDONLY);
	if (fd_tape < 0) {
		printf("failed!!!, because %m\n");
		return 2;
	}

	struct mtget mt;
	int failed = ioctl(fd_tape, MTIOCGET, &mt);
	if (failed != 0) {
		close(fd_tape);
		printf("Oops: seem not to be a valid tape device\n");
		return 2;
	}

	if (GMT_WR_PROT(mt.mt_gstat)) {
		close(fd_tape);
		printf("Oops: Write lock enabled\n");
		return 2;
	}

	failed = close(fd_tape);

	fd_tape = open(device, O_WRONLY);
	if (fd_tape < 0) {
		printf("failed!!!, because %m\n");
		return 2;
	} else {
		printf("fd: %d\n", fd_tape);
	}

	if (rewind && !rewind_tape(fd_tape))
		return 2;

	ssize_t current_block_size = (mt.mt_dsreg & MT_ST_BLKSIZE_MASK) >> MT_ST_BLKSIZE_SHIFT;

	print_time();
	print_flush("Generate random data from \"/dev/urandom\"... ");
	int fd_ran = open("/dev/urandom", O_RDONLY);
	if (fd_ran < 0) {
		printf("Failed to open because %m\n");
		close(fd_tape);
		return 2;
	}

	int j;
	for (j = 0; j < 32; j++) {
		buffer[j] = malloc(max_buffer_size);
		if (buffer[j] == NULL) {
			printf("Error: failed to allocated memory (size: %zd) because %m\n", max_buffer_size);
			close(fd_tape);
			close(fd_ran);
			return 3;
		}

		ssize_t nb_read = read(fd_ran, buffer[j], max_buffer_size);
		if (nb_read < 0)
			printf("\nWarning: failed to read from \"/dev/urandom\" because %m\n");
		else if (nb_read < max_buffer_size)
			printf("\nWarning: read less than expected, %zd instead of %zd\n", nb_read, max_buffer_size);
	}
	close(fd_ran);
	printf("done\n");

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
		gettimeofday(&time_start, NULL);

		ssize_t nb_loop = size / write_size;
		convert_size(buffer_size, 16, write_size);

		print_time();
		printf("Starting, nb loop: %zd, block size: %s\n", nb_loop, buffer_size);

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
						convert_size(buffer_size, 16, write_size >> 1);
						printf("It seems that you cannot use a block size greater than %s\n", buffer_size);
						break;

					case EBUSY:
						printf("rDevice is busy, so we wait a few seconds before restarting\n");
						sleep(8);

						print_time();
						printf("Restarting, nb loop: %zd, block size: %s\n", nb_loop, buffer_size);
						i = -1;

						clock_gettime(CLOCK_MONOTONIC, &start);
						break;

					default:
						printf("Oops: an error occured => (%d) %m\n", errno);
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
				convert_size(buffer_size, 16, speed);

				printf("\r%*s\r", last_width, clean_line);
				printf("loop: %lld, current speed %s, done: %.2f%%%n", i, buffer_size, pct / nb_loop, &last_width);
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
		convert_size(buffer_size, 16, speed);

		print_time();
		printf("Finished, current speed %s\n", buffer_size);

		struct mtget mt2;
		failed = ioctl(fd_tape, MTIOCGET, &mt2);
		if (failed != 0) {
			printf("MTIOCGET failed => %m\n");
			break;
		}

		struct mtop eof = { MTWEOF, 1 };
		failed = ioctl(fd_tape, MTIOCTOP, &eof);
		if (failed != 0) {
			printf("Weof failed => %m\n");
			break;
		}

		struct mtop nop = { MTNOP, 1 };
		failed = ioctl(fd_tape, MTIOCTOP, &nop);
		if (failed != 0) {
			printf("Nop failed => %m\n");
			break;
		}

		failed = ioctl(fd_tape, MTIOCGET, &mt2);
		if (failed != 0) {
			printf("MTIOCGET failed => %m\n");
			break;
		}

		if (!no_rewind) {
			if (mt.mt_fileno < 2) {
				rewind_tape(fd_tape);
			} else {
				print_time();
				print_flush("Moving backward space 1 file... ");

				static struct mtop rewind = { MTBSFM, 2 };
				failed = ioctl(fd_tape, MTIOCTOP, &rewind);
				if (failed != 0)
					printf("Failed => %m\n");
				else
					printf("done\n");
			}
		}

		failed = ioctl(fd_tape, MTIOCGET, &mt2);
		if (failed)
			printf("MTIOCGET failed => %m\n");

		if (current_block_size > 0 || write_error)
			break;
	}

	close(fd_tape);

	for (j = 0; j < 32; j++)
		free(buffer[j]);

	return 0;
}

/**
 * \brief Convert size from string to integer
 * \param size[in]: string representing a size
 * \return \a size or \b -1 if failed
 */
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

/**
 * \brief print_flush is a shortcut of printf(format, ...) then fflush(stdout)
 * \param format[in]: printf compatible format
 */
static void print_flush(const char * format, ...) {
	va_list va;
	va_start(va, format);
	vprintf(format, va);
	va_end(va);

	fflush(stdout);
}

/**
 * \brief print current time to stdout
 */
static void print_time() {
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);

	char buf_time[32];
	strftime(buf_time, 32, "[ %T ] ", localtime(&now.tv_sec));

	printf("%s", buf_time);
}

/**
 * \brief rewind tape
 * \param fd[in]: valid file descriptor
 * \return true if success
 */
static bool rewind_tape(int fd) {
	static struct mtop rewind = { MTREW, 1 };

	print_flush("Rewinding tape... ");

	int failed = ioctl(fd, MTIOCTOP, &rewind);

	if (failed != 0)
		printf("Rewind failed => %m\n");
	else
		printf(", done\n");

	return failed == 0;
}

