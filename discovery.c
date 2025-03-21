// SPDX-License-Identifier: MIT

/* TCG Discovery for SED/OPAL devices with sed-ioctl */

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/sed-opal.h>
#include "bitops.h"

struct level_0_discovery_header {
	uint32_t length;
	uint32_t revision;
	uint8_t reserved[8];
	uint8_t vendor_specific[32];
}  __attribute__ ((packed));

struct level_0_discovery_feature_shared {
	uint16_t feature_code;
	uint8_t reserved_minor : 4;
	uint8_t descriptor_version : 4;
	uint8_t length;
}  __attribute__ ((packed));

static void print_hex(const void *ptr, unsigned len)
{
	const char *buf = ptr;

	for (int i = 0; i < len; i++) {
		if (i && !(i % 16))
			printf("\n");
		printf("%02x", (unsigned char)buf[i]);
	}
	printf("\n");
}

static int status(int fd, bool debug)
{
	struct opal_status st;
	int r;

	r = ioctl(fd, IOC_OPAL_GET_STATUS, &st);
	if (r) {
		printf("IOC_OPAL_GET_STATUS failed (%d)\n", r);
		return EXIT_FAILURE;
	}

	printf("Opal status flags: %04x\n%s%s%s%s%s%s%s", st.flags,
		st.flags & OPAL_FL_SUPPORTED ? " OPAL_FL_SUPPORTED\n" : "",
		st.flags & OPAL_FL_LOCKING_SUPPORTED ? " OPAL_FL_LOCKING_SUPPORTED\n" : "",
		st.flags & OPAL_FL_LOCKING_ENABLED ? " OPAL_FL_LOCKING_ENABLED\n" : "",
		st.flags & OPAL_FL_LOCKED ? " OPAL_FL_LOCKED\n" : "",
		st.flags & OPAL_FL_MBR_ENABLED ? " OPAL_FL_MBR_ENABLED\n" : "",
		st.flags & OPAL_FL_MBR_DONE ? " OPAL_FL_MBR_DONE\n" : "",
		st.flags & OPAL_FL_SUM_SUPPORTED ? " OPAL_FL_SUM_SUPPORTED\n" : "");

	return EXIT_SUCCESS;
}

static int discovery(int fd, bool debug)
{
	int r, feat_length;
	struct opal_discovery discovery;
	struct level_0_discovery_header *dh;
	struct level_0_discovery_feature_shared *feat_hdr;
	char buf[4096];
	void *feat_ptr, *feat_end;

	discovery.data = (uintptr_t)buf;
	discovery.size = sizeof(buf);
	memset(buf, 0, sizeof(buf));

	r = ioctl(fd, IOC_OPAL_DISCOVERY, &discovery);
	/* This ioctl is misdesigned, as it returns data size... */
	if (r < 0) {
		printf("IOC_OPAL_DISCOVERY failed (%d)\n", r);
		return EXIT_FAILURE;
	}

	dh = (struct level_0_discovery_header *)buf;
	feat_ptr = buf + sizeof(*dh);
	feat_end = buf + be32_to_cpu(dh->length);

	/* Length not including length field itself [3.3.6 Core spec] */
	printf("Discovery0 [len %zu]\n", be32_to_cpu(dh->length) + sizeof(dh->length));
	if (debug)
		print_hex(dh, sizeof(*dh));

	while (feat_ptr < feat_end) {
		feat_hdr = feat_ptr;
		/* Length defines data following the header [3.3.6 Core spec] */
		feat_length = feat_hdr->length + sizeof(*feat_hdr);
		printf("Feature 0x%03x [len %d] ", be16_to_cpu(feat_hdr->feature_code), feat_length);

		switch (be16_to_cpu(feat_hdr->feature_code)) {
		/*   0x0000: reserved */
		case 0x0001: printf("TPer\n"); break;
		case 0x0002: printf("Locking\n"); break;
		case 0x0003: printf("Geometry\n"); break;
		case 0x0004: printf("Secure messaging\n"); break;
		case 0x0005: printf("SIIS\n"); break;
		/*   0x00ff: reserved */
		/*   0x0100 - 0x03ff: SSCs */
		case 0x0100: printf("Enterprise SSC\n"); break;
		case 0x0200: printf("Opal1 SSC\n"); break;
		case 0x0201: printf("SUM\n"); break;
		case 0x0202: printf("Data store\n"); break;
		case 0x0203: printf("Opal2\n"); break;
		case 0x0301: printf("Opalite SSC\n"); break;
		case 0x0302: printf("Pyrite1 SSC\n"); break;
		case 0x0303: printf("Pyrite2 SSC\n"); break;
		case 0x0304: printf("Ruby SSC\n"); break;
		case 0x0305: printf("KPIO\n"); break;
		/*   0x0400 - 0xbfff reserved */
		case 0x0401: printf("Locking LBA ranges\n"); break;
		case 0x0402: printf("Block SID\n"); break;
		case 0x0403: printf("NS locking\n"); break;
		case 0x0404: printf("Data removal\n"); break;
		case 0x0405: printf("NS geometry\n"); break;
		case 0x0407: printf("NS Shadow MBR\n"); break;
		case 0x0409: printf("CPIN\n"); break;
		case 0x040a: printf("NS KPIO\n"); break;
		/*   0xC000 - 0xffff vendor specific */
		default: printf("(unknown)\n"); break;
		}

		if (debug)
			print_hex(feat_ptr, feat_length);
		feat_ptr += feat_length;
	}

	return 0;
}

int main (int argc, char *argv[])
{
	bool debug = false;
	int fd, r;

	if (argc < 2) {
		printf("use <device> as parameter\n");
		return EXIT_FAILURE;
	}

	if (argc > 2 && !strcmp(argv[2], "--debug"))
		debug = true;

	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		printf("open fail\n");
		return EXIT_FAILURE;
	}

	r = status(fd, debug);

	/* Not all kernels support discovery */
	if (r == EXIT_SUCCESS)
		(void)discovery(fd, debug);

	close(fd);
	return r;
}
