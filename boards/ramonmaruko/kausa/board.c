#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/drivers/mfd/npm13xx.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(kausa_board_init);

static const struct device *pmic = DEVICE_DT_GET(DT_NODELABEL(npm1300_pmic));

static void event_callback(const struct device *dev, struct gpio_callback *cb, uint32_t events)
{
	if (events & BIT(NPM13XX_EVENT_SHIPHOLD_PRESS)) {
		LOG_INF("SHPHLD pressed\n");
	}
	if (events & BIT(NPM13XX_EVENT_SHIPHOLD_RELEASE)) {
		LOG_INF("SHPHLD released\n");
		sys_reboot(SYS_REBOOT_WARM);
	}
	if (events & BIT(NPM13XX_EVENT_VBUS_DETECTED)) {
		printk("VBUS detected\n");
	}
	if (events & BIT(NPM13XX_EVENT_VBUS_REMOVED)) {
		printk("VBUS removed\n");
	}
}

void configure_events(void)
{
	LOG_INF("Configuring events");
	if (!device_is_ready(pmic)) {
		printk("Pmic device not ready.\n");
		return;
	}

	/* Setup callback for shiphold button press */
	static struct gpio_callback event_cb;

	gpio_init_callback(&event_cb, event_callback, BIT(NPM13XX_EVENT_SHIPHOLD_PRESS) |
			   BIT(NPM13XX_EVENT_SHIPHOLD_RELEASE) | BIT(NPM13XX_EVENT_VBUS_DETECTED) | BIT(NPM13XX_EVENT_VBUS_REMOVED));
	// gpio_init_callback(&event_cb, event_callback, BIT(NPM13XX_EVENT_SHIPHOLD_PRESS) |
	// 		   BIT(NPM13XX_EVENT_SHIPHOLD_RELEASE));

	int rc = mfd_npm13xx_add_callback(pmic, &event_cb);
	if (rc != 0) {
		LOG_ERR("Failed to add event callback: %d", rc);
		return;
	}
}

void board_late_init_hook(void) {
	printk("RAMON\n");
	configure_events();
}
