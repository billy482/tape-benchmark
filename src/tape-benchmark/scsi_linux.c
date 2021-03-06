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
*  Last modified: Sun, 12 Oct 2014 12:59:54 +0200                           *
\***************************************************************************/

#define _GNU_SOURCE
// be16toh, be32toh
#include <endian.h>
// glob, globfree
#include <glob.h>
// sg_io_hdr_t
#include <scsi/scsi.h>
// sg_io_hdr_t
#include <scsi/sg.h>
// bool
#include <stdbool.h>
// asprintf
#include <stdio.h>
// free
#include <stdlib.h>
// memset, strncpy, strrchr
#include <string.h>
// bzero
#include <strings.h>
// ioctl
#include <sys/ioctl.h>

#include "scsi.h"
#include "util.h"

struct scsi_inquiry {
	unsigned char operation_code;
	bool enable_vital_product_data:1;
	unsigned char reserved0:4;
	unsigned char obsolete:3;
	unsigned char page_code;
	unsigned char reserved1;
	unsigned char allocation_length;
	unsigned char reserved2;
} __attribute__((packed));

struct scsi_log_sense_request {
	unsigned char operation_code;
	bool saved_paged:1;
	bool parameter_pointer_control:1;
	unsigned char reserved0:3;
	unsigned char obsolete:3;
	unsigned char page_code:6;
	enum {
		page_control_max_value = 0x0,
		page_control_current_value = 0x1,
		page_control_max_value2 = 0x2, // same as page_control_max_value ?
		page_control_power_on_value = 0x3,
	} page_control:2;
	unsigned char reserved1[2];
	unsigned short parameter_pointer; // must be a bigger endian integer
	unsigned short allocation_length; // must be a bigger endian integer
	unsigned char control;
} __attribute__((packed));

enum scsi_mam_attribute {
	scsi_mam_remaining_capacity  = 0x0000,
	scsi_mam_maximum_capacity    = 0x0001,
	scsi_mam_load_count          = 0x0003,
	scsi_mam_mam_space_remaining = 0x0004,

	scsi_mam_device_at_last_load           = 0x020A,
	scsi_mam_device_at_last_load_2         = 0x020B,
	scsi_mam_device_at_last_load_3         = 0x020C,
	scsi_mam_device_at_last_load_4         = 0x020D,
	scsi_mam_total_written_in_medium_life  = 0x0220,
	scsi_mam_total_read_in_medium_life     = 0x0221,
	scsi_mam_total_written_in_current_load = 0x0222,
	scsi_mam_total_read_current_load       = 0x0223,

	scsi_mam_medium_manufacturer      = 0x0400,
	scsi_mam_medium_serial_number     = 0x0401,
	scsi_mam_medium_manufacturer_date = 0x0406,
	scsi_mam_mam_capacity             = 0x0407,
	scsi_mam_medium_type              = 0x0408,
	scsi_mam_medium_type_information  = 0x0409,

	scsi_mam_application_vendor           = 0x0800,
	scsi_mam_application_name             = 0x0801,
	scsi_mam_application_version          = 0x0802,
	scsi_mam_user_medium_text_label       = 0x0803,
	scsi_mam_date_and_time_last_written   = 0x0804,
	scsi_mam_text_localization_identifier = 0x0805,
	scsi_mam_barcode                      = 0x0806,
	scsi_mam_owning_host_textual_name     = 0x0807,
	scsi_mam_media_pool                   = 0x0808,

	scsi_mam_unique_cardtrige_identity = 0x1000,
};

struct scsi_request_sense {
	unsigned char error_code:7;						/* Byte 0 Bits 0-6 */
	bool valid:1;									/* Byte 0 Bit 7 */
	unsigned char segment_number;					/* Byte 1 */
	unsigned char sense_key:4;						/* Byte 2 Bits 0-3 */
	bool :1;										/* Byte 2 Bit 4 */
	bool ili:1;										/* Byte 2 Bit 5 */
	bool eom:1;										/* Byte 2 Bit 6 */
	bool filemark:1;								/* Byte 2 Bit 7 */
	unsigned char information[4];					/* Bytes 3-6 */
	unsigned char additional_sense_length;			/* Byte 7 */
	unsigned char command_specific_information[4];	/* Bytes 8-11 */
	unsigned char additional_sense_code;			/* Byte 12 */
	unsigned char additional_sense_code_qualifier;	/* Byte 13 */
	unsigned char :8;								/* Byte 14 */
	unsigned char bit_pointer:3;					/* Byte 15 */
	bool bpv:1;
	unsigned char :2;
	bool command_data:1;
	bool sksv:1;
	unsigned char field_data[2];					/* Byte 16,17 */
};


