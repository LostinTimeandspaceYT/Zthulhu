#include <sample_usbd.h>

#include "file_manager.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbd.h>
#include <lvgl.h>

LOG_MODULE_REGISTER(zthulhu, LOG_LEVEL_DBG);

#define UI_TICK_MS 10

static const struct device *const console_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
static struct usbd_context *sample_usbd;
static K_SEM_DEFINE(dtr_sem, 0, 1);

static void usb_msg_cb(struct usbd_context *const ctx, const struct usbd_msg *msg)
{

	/* Handle VBUS detection if supported */
	if (usbd_can_detect_vbus(ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			if (usbd_enable(ctx)) {
				LOG_ERR("Failed to enable USB device");
			}
		}
		if (msg->type == USBD_MSG_VBUS_REMOVED) {
			if (usbd_disable(ctx)) {
				LOG_ERR("Failed to disable USB device");
			}
		}
	}

	/* Handle DTR signal for console connection detection */
	if (msg->type == USBD_MSG_CDC_ACM_CONTROL_LINE_STATE) {
		uint32_t dtr = 0;
		uart_line_ctrl_get(msg->dev, UART_LINE_CTRL_DTR, &dtr);
		if (dtr) {
			LOG_INF("USB console connected");
			k_sem_give(&dtr_sem);
		}
	}
}

static int enable_usb_device(void)
{
	int err;

	sample_usbd = sample_usbd_init_device(usb_msg_cb);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	/* If VBUS detection is not available, enable USB immediately */
	if (!usbd_can_detect_vbus(sample_usbd)) {
		err = usbd_enable(sample_usbd);
		if (err) {
			LOG_ERR("Failed to enable USB device");
			return err;
		}
	}

	LOG_INF("USB device support enabled");
	return 0;
}

int main(void)
{
	const struct device *display_dev;
	int ret;

	LOG_INF("Zthulhu starting...");

	/* Initialize USB CDC ACM */
	if (!device_is_ready(console_dev)) {
		LOG_ERR("CDC ACM device not ready");
		return -ENODEV;
	}

	ret = enable_usb_device();
	if (ret != 0) {
		LOG_ERR("Failed to enable USB device support");
		return ret;
	}

	/* Wait for DTR with timeout (optional - don't block indefinitely) */
	if (k_sem_take(&dtr_sem, K_MSEC(5000)) == 0) {
		LOG_INF("Console terminal connected");
	} else {
		LOG_INF("No console terminal connected, continuing anyway");
	}

	/* Initialize display */
	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device not ready");
		return -ENODEV;
	}

	LOG_INF("Display device ready: %s", display_dev->name);

	ret = display_blanking_off(display_dev);
	if (ret != 0) {
		LOG_WRN("Failed to turn off display blanking: %d", ret);
	}

	/* Set up LVGL screen */
	lv_obj_t *screen = lv_screen_active();
	lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

	lv_obj_t *label = lv_label_create(screen);
	lv_label_set_text(label, "Zthulhu: TTRPG Companion");
	lv_obj_set_style_text_color(label, lv_color_hex(0x00FF00), LV_PART_MAIN);
	lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);

	/* Mount SD card and show status on display */
	lv_obj_t *sd_status = lv_label_create(screen);
	lv_obj_align(sd_status, LV_ALIGN_CENTER, 0, 20);

	lv_label_set_text(sd_status, "SD: initializing...");
	lv_obj_set_style_text_color(sd_status, lv_color_hex(0xFFFF00), LV_PART_MAIN);
	lv_timer_handler();

	ret = zth_file_manager_mount_sdcard();
	if (ret != 0) {
		const char *step = zth_file_manager_get_error_step();
		int code = zth_file_manager_get_error_code();
		LOG_WRN("Failed to mount SD card: %d (continuing without storage)", ret);
		lv_label_set_text_fmt(sd_status, "SD: %s err %d", step ? step : "unknown", code);
		lv_obj_set_style_text_color(sd_status, lv_color_hex(0xFF0000), LV_PART_MAIN);
	} else {
		LOG_INF("SD card mounted at %s", ZTH_MOUNT_POINT);
		lv_label_set_text_fmt(sd_status, "SD: mounted at %s", ZTH_MOUNT_POINT);
		lv_obj_set_style_text_color(sd_status, lv_color_hex(0x00FF00), LV_PART_MAIN);
		zth_file_manager_print_directory(ZTH_MOUNT_POINT, 0);
	}

	lv_timer_handler();
	LOG_INF("LVGL initialized, entering main loop");

	/* Main loop - LVGL timer handler
	 * Note: lv_tick_inc() is handled automatically by Zephyr's LVGL integration
	 */
	while (1) {
		lv_timer_handler();
		k_msleep(UI_TICK_MS);
	}

	return 0;
}
