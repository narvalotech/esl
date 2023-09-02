#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

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

int main(void)
{
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