int tb_scsi_do_inquery(int fd, struct tb_scsi_inquery * data) {
	struct {
		unsigned char peripheral_device_type:5;
		unsigned char peripheral_device_qualifier:3;
		unsigned char reserved0:7;
		bool removable_medium_bit:1;
		unsigned char version:3;
		unsigned char ecma_version:3;
		unsigned char iso_version:2;
		unsigned char response_data_format:4;
		bool hi_sup:1;
		bool norm_aca:1;
		bool obsolete0:1;
		bool asynchronous_event_reporting_capability:1;
		unsigned char additional_length;
		unsigned char reserved1:7;
		bool scc_supported:1;
		bool addr16:1;
		bool addr32:1;
		bool obsolete1:1;
		bool medium_changer:1;
		bool multi_port:1;
		bool vs:1;
		bool enclosure_service:1;
		bool basic_queuing:1;
		bool reserved2:1;
		bool command_queuing:1;
		bool trans_dis:1;
		bool linked_command:1;
		bool synchonous_transfer:1;
		bool wide_bus_16:1;
		bool obsolete2:1;
		bool relative_addressing:1;
		char vendor_identification[8];
		char product_identification[16];
		char product_revision_level[4];
		bool aut_dis:1;
		unsigned char performance_limit;
		unsigned char reserved3[3];
		unsigned char oem_specific;
	} __attribute__((packed)) result_inquiry;

	struct scsi_inquiry command_inquiry = {
		.operation_code = 0x12, // INQUIRY
		.enable_vital_product_data = false,
		.page_code = 0,
		.allocation_length = sizeof(result_inquiry),
	};

	struct scsi_request_sense sense;
	sg_io_hdr_t header;
	memset(&header, 0, sizeof(header));
	memset(&sense, 0, sizeof(sense));
	memset(&result_inquiry, 0, sizeof(result_inquiry));

	header.interface_id = 'S';
	header.cmd_len = sizeof(command_inquiry);
	header.mx_sb_len = sizeof(sense);
	header.dxfer_len = sizeof(result_inquiry);
	header.cmdp = (unsigned char *) &command_inquiry;
	header.sbp = (unsigned char *) &sense;
	header.dxferp = (unsigned char *) &result_inquiry;
	header.timeout = 60000; // 1 minutes
	header.dxfer_direction = SG_DXFER_FROM_DEV;

	if (ioctl(fd, SG_IO, &header))
		return -1;

	strncpy(data->vendor, result_inquiry.vendor_identification, 8);
	data->vendor[8] = '\0';
	tb_string_rtim(data->vendor, ' ');

	strncpy(data->model, result_inquiry.product_identification, 16);
	data->model[16] = '\0';
	tb_string_rtim(data->model, ' ');

	strncpy(data->revision, result_inquiry.product_revision_level, 4);
	data->revision[4] = '\0';
	tb_string_rtim(data->revision, ' ');

	struct {
		unsigned char peripheral_device_type:5;
		unsigned char peripheral_device_qualifier:3;
		unsigned char page_code;
		unsigned char reserved;
		unsigned char page_length;
		char unit_serial_number[12];
	} __attribute__((packed)) result_serial_number;

	struct scsi_inquiry command_serial_number = {
		.operation_code = 0x12, // INQUIRY
		.enable_vital_product_data = true,
		.page_code = 0x80, // UNIT SERIAL NUMBER
		.allocation_length = sizeof(result_serial_number),
	};

	memset(&header, 0, sizeof(header));
	memset(&sense, 0, sizeof(sense));
	memset(&result_serial_number, 0, sizeof(result_serial_number));

	header.interface_id = 'S';
	header.cmd_len = sizeof(command_serial_number);
	header.mx_sb_len = sizeof(sense);
	header.dxfer_len = sizeof(result_serial_number);
	header.cmdp = (unsigned char *) &command_serial_number;
	header.sbp = (unsigned char *) &sense;
	header.dxferp = (unsigned char *) &result_serial_number;
	header.timeout = 60000; // 1 minutes
	header.dxfer_direction = SG_DXFER_FROM_DEV;

	if (ioctl(fd, SG_IO, &header))
		return -1;

	strncpy(data->serial_number, result_serial_number.unit_serial_number, 12);
	data->serial_number[12] = '\0';
	tb_string_rtim(data->serial_number, ' ');

	return 0;
}

