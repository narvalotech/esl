#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/led.h>

#include "framebuffer.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, 4);

void setup_usb(void)
{
#if CONFIG_USB_DEVICE_STACK
	const struct device *dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
	if (!device_is_ready(dev)) {
		LOG_ERR("CDC ACM device not ready");
		return;
	}

	int ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB: %d", ret);
		return;
	}
#endif
}

static void input_cb(struct input_event *evt)
{
	/* Don't do anything for now */
	printk("sync %u type %u code 0x%x value %d\n",
	       evt->sync, evt->type, evt->code, evt->value);
}

/* Invoke callback for all input devices */
INPUT_CALLBACK_DEFINE(NULL, input_cb);

void led_test(void)
{
#define LEDS_NODE_ID DT_COMPAT_GET_ANY_STATUS_OKAY(gpio_leds)

	if (IS_ENABLED(CONFIG_NATIVE_APPLICATION)) {
		return;
	}

	const char *led_label[] = {
	DT_FOREACH_CHILD_SEP_VARGS(LEDS_NODE_ID, DT_PROP_OR, (,), label, NULL)};

	const int num_leds = ARRAY_SIZE(led_label);

	const struct device *leds;

	leds = DEVICE_DT_GET(LEDS_NODE_ID);
	if (!device_is_ready(leds)) {
		LOG_ERR("Device %s is not ready", leds->name);
		return;
	}

	if (!num_leds) {
		LOG_ERR("No LEDs found for %s", leds->name);
		return;
	}

	/* Turn on all LEDs */
	for (uint8_t led = 0; led < num_leds; led++) {
		LOG_INF("turn on LED %d", led);
		led_on(leds, led);
		k_msleep(300);
		led_off(leds, led);
	}
}

int main(void)
{
	led_test();

	setup_usb();

	const struct device *dev =
		DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	framebuffer_setup(dev);

	printk("entering main loop\n");

	/* framebuffer rendering loop */
	while (1) {
		framebuffer_update(dev);
	}

	return 0;
}
