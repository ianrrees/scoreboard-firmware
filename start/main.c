// Firmware to go with the VCW / pact scoreboard
// 
// Ian Rees May 2017
//
#include <atmel_start.h>

// Segments are encoded as seen from font:
//
//  --E--
// |     |
// C     G
// |--D--|
// B     F
// |     |
//  --A--

// The IIC slave address is determined by the state of the ADDR jumpers:
// address = IIC_BASE_ADDRESS + offset
//
// offset | ADDR3  | ADDR2  | ADDR1
//    0   | Open   | Open   | Open
//    1   | Open   | Open   | Jumped 
//    2   | Open   | Jumped | Open
//    3   | Open   | Jumped | Jumped 
//    4   | Jumped | Open   | Open
//    5   | Jumped | Open   | Jumped 
//    6   | Jumped | Jumped | Open
//    7   | Jumped | Jumped | Jumped 
#define IIC_BASE_ADDRESS 0x10

/// These need to be representable with 8-bits
enum IIC_command_enum {
    ZERO,
    ONE,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE,

    IIC_COMMAND_OFF = 0xFF
};

// WARNING: This is shared with the reset pin, don't make it an output
// until waiting a second to leave a window for re-programming
#define HEARTBEAT_PIN PIN_PA30

#define SEGMENT_A_PIN PIN_PA25
#define SEGMENT_B_PIN PIN_PA24
#define SEGMENT_C_PIN PIN_PA02
#define SEGMENT_D_PIN PIN_PA04
#define SEGMENT_E_PIN PIN_PA05
#define SEGMENT_F_PIN PIN_PA08
#define SEGMENT_G_PIN PIN_PA09

#define SEGMENT_A_MASK 0x01
#define SEGMENT_B_MASK 0x02
#define SEGMENT_C_MASK 0x04
#define SEGMENT_D_MASK 0x08
#define SEGMENT_E_MASK 0x10
#define SEGMENT_F_MASK 0x20
#define SEGMENT_G_MASK 0x40

void show_digit(enum IIC_command_enum value)
{
    uint8_t led_mask;

    switch(value) {
        case ZERO:
            led_mask = SEGMENT_A_MASK |
                       SEGMENT_B_MASK |
                       SEGMENT_C_MASK |
                       SEGMENT_E_MASK |
                       SEGMENT_F_MASK |
                       SEGMENT_G_MASK;
            break;

        case ONE:
            led_mask = SEGMENT_F_MASK |
                       SEGMENT_G_MASK;
            break;

        case TWO:
            led_mask = SEGMENT_A_MASK |
                       SEGMENT_B_MASK |
                       SEGMENT_D_MASK |
                       SEGMENT_E_MASK |
                       SEGMENT_G_MASK;
            break;

        case THREE:
            led_mask = SEGMENT_A_MASK |
                       SEGMENT_D_MASK |
                       SEGMENT_E_MASK |
                       SEGMENT_F_MASK |
                       SEGMENT_G_MASK;
            break;

        case FOUR:
            led_mask = SEGMENT_C_MASK |
                       SEGMENT_D_MASK |
                       SEGMENT_F_MASK |
                       SEGMENT_G_MASK;
            break;

        case FIVE:
            led_mask = SEGMENT_A_MASK |
                       SEGMENT_C_MASK |
                       SEGMENT_D_MASK |
                       SEGMENT_E_MASK |
                       SEGMENT_F_MASK;
            break;

        case SIX:
            led_mask = SEGMENT_A_MASK |
                       SEGMENT_B_MASK |
                       SEGMENT_C_MASK |
                       SEGMENT_D_MASK |
                       SEGMENT_E_MASK |
                       SEGMENT_F_MASK;
            break;

        case SEVEN:
            led_mask = SEGMENT_E_MASK |
                       SEGMENT_F_MASK |
                       SEGMENT_G_MASK;
            break;

        case EIGHT:
            led_mask = SEGMENT_A_MASK |
                       SEGMENT_B_MASK |
                       SEGMENT_C_MASK |
                       SEGMENT_D_MASK |
                       SEGMENT_E_MASK |
                       SEGMENT_F_MASK |
                       SEGMENT_G_MASK;
            break;

        case NINE:
            led_mask = SEGMENT_C_MASK |
                       SEGMENT_D_MASK |
                       SEGMENT_E_MASK |
                       SEGMENT_F_MASK |
                       SEGMENT_G_MASK;
            break;

        default:
            led_mask = 0;
            break;
    }

    gpio_set_pin_level(SEGMENT_A_PIN, led_mask & SEGMENT_A_MASK);
    gpio_set_pin_level(SEGMENT_B_PIN, led_mask & SEGMENT_B_MASK);
    gpio_set_pin_level(SEGMENT_C_PIN, led_mask & SEGMENT_C_MASK);
    gpio_set_pin_level(SEGMENT_D_PIN, led_mask & SEGMENT_D_MASK);
    gpio_set_pin_level(SEGMENT_E_PIN, led_mask & SEGMENT_E_MASK);
    gpio_set_pin_level(SEGMENT_F_PIN, led_mask & SEGMENT_F_MASK);
    gpio_set_pin_level(SEGMENT_G_PIN, led_mask & SEGMENT_G_MASK);
}

