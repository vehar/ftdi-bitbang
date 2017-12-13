/*
 * ftdi-bitbang
 *
 * Common routines for all command line utilies.
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <libftdi1/ftdi.h>
#include "cmd-common.h"

/* usb vid */
uint16_t usb_vid = 0;
/* usb pid */
uint16_t usb_pid = 0;
/* usb description */
const char *usb_description = NULL;
/* usb serial */
const char *usb_serial = NULL;
/* interface (defaults to first one) */
int interface = INTERFACE_ANY;
/* reset flag, reset usb device if this is set */
int reset = 0;


void common_help(int argc, char *argv[])
{
	printf(
	    "\n"
	    "Usage:\n"
	    " %s [options]\n"
	    "\n"
	    "Definitions for options:\n"
	    " ID = hexadecimal word\n"
	    " PIN = decimal between 0 and 15\n"
	    " INTERFACE = integer between 1 and 4 depending on device type\n"
	    "\n"
	    "Options:\n"
	    "  -h, --help                 display this help and exit\n"
	    "  -V, --vid=ID               usb vendor id\n"
	    "  -P, --pid=ID               usb product id\n"
	    "                             as default vid and pid are zero, so any first compatible ftdi device is used\n"
	    "  -D, --description=STRING   usb description (product) to use for opening right device, default none\n"
	    "  -S, --serial=STRING        usb serial to use for opening right device, default none\n"
	    "  -I, --interface=INTERFACE  ftx232 interface number, defaults to first\n"
	    "  -R, --reset                do usb reset on the device at start\n"
	    "\n"
	    , basename(argv[0]));
	p_help();
}

int common_options(int argc, char *argv[], const char opts[], struct option longopts[])
{
	int err = 0;
	int longindex = 0, c;
	int i;

	while ((c = getopt_long(argc, argv, opts, longopts, &longindex)) > -1) {
		/* check for command specific options */
		if (p_options(c, optarg)) {
			continue;
		}
		/* check for common options */
		switch (c) {
		case 'V':
			i = (int)strtol(optarg, NULL, 16);
			if (errno == ERANGE || i < 0 || i > 0xffff) {
				fprintf(stderr, "invalid usb vid value\n");
				p_exit(1);
			}
			usb_vid = (uint16_t)i;
			break;
		case 'P':
			i = (int)strtol(optarg, NULL, 16);
			if (errno == ERANGE || i < 0 || i > 0xffff) {
				fprintf(stderr, "invalid usb pid value\n");
				p_exit(1);
			}
			usb_pid = (uint16_t)i;
			break;
		case 'D':
			usb_description = strdup(optarg);
			break;
		case 'S':
			usb_serial = strdup(optarg);
			break;
		case 'I':
			interface = atoi(optarg);
			if (interface < 0 || interface > 4) {
				fprintf(stderr, "invalid interface\n");
				p_exit(1);
			}
			break;
		case 'R':
			reset = 1;
			break;
		default:
		case '?':
		case 'h':
			common_help(argc, argv);
			p_exit(1);
		}
	}

	return 0;
}

struct ftdi_context *common_ftdi_init()
{
	int err = 0;
	struct ftdi_context *ftdi = NULL;

	/* initialize ftdi */
	ftdi = ftdi_new();
	if (!ftdi) {
		fprintf(stderr, "ftdi_new() failed\n");
		return NULL;
	}
	err = ftdi_set_interface(ftdi, interface);
	if (err < 0) {
		fprintf(stderr, "unable to set selected interface on ftdi device: %d (%s)\n", err, ftdi_get_error_string(ftdi));
		return NULL;
	}
	/* find first device if vid or pid is zero */
	if (usb_vid == 0 && usb_pid == 0) {
		struct ftdi_device_list *list;
		if (ftdi_usb_find_all(ftdi, &list, usb_vid, usb_pid) < 1) {
			fprintf(stderr, "unable to find any matching device\n");
			return NULL;
		}
		err = ftdi_usb_open_dev(ftdi, list->dev);
		ftdi_list_free(&list);
		if (err < 0) {
			fprintf(stderr, "unable to open ftdi device: %s\n", ftdi_get_error_string(ftdi));
			return NULL;
		}
	} else {
		err = ftdi_usb_open_desc(ftdi, usb_vid, usb_pid, usb_description, usb_serial);
		if (err < 0) {
			fprintf(stderr, "unable to open ftdi device: %s\n", ftdi_get_error_string(ftdi));
			return NULL;
		}
	}
	/* reset chip */
	if (reset) {
		if (ftdi_usb_reset(ftdi)) {
			fprintf(stderr, "failed to reset device: %s\n", ftdi_get_error_string(ftdi));
			p_exit(EXIT_FAILURE);
		}
	}

	return ftdi;
}
