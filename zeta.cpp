#include "zeta.hpp"

namespace zt
{
u8_t *Property::data()
{
    return m_data;
}
int Property::setData(u8_t *data, size_t size)
{
    if (size == m_size) {
        memcpy(m_data, data, size);
        return 0;
    }
    return -EINVAL;
}
size_t Property::size()
{
    return m_size;
}


/* HardProperty */
HardProperty::HardProperty() : m_observer(Observer(nullptr, 0))
{
}
int HardProperty::write(u8_t state)
{
    return 0;
}
int HardProperty::notify()
{
    if (m_observer.property) {
        switch (m_observer.method) {
        case 'w':
            m_observer.property->write(*m_data);
            break;
        case 't':
            m_observer.property->toggle();
            break;
        case 'n':
            m_observer.property->notify();
            break;
        default:
            break;
        }
    }
    return 0;
}
int HardProperty::toggle()
{
    return 0;
}

void HardProperty::connect(HardProperty *property, u8_t method)
{
    m_observer.property = property;
    m_observer.method   = method;
}

/* DigitalInput */

DigitalInput::DigitalInput(u8_t *data, const char *controller, u32_t pin) : m_pin(pin)
{
    m_data   = data;
    *data    = 0;
    m_device = device_get_binding(controller);
    gpio_pin_configure(m_device, m_pin, this->configureFlags());
}
int DigitalInput::configureFlags()
{
    return (GPIO_PUD_PULL_UP | GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE
            | GPIO_INT_ACTIVE_LOW);
}
int DigitalInput::read(u8_t *state)
{
    *state = *m_data;
    return 0;
}
int DigitalInput::callback()
{
    u8_t state = 0;
    int error  = gpio_pin_read(m_device, m_pin, (u32_t *) &state);
    if (!error) {
        *m_data = state;
        this->notify();
    }
    return 0;
}
struct device *DigitalInput::device()
{
    return m_device;
}
u32_t DigitalInput::pin()
{
    return m_pin;
}
int DigitalInput::notify()
{
    printk("Triggered pin %d:%d\n", m_pin, (u8_t) *m_data);
    HardProperty::notify();
    return 0;
}

/* DigitalOutput */

DigitalOutput::DigitalOutput(u8_t *data, const char *controller, u32_t pin) : m_pin(pin)
{
    m_data   = data;
    *data    = 0;
    m_device = device_get_binding(controller);
    gpio_pin_configure(m_device, m_pin, this->configureFlags());
}
int DigitalOutput::configureFlags()
{
    return GPIO_DIR_OUT;
}
int DigitalOutput::read(u8_t *state)
{
    return gpio_pin_read(m_device, m_pin, (u32_t *) state);
}
int DigitalOutput::write(u8_t state)
{
    int error = gpio_pin_write(m_device, m_pin, state);
    if (!error) {
        *m_data = state;
        this->notify();
    }
    return error;
}
int DigitalOutput::toggle()
{
    int error = gpio_pin_write(m_device, m_pin, !*m_data);
    if (!error) {
        *m_data = !*m_data;
        this->notify();
    }
    return error;
}
int DigitalOutput::notify()
{
    printk("State changed to %d\n", *m_data);
    HardProperty::notify();
    return 0;
}
struct device *DigitalOutput::device()
{
    return m_device;
}
u32_t DigitalOutput::pin()
{
    return m_pin;
}

/* GPIOController */

GPIOController::GPIOController(const char *controller, gpio_callback_t gpio_callback)
    : m_gpio_callback(gpio_callback)
{
    m_device = device_get_binding(controller);
    if (!m_device) {
        // ERROR
    } else {
        gpio_add_callback(m_device, &m_gpio_cb);
    }
}
int GPIOController::add_observer(DigitalInput *pin_input)
{
    m_pin_mask = m_pin_mask | BIT(pin_input->pin());
    gpio_init_callback(&m_gpio_cb, m_gpio_callback, m_pin_mask);
    printk("Error 2: %d\n", gpio_pin_enable_callback(m_device, pin_input->pin()));
    s8_t available = -1;
    bool exists    = false;
    for (u8_t i = 0; i < ZT_MAX_OBSERVERS; i++) {
        if (m_observers[i] == nullptr) {
            available = i;
            break;
        } else if (m_observers[i] == pin_input) {
            exists = true;
            break;
        }
    }
    if (!exists) {
        if (available >= 0) {
            m_observers[available] = pin_input;
            printk("Pin added at %d\n", available);
        } else {
            printk("No more room for this item\n");
        }
    }
    return 0;
}

}  // namespace zt