/// Twiddles GPIO pins to figure out what our IIC address is set to
uint8_t get_address(void)
{
    uint8_t offset = 0;

    // ADDR1
    gpio_set_pin_direction(SEGMENT_C_PIN, GPIO_DIRECTION_IN);
    gpio_set_pin_direction(SEGMENT_B_PIN, GPIO_DIRECTION_OUT);
    gpio_set_pin_level(SEGMENT_B_PIN, 1);
    if (gpio_get_pin_level(SEGMENT_C_PIN)) {
        offset += 1;
    }
    gpio_set_pin_level(SEGMENT_B_PIN, 0);

    // ADDR2
    gpio_set_pin_direction(SEGMENT_E_PIN, GPIO_DIRECTION_IN);
    gpio_set_pin_direction(SEGMENT_D_PIN, GPIO_DIRECTION_OUT);
    gpio_set_pin_level(SEGMENT_D_PIN, 1);
    if (gpio_get_pin_level(SEGMENT_E_PIN)) {
        offset += 2;
    }
    gpio_set_pin_level(SEGMENT_D_PIN, 0);

    // ADDR3
    gpio_set_pin_direction(SEGMENT_G_PIN, GPIO_DIRECTION_IN);
    gpio_set_pin_direction(SEGMENT_A_PIN, GPIO_DIRECTION_OUT);
    gpio_set_pin_level(SEGMENT_A_PIN, 1);

    if (gpio_get_pin_level(SEGMENT_G_PIN)) {
        offset += 4;
    }
    gpio_set_pin_level(SEGMENT_A_PIN, 0);

    return IIC_BASE_ADDRESS + offset;
}

static void I2C_0_error(const struct i2c_s_async_descriptor *const descr)
{
    REG_SERCOM1_I2CS_INTENCLR = (1 << 7);
}

static void I2C_0_tx_complete(const struct i2c_s_async_descriptor *const descr)
{
    REG_SERCOM1_I2CS_CTRLB &= ~(1<<18); // Enable ACK
    REG_SERCOM1_I2CS_CTRLB |= (3<<16); // Send ACK
}

/// Setup asynchronous I2C slave
void setup_iic(uint8_t address)
{
    i2c_s_async_register_callback(&I2C_0, I2C_S_ERROR, I2C_0_error);    
    i2c_s_async_register_callback(&I2C_0, I2C_S_TX_COMPLETE, I2C_0_tx_complete);

    i2c_s_async_set_addr(&I2C_0, address);
    i2c_s_async_enable(&I2C_0);    
}

