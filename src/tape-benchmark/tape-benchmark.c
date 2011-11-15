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
*  Copyright (C) 2011, Clercin guillaume <gclercin@intellique.com>          *
*  Last modified: Tue, 15 Nov 2011 11:37:51 +0100                           *
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

#define DEFAULT_BUFFER_SIZE 16777216L
#define DEFAULT_DEVICE "/dev/nst0"
#define MIN_BUFFER_SIZE 4096L
#define MAX_BUFFER_SIZE 262144L

int check_size(ssize_t size) {
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

void convert_size(char * buffer, unsigned int bufferSize, ssize_t size) {
    unsigned short mult = 0;
    double tsize = size;

    while (tsize >= 1024 && mult < 4) {
        tsize /= 1024;
        mult++;
    }

    switch (mult) {
        case 0:
            snprintf(buffer, bufferSize, "%zd Bytes", size);
            break;
        case 1:
            snprintf(buffer, bufferSize, "%.1f KBytes", tsize);
            break;
        case 2:
            snprintf(buffer, bufferSize, "%.2f MBytes", tsize);
            break;
        case 3:
            snprintf(buffer, bufferSize, "%.3f GBytes", tsize);
            break;
        default:
            snprintf(buffer, bufferSize, "%.4f TBytes", tsize);
    }

    if (strchr(buffer, '.')) {
        char * ptrEnd = strchr(buffer, ' ');
        char * ptrBegin = ptrEnd - 1;
        while (*ptrBegin == '0')
            ptrBegin--;
        if (*ptrBegin == '.')
            ptrBegin--;

        if (ptrBegin + 1 < ptrEnd)
            memmove(ptrBegin + 1, ptrEnd, strlen(ptrEnd) + 1);
    }
}

ssize_t parse_size(const char * size) {
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

            return 1 << lsize;
        }
    }

    return 0;
}

int main(int argc, char ** argv) {
    char * buffer[32];
    char buffer_size[16];
    char * device = DEFAULT_DEVICE;
    ssize_t size = DEFAULT_BUFFER_SIZE;

    ssize_t max_buffer_size = MAX_BUFFER_SIZE;
    ssize_t min_buffer_size = MIN_BUFFER_SIZE;
    ssize_t tmp_size = 0;

    enum {
        OPT_DEVICE     = 'd',
        OPT_HELP       = 'h',
        OPT_MAX_BUFFER = 'M',
        OPT_MIN_BUFFER = 'm',
        OPT_SIZE       = 's',
    };

    static struct option op[] = {
        {"device",          1, 0, OPT_DEVICE},
        {"help",            0, 0, OPT_HELP},
        {"max-buffer-size", 1, 0, OPT_MAX_BUFFER},
        {"min-buffer-size", 1, 0, OPT_MIN_BUFFER},
        {"size",            1, 0, OPT_SIZE},

        {0, 0, 0, 0},
    };

    static int lo;
    for (;;) {
        int c = getopt_long(argc, argv, "d:hm:M:s:?", op, &lo);

        if (c == -1)
            break;

        switch (c) {
            case OPT_DEVICE:
                device = optarg;
                break;

            case OPT_HELP:
            case '?':
                printf("tape-benchmark (version: %s)\n", TAPEBENCHMARK_VERSION);
                printf("  -d, --device=DEV           : use this device DEV instead of \"%s\"\n", DEFAULT_DEVICE);
                printf("  -h, --help                 : show this and quit\n");
                convert_size(buffer_size, 16, MAX_BUFFER_SIZE);
                printf("  -M, --max-buffer-size=SIZE : maximum buffer size (instead of %s)\n", buffer_size);
                convert_size(buffer_size, 16, MIN_BUFFER_SIZE);
                printf("  -m, --min-buffer-size=SIZE : minimum buffer size (instead of %s)\n", buffer_size);
                convert_size(buffer_size, 16, DEFAULT_BUFFER_SIZE);
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

        struct timeval start;
        gettimeofday(&start, 0);
        struct tm * tv = localtime(&start.tv_sec);
        char buf[32];
        strftime(buf, 32, "%F %T", tv);
        ssize_t nb_loop = size / write_size;
        convert_size(buffer_size, 16, write_size);
        printf("Starting at %s, nb loop: %zd, block size: %s\n", buf, nb_loop, buffer_size);

        int write_error = 0;
        for (i = 0; i < nb_loop; i++) {
            ssize_t nb_write = write(fd_tape, buffer[i & 0x1F], write_size);
            if (nb_write < 0) {
                switch (errno) {
                    case EINVAL:
                        convert_size(buffer_size, 16, write_size >> 1);
                        printf("It seems that you cannot use a block size greater than %s\n", buffer_size);
                        break;

                    case EBUSY:
                        printf("Device is busy, so we wait a few seconds before restarting\n");
                        sleep(8);

                        gettimeofday(&start, 0);
                        strftime(buf, 32, "%F %T", tv);
                        printf("Restarting at %s, nb loop: %zd, block size: %s\n", buf, nb_loop, buffer_size);
                        i = -1;
                        break;

                    default:
                        printf("Oops: an error occured => (%d) %m\n", errno);
                        printf("fd: %d, buffer: %p, count: %zd\n", fd_tape, buffer[i & 0x1F], write_size);
                        break;
                }
                write_error = 1;
                break;
            }
        }

        struct timeval end;
        gettimeofday(&end, 0);
        tv = localtime(&end.tv_sec);
        strftime(buf, 32, "%F %T", tv);

        double time_spent = difftime(end.tv_sec, start.tv_sec) + difftime(end.tv_usec, start.tv_usec) / 1000000;
        double speed = i * write_size;
        speed /= time_spent;

        convert_size(buffer_size, 16, speed);
        printf("Finished at %s, current speed %s\n", buf, buffer_size);

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

