/*
 * Copyright (c) 2023 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZTHULHU_USB_SAMPLE_USBD_H
#define ZTHULHU_USB_SAMPLE_USBD_H

#include <stdint.h>
#include <zephyr/usb/usbd.h>

/*
 * Local helper for Zthulhu's USB device setup using the next-gen USBD stack.
 * This is a local copy of Zephyr's sample helper with comments adjusted.
 */

/*
 * This function uses Kconfig SAMPLE_USBD_* options to configure and initialize
 * a USB device. It configures the device context, default string descriptors,
 * USB device configuration, registers any available class instances, and then
 * initializes the USB device. It is limited to a single device with a single
 * configuration instantiated in sample_usbd_init.c.
 *
 * Returns the configured and initialized USB device context on success,
 * otherwise NULL.
 */
struct usbd_context *sample_usbd_init_device(usbd_msg_cb_t msg_cb);

/*
 * This function is similar to sample_usbd_init_device(), but does not
 * initialize the device. It allows the application to set additional features,
 * such as additional descriptors.
 */
struct usbd_context *sample_usbd_setup_device(usbd_msg_cb_t msg_cb);

#endif /* ZTHULHU_USB_SAMPLE_USBD_H */
