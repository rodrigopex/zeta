/**!
 * @file gpio_controller.hpp
 * @author Rodrigo Peixoto (rodrigopex@edge.ufal.br)
 * @brief
 * @version 0.1
 * @date 2019-01-28
 *
 */

#include <device.h>
#include <gpio.h>
#include <misc/printk.h>
#include <zephyr.h>

#define MAX_INPUT_PINS 4

#include "property.hpp"


class GPIOController
{
   public:
    void init(const char *controller);
    int add_callback(zt::DigitalInput *pin_input);
    static void changed(struct device *dev, struct gpio_callback *cb, u32_t pins)
    {
        GPIOController *c = GPIOController::instance();
        for (u8_t i = 0; c->m_pins[i] && (i < MAX_INPUT_PINS); i++) {
            if (BIT(c->m_pins[i]->pin()) & pins) {
                c->m_pins[i]->callback();
            }
        }
    }
    static GPIOController *instance()
    {
        return &m_instance;
    }

   private:
    GPIOController();
    struct gpio_callback m_gpio_cb;
    struct device *m_device;
    zt::DigitalInput *m_pins[MAX_INPUT_PINS];
    static GPIOController m_instance;
    u32_t m_pin_mask;
};