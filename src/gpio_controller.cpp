#include "gpio_controller.hpp"

GPIOController GPIOController::m_instance = GPIOController();

GPIOController::GPIOController() : m_device(nullptr)
{
    m_pin_mask = 0;
    for (u8_t i = 0; i < MAX_INPUT_PINS; i++) {
        m_pins[i] = nullptr;
    }
}
void GPIOController::init(PinInput *pin_input)
{
    if (pin_input && m_device == nullptr) {
        m_device = pin_input->device();
        gpio_add_callback(m_device, &m_gpio_cb);
    }
    this->add_callback(pin_input);
}
int GPIOController::add_callback(PinInput *pin_input)
{
    m_pin_mask = m_pin_mask | BIT(pin_input->pin());
    gpio_init_callback(&m_gpio_cb, &GPIOController::changed, m_pin_mask);
    printk("Error 2: %d\n", gpio_pin_enable_callback(m_device, pin_input->pin()));
    s8_t available = -1;
    bool exists    = false;
    for (u8_t i = 0; i < MAX_INPUT_PINS; i++) {
        if (m_pins[i] == nullptr) {
            available = i;
            break;
        } else if (m_pins[i] == pin_input) {
            exists = true;
            break;
        }
    }
    if (!exists) {
        if (available >= 0) {
            m_pins[available] = pin_input;
            printk("Pin added at %d\n", available);
        } else {
            printk("No more room for this item\n");
        }
    }
    return 0;
}