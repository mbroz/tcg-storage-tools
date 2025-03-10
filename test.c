// SPDX-License-Identifier: MIT

/* Various TCG functions / sed-ioctl  playground */

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


#define LR_NUM 2
#define USER_KEY_DEF {.lr=LR_NUM,.key_len=4,.key={'t','e','s','t'}}

static int setup_opal(int fd, bool debug)
{
	// IOC_OPAL_ACTIVATE_LSP
	struct opal_lr_act lrs = {
		.key = USER_KEY_DEF,
		.sum = 1,
		.num_lrs = 8,
		.lr = { 1,2,3,4,5,6,7,8 }
		//.num_lrs = 1,
		//.lr = { LR_NUM }
	};
	// IOC_OPAL_ACTIVATE_USR
	struct opal_session_info si = {
		.sum = 1,
		.who = OPAL_USER1,
		.opal_key = USER_KEY_DEF
	};
	// IOC_OPAL_SET_PW
	struct opal_new_pw pw = {
		.session = {
			.sum = 1,
			.who = OPAL_ADMIN1,
			.opal_key = {
				.lr = LR_NUM,
				.key_len = 0, /* SUM password is "" */
			}
		},
		.new_user_pw = {
			.sum = 1,
			.who = LR_NUM + 1,
			.opal_key = USER_KEY_DEF
		}
	};
	// IOC_OPAL_LR_SETUP
	struct opal_user_lr_setup user_lr_setup = {
		.range_start = 32768, /* 16MB with 512-bytes sector */
		.range_length = 2097152, /* 1G */
		.RLE = 1,
		.WLE = 1,
		.session = {
			.sum = 1,
			.who = 0,
			.opal_key = USER_KEY_DEF
		}
	};
	// IOC_OPAL_LOCK_UNLOCK
	struct opal_lock_unlock lock_unlock = {
		.l_state = OPAL_LK,
		.session = {
			.sum = 1,
			.who = 0,
			.opal_key = USER_KEY_DEF
		}
	};
	int r;

	r = ioctl(fd, IOC_OPAL_TAKE_OWNERSHIP, &lrs.key);
	if (r) {
		printf("IOC_OPAL_TAKE_OWNERSHIP failed (%d)\n", r);
		return EXIT_FAILURE;
	}
	printf("IOC_OPAL_TAKE_OWNERSHIP [OK]\n");

	r = ioctl(fd, IOC_OPAL_ACTIVATE_LSP, &lrs);
	if (r) {
		printf("IOC_OPAL_ACTIVATE_LSP failed (%d)\n", r);
		return EXIT_FAILURE;
	}
	printf("IOC_OPAL_ACTIVATE_LSP [OK]\n");

	r = ioctl(fd, IOC_OPAL_ACTIVATE_USR, &si);
	if (r) {
		printf("IOC_OPAL_ACTIVATE_USR failed (%d)\n", r);
		return EXIT_FAILURE;
	}
	printf("IOC_OPAL_ACTIVATE_USR [OK]\n");

	r = ioctl(fd, IOC_OPAL_SET_PW, &pw);
	if (r) {
		printf("IOC_OPAL_SET_PW failed (%d)\n", r);
		return EXIT_FAILURE;
	}
	printf("IOC_OPAL_SET_PW [OK]\n");

	r = ioctl(fd, IOC_OPAL_LR_SETUP, &user_lr_setup);
	if (r) {
		printf("IOC_OPAL_LR_SETUP failed (%d)\n", r);
		return EXIT_FAILURE;
	}
	printf("IOC_OPAL_LR_SETUP [OK]\n");

	r = ioctl(fd, IOC_OPAL_LOCK_UNLOCK, &lock_unlock);
	if (r) {
		printf("IOC_OPAL_LOCK_UNLOCK failed (%d)\n", r);
		return EXIT_FAILURE;
	}
	printf("IOC_OPAL_LOCK_UNLOCK [OK]\n");

	return EXIT_SUCCESS;
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

	if (r == EXIT_SUCCESS)
		r = setup_opal(fd, debug);

	close(fd);
	return r;
}