void led_init(void)
{
    gpio_set_pin_direction(SEGMENT_A_PIN, GPIO_DIRECTION_OUT);
    gpio_set_pin_direction(SEGMENT_B_PIN, GPIO_DIRECTION_OUT);
    gpio_set_pin_direction(SEGMENT_C_PIN, GPIO_DIRECTION_OUT);
    gpio_set_pin_direction(SEGMENT_D_PIN, GPIO_DIRECTION_OUT);
    gpio_set_pin_direction(SEGMENT_E_PIN, GPIO_DIRECTION_OUT);
    gpio_set_pin_direction(SEGMENT_F_PIN, GPIO_DIRECTION_OUT);
    gpio_set_pin_direction(SEGMENT_G_PIN, GPIO_DIRECTION_OUT);
    gpio_set_pin_level(SEGMENT_A_PIN, 0);
    gpio_set_pin_level(SEGMENT_B_PIN, 0);
    gpio_set_pin_level(SEGMENT_C_PIN, 0);
    gpio_set_pin_level(SEGMENT_D_PIN, 0);
    gpio_set_pin_level(SEGMENT_E_PIN, 0);
    gpio_set_pin_level(SEGMENT_F_PIN, 0);
    gpio_set_pin_level(SEGMENT_G_PIN, 0);
}

bool heartbeat_enabled = false;

/// Don't start heartbeat right away - otherwise can't reprogram
static void TIMER_0_task1_cb(const struct timer_task *const timer_task)
{
    heartbeat_enabled = true;
    gpio_set_pin_direction(HEARTBEAT_PIN, GPIO_DIRECTION_OUT);
}

/// PWM the heartbeat LED
static void TIMER_0_task2_cb(const struct timer_task *const timer_task)
{
    static const int HEARTRATE = 3;
    static const int HEARTBEAT_PWM_TICKS = 500;
    static const int HEARTBEAT_MIN_BRIGHTNESS = 1;
    static const int HEARTBEAT_MAX_BRIGHTNESS = 100;
    static int heartbeat_counter = 0;
    static int heartbeat_level = 0; // Brightness 0=off HEARTBEAT_PWM_TICKS=full
    static bool beat_direction = false;

    if (heartbeat_enabled) {
        // low = light on
        gpio_set_pin_level(HEARTBEAT_PIN, heartbeat_counter > heartbeat_level);

        if (++heartbeat_counter >= HEARTBEAT_PWM_TICKS) {
            heartbeat_counter = 0;

            if (beat_direction) {
                heartbeat_level += HEARTRATE;
                if (heartbeat_level > HEARTBEAT_MAX_BRIGHTNESS) {
                    beat_direction = false;
                    heartbeat_level = HEARTBEAT_MAX_BRIGHTNESS;
                }
            } else {
                heartbeat_level -= HEARTRATE;
                if (heartbeat_level < HEARTBEAT_MIN_BRIGHTNESS) {
                    beat_direction = true;
                    heartbeat_level = HEARTBEAT_MIN_BRIGHTNESS;
                }
            }
        }
    }
}

int main(void)
{
    atmel_start_init();

    struct io_descriptor *i2c_slave;
    i2c_s_async_get_io_descriptor(&I2C_0, &i2c_slave);

    setup_iic( get_address() );
    led_init();

    struct timer_task TIMER_0_task1;
    TIMER_0_task1.interval = 400000;
    TIMER_0_task1.cb = TIMER_0_task1_cb;
    TIMER_0_task1.mode = TIMER_TASK_ONE_SHOT;
    timer_add_task(&TIMER_0, &TIMER_0_task1);

    struct timer_task TIMER_0_task2;
    TIMER_0_task2.interval = 2;
    TIMER_0_task2.cb = TIMER_0_task2_cb;
    TIMER_0_task2.mode = TIMER_TASK_REPEAT;
    timer_add_task(&TIMER_0, &TIMER_0_task2);

    timer_set_clock_cycles_per_tick(&TIMER_0, 20);
    timer_start(&TIMER_0);

    uint8_t cmd_byte = 0;
    while (1) {
        if (i2c_slave->read(i2c_slave, &cmd_byte, 1)) {
            switch(cmd_byte) {
                case ZERO:
                case ONE:
                case TWO:
                case THREE:
                case FOUR:
                case FIVE:
                case SIX:
                case SEVEN:
                case EIGHT:
                case NINE:
                case IIC_COMMAND_OFF: // show_digit() turns off segments for invalid digits
                    show_digit(cmd_byte);
                default:
                    break;
            }
        }
    }
}
