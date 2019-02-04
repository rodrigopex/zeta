/**!
 * @file zeta.hpp
 * @author Rodrigo Peixoto (rodrigopex@gmail.com)
 * @brief
 * @version 0.1
 * @date 2019-02-04
 *
 *
 */
#include <device.h>
#include <gpio.h>
#include <misc/printk.h>
#include <string.h>
#include <zephyr.h>

#define ZT_ALLOC_BYTES(x) (u8_t *) alloca(x)
#define ZT_ALLOC_BYTE() (u8_t *) alloca(1)

#define ZT_MAX_OBSERVERS 4

typedef enum {
    VOLATILE_FLAG  = 0,
    IN_FLASH_FLAG  = 1 << 0,
    READ_ONLY_FLAG = 1 << 1
} PropertyFlag;

namespace zt
{
class Property
{
   public:
    virtual u8_t *data();
    virtual int setData(u8_t *data, size_t size);
    virtual size_t size();
    virtual int notify() = 0;

   protected:
    size_t m_size;
    u8_t *m_data;
    u8_t m_changed;
    u8_t m_flags;
};

class HardProperty;

struct Observer {
    HardProperty *property;
    u8_t method;
    Observer(HardProperty *property, u8_t method) : property(property), method(method)
    {
    }
    Observer()
    {
        property = nullptr;
        method   = 0;
    }
};

class HardProperty : public Property
{
   public:
    HardProperty();
    virtual int notify();
    virtual void connect(HardProperty *property, u8_t method);
    virtual struct device *device() = 0;
    virtual u32_t pin()             = 0;
    virtual int configureFlags()    = 0;
    virtual int read(u8_t *state)   = 0;
    virtual int write(u8_t state);
    virtual int toggle();

   protected:
    Observer m_observer;
};


class DigitalInput : public HardProperty
{
   protected:
    struct device *m_device;
    u32_t m_pin;

   public:
    DigitalInput(u8_t *data, const char *controller, u32_t pin);
    int configureFlags();
    int read(u8_t *state);
    int callback();
    struct device *device();
    u32_t pin();

    virtual int notify();
};

class DigitalOutput : public HardProperty
{
   protected:
    struct device *m_device;
    u32_t m_pin;

   public:
    DigitalOutput(u8_t *data, const char *controller, u32_t pin);
    virtual int notify();
    int configureFlags();
    int read(u8_t *state);
    int write(u8_t state);
    int toggle();
    struct device *device();
    u32_t pin();
};


/* GPIOController */

#define ZT_CALLBACK_GENERATE(name)                                     \
    [name](struct device *dev, struct gpio_callback *cb, u32_t pins) { \
        name.notifyObservers(dev, pins);                               \
    }

typedef void (*gpio_callback_t)(struct device *dev, struct gpio_callback *cb, u32_t pins);
class GPIOController
{
   public:
    GPIOController(const char *controller, gpio_callback_t gpio_callback);
    int add_observer(zt::DigitalInput *pin_input);
    void notifyObservers(struct device *dev, u32_t pins)
    {
        for (u8_t i = 0; m_observers[i] && (i < ZT_MAX_OBSERVERS); i++) {
            if (BIT(m_observers[i]->pin()) & pins) {
                m_observers[i]->callback();
            }
        }
    }

   private:
    gpio_callback_t m_gpio_callback;
    struct gpio_callback m_gpio_cb;
    struct device *m_device                         = nullptr;
    zt::DigitalInput *m_observers[ZT_MAX_OBSERVERS] = {nullptr};
    u32_t m_pin_mask                                = 0;
};

}  // namespace zt
