#include <device.h>
#include <gpio.h>
#include <misc/printk.h>
#include <string.h>
#include <zephyr.h>

#define LED_PORT LED0_GPIO_CONTROLLER
#define LED LED1_GPIO_PIN

#define BUTTON0_PIN SW0_GPIO_PIN
#define BUTTON1_PIN SW1_GPIO_PIN
#define BUTTON2_PIN SW2_GPIO_PIN
#define BUTTON3_PIN SW3_GPIO_PIN
#define BUTTON_GPIO_CONTROLLER SW0_GPIO_CONTROLLER

typedef enum {
    VOLATILE_FLAG  = 0,
    IN_FLASH_FLAG  = 1 << 0,
    READ_ONLY_FLAG = 1 << 1
} PropertyFlag;

class Property
{
   public:
    virtual u8_t *data()
    {
        return m_data;
    }
    virtual int setData(u8_t *data, size_t size)
    {
        if (size == m_size) {
            memcpy(m_data, data, size);
            return 0;
        }
        return -EINVAL;
    }
    virtual size_t size()
    {
        return m_size;
    }
    virtual int notify() = 0;

   protected:
    size_t m_size;
    u8_t *m_data;
    u8_t m_changed;
    u8_t m_flags;
};

class HardProperty : public Property
{
   public:
    virtual struct device *device() = 0;
    virtual u32_t pin()             = 0;
    virtual int configureFlags()    = 0;
    virtual int read(u8_t *state)   = 0;
    virtual int write(u8_t state)
    {
        return 0;
    }
};

class PinInput : public HardProperty
{
   protected:
    struct device *m_device;
    u32_t m_pin;

   public:
    PinInput(u8_t *data, const char *controller, u32_t pin) : m_pin(pin)
    {
        m_data   = data;
        m_device = device_get_binding(controller);
        gpio_pin_configure(m_device, m_pin, this->configureFlags());
    }
    int configureFlags()
    {
        return (GPIO_PUD_PULL_UP | GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE
                | GPIO_INT_ACTIVE_LOW);
    }
    int read(u8_t *state)
    {
        *state = *m_data;
        return 0;
    }
    int callback()
    {
        u8_t state = 0;
        int error  = gpio_pin_read(m_device, m_pin, (u32_t *) &state);
        if (!error) {
            *m_data = state;
            this->notify();
        }
        return 0;
    }
    struct device *device()
    {
        return m_device;
    }
    u32_t pin()
    {
        return m_pin;
    }
    virtual int notify()
    {
        printk("Triggered pin %d:%d\n", m_pin, (u8_t) *m_data);
        return 0;
    }
};

class PinOutput : public HardProperty
{
   protected:
    struct device *m_device;
    u32_t m_pin;

   public:
    PinOutput(u8_t *data, const char *controller, u32_t pin) : m_pin(pin)
    {
        m_data   = data;
        m_device = device_get_binding(controller);
        gpio_pin_configure(m_device, m_pin, this->configureFlags());
    }
    int configureFlags()
    {
        return GPIO_DIR_OUT;
    }
    int read(u8_t *state)
    {
        return gpio_pin_read(m_device, m_pin, (u32_t *) state);
    }
    int write(u8_t state)
    {
        int error = gpio_pin_write(m_device, m_pin, state);
        if (!error) {
            *m_data = state;
            this->notify();
        }
        return error;
    }
    int toggle()
    {
        int error = gpio_pin_write(m_device, m_pin, !*m_data);
        if (!error) {
            *m_data = !*m_data;
            this->notify();
        }
        return error;
    }
    virtual int notify()
    {
        printk("State changed to %d\n", *m_data);
        return 0;
    }
    struct device *device()
    {
        return m_device;
    }
    u32_t pin()
    {
        return m_pin;
    }
};