int tb_scsi_do_read_mam(int fd, struct tb_scsi_mam * mam) {
	struct {
		unsigned char operation_code;
		enum {
			scsi_read_attribute_service_action_attributes_values = 0x00,
			scsi_read_attribute_service_action_attribute_list = 0x01,
			scsi_read_attribute_service_action_volume_list = 0x02,
			scsi_read_attribute_service_action_parition_list = 0x03,
		} service_action:5;
		unsigned char obsolete:3;
		unsigned char reserved0[3];
		unsigned char volume_number;
		unsigned char reserved1;
		unsigned char partition_number;
		unsigned short first_attribute_id;
		unsigned short allocation_length;
		unsigned char reserved2;
		unsigned char control;
	} __attribute__((packed)) command = {
		.operation_code = 0x8C, // READ ATTRIBUTE
		.service_action = scsi_read_attribute_service_action_attributes_values,
		.volume_number = 0,
		.partition_number = 0,
		.first_attribute_id = 0,
		.allocation_length = htobe16(1024),
		.control = 0,
	};

	struct scsi_request_sense sense;
	unsigned char buffer[1024];

	sg_io_hdr_t header;
	memset(&header, 0, sizeof(header));
	memset(&sense, 0, sizeof(sense));
	memset(buffer, 0, 1024);

	header.interface_id = 'S';
	header.cmd_len = sizeof(command);
	header.mx_sb_len = sizeof(sense);
	header.dxfer_len = sizeof(buffer);
	header.cmdp = (unsigned char *) &command;
	header.sbp = (unsigned char *) &sense;
	header.dxferp = buffer;
	header.timeout = 60000; // 1 minute
	header.dxfer_direction = SG_DXFER_FROM_DEV;

	int status = ioctl(fd, SG_IO, &header);
	if (status)
		return status;

	struct scsi_mam {
		enum scsi_mam_attribute attribute_identifier:16;
		unsigned char format:2;
		unsigned char reserved:5;
		bool read_only:1;
		unsigned short attribute_length;
		union {
			unsigned char int8;
			unsigned short be16;
			unsigned long long be64;
			char text[160];
		} attribute_value;
	} __attribute__((packed));

	unsigned int data_available = be32toh(*(unsigned int *) buffer);
	unsigned char * ptr = buffer + 4;

	for (ptr = buffer + 4; ptr < buffer + data_available;) {
		struct scsi_mam * attr = (struct scsi_mam *) ptr;
		attr->attribute_identifier = be16toh(attr->attribute_identifier);
		attr->attribute_length = be16toh(attr->attribute_length);

		ptr += attr->attribute_length + 5;

		if (attr->attribute_length == 0)
			continue;

		switch (attr->attribute_identifier) {
			case scsi_mam_load_count:
				mam->load_count = be64toh(attr->attribute_value.be64);
				break;

			case scsi_mam_medium_manufacturer:
				strncpy(mam->vendor, attr->attribute_value.text, 8);
				mam->vendor[8] = '\0';
				tb_string_rtim(mam->vendor, ' ');
				break;

			case scsi_mam_medium_serial_number:
				strncpy(mam->serial_number, attr->attribute_value.text, 32);
				mam->serial_number[32] = '\0';
				tb_string_rtim(mam->serial_number, ' ');
				break;

			case scsi_mam_medium_manufacturer_date:
				strncpy(mam->manufactured_date, attr->attribute_value.text, 8);
				mam->manufactured_date[8] = '\0';
				tb_string_rtim(mam->manufactured_date, ' ');
				break;

			case scsi_mam_medium_type:
				mam->is_worm = attr->attribute_value.int8 == 0x80;
				break;
		}
	}

	return 0;
}

char * tb_scsi_lookup_scsi_file(const char * file) {
	if (file == NULL)
		return NULL;

	char * dev = strrchr(file, '/');
	if (dev == NULL)
		return NULL;

	char * path;
	asprintf(&path, "/sys/class/scsi_tape/%s/device/scsi_generic/*", dev + 1);

	glob_t gl;
	bzero(&gl, sizeof(gl));
	int ret = glob(path, GLOB_NOSORT, NULL, &gl);
	if (ret != 0)
		goto exit;

	dev = strrchr(gl.gl_pathv[0], '/');
	if (dev == NULL)
		goto exit;

	globfree(&gl);
	free(path);

	asprintf(&path, "/dev%s", dev);
	return path;

exit:
	globfree(&gl);
	free(path);

	return NULL;
}

