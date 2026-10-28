#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/drivers/mfd/npm13xx.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/workqueue.h>
#include "fuel_gauge.h"

LOG_MODULE_REGISTER(kausa_board_init);

static const struct device *pmic = DEVICE_DT_GET(DT_NODELABEL(npm1300_pmic));
static const struct device *charger = DEVICE_DT_GET(DT_NODELABEL(npm1300_charger));
atomic_t vbus_connected;

static void event_callback(const struct device *dev, struct gpio_callback *cb, uint32_t events)
{
	if (events & BIT(NPM13XX_EVENT_SHIPHOLD_PRESS)) {
		LOG_DBG("SHPHLD pressed");
	}
	if (events & BIT(NPM13XX_EVENT_SHIPHOLD_RELEASE)) {
		LOG_DBG("SHPHLD released");
		sys_reboot(SYS_REBOOT_WARM);
	}
	if (events & BIT(NPM13XX_EVENT_VBUS_DETECTED)) {
		LOG_DBG("VBUS detected\n");
		atomic_set_bit(&vbus_connected, 0);
	}
	if (events & BIT(NPM13XX_EVENT_VBUS_REMOVED)) {
		LOG_DBG("VBUS removed\n");
		atomic_clear_bit(&vbus_connected, 0);
	}
}
static void npm1300_work(struct k_work *work) {
	int soc = 0;
	int rc = 0;

	fuel_gauge_update(charger, atomic_test_bit(&vbus_connected, 0), &soc);

	LOG_DBG("soc_i: %d", soc);

	
        rc = raise_zmk_battery_state_changed(
            (struct zmk_battery_state_changed){.state_of_charge = soc});

        if (rc != 0) {
            LOG_ERR("Failed to raise battery state changed event: %d", rc);
        }
}

K_WORK_DEFINE(fuel_gauge_work, npm1300_work);

static void zmk_npm1300_timer(struct k_timer *timer) {
    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &fuel_gauge_work);
}

K_TIMER_DEFINE(fuel_gauge_timer, zmk_npm1300_timer, NULL);

static void npm1300_start_reporting() {
	k_timer_start(&fuel_gauge_timer, K_NO_WAIT, K_SECONDS(CONFIG_ZMK_BATTERY_REPORT_INTERVAL));
}

static int battery_event_listener(const zmk_event_t *eh) {

    if (as_zmk_activity_state_changed(eh)) {
        switch (zmk_activity_get_state()) {
        case ZMK_ACTIVITY_ACTIVE:
            npm1300_start_reporting();
            return 0;
        case ZMK_ACTIVITY_IDLE:
        case ZMK_ACTIVITY_SLEEP:
            k_timer_stop(&fuel_gauge_timer);
            return 0;
        default:
            break;
        }
    }
    return -ENOTSUP;
}

ZMK_LISTENER(charger, battery_event_listener);

void board_init(void) {
	if (!device_is_ready(pmic)) {
		LOG_ERR("Pmic device not ready.");
		// return 0;
	}

	if (!device_is_ready(charger)) {
		LOG_ERR("Charger device not ready.");
		// return 0;
	}

	if (fuel_gauge_init(charger) < 0) {
		LOG_ERR("Could not initialise fuel gauge.");
		// return 0;
	}
	LOG_INF("Let's go");
}

void configure_events(void)
{
	LOG_INF("Configuring events");
	static struct gpio_callback event_cb;

	gpio_init_callback(&event_cb, event_callback, BIT(NPM13XX_EVENT_SHIPHOLD_PRESS) |
			   BIT(NPM13XX_EVENT_SHIPHOLD_RELEASE) | BIT(NPM13XX_EVENT_VBUS_DETECTED) | BIT(NPM13XX_EVENT_VBUS_REMOVED));

	int rc = mfd_npm13xx_add_callback(pmic, &event_cb);
	if (rc != 0) {
		LOG_ERR("Failed to add event callback: %d", rc);
		return;
	}


	/* Initialise vbus detection status. */
	struct sensor_value val;
	sensor_attr_get(charger, SENSOR_CHAN_CURRENT, SENSOR_ATTR_UPPER_THRESH, &val);
	// int ret = sensor_attr_get(charger, SENSOR_CHAN_CURRENT, SENSOR_ATTR_UPPER_THRESH, &val);

	// if (ret < 0) {
	// 	return false;
	// }

	atomic_set_bit_to(&vbus_connected, 0, (val.val1 != 0) || (val.val2 != 0));
	npm1300_start_reporting(); // do we need this?
}




void board_late_init_hook(void) {
	board_init();
	configure_events();
}
