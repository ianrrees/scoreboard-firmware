#include <avr/sleep.h>
#include <Wire.h>

class Scoreboard_mockup
{
    public:
        Scoreboard_mockup();

        /// Indices start at 0 on the left, value is [0-9]
        void set_digit(int index, int value);

        /// Turns the digit on or off (real one will likely be variable)
        void enable_digit(int index, bool enable = true);
        
    protected:
        static const int i2c_addr = 0x3f;
        static const int digits_per_team = 2;
        static const int lcd_width = 16;

        static const int rs_pin = 0;
        static const int rw_pin = 1;
        static const int en_pin = 2;
        static const int backlight_pin = 3;
        // assumes D4-D7 are in order
        static const int data_nibble_shift = 4;

        void init_lcd();
        void update_lcd();

        /// Writes a byte of either command or data out to the LCD
        void write_lcd(char val, bool is_data);

        /// Write byte out to i2c interface
        void write_raw(char val);

        // '0'-'9' are valid
        char digits[2 * digits_per_team];

        bool enabled[2 * digits_per_team];
}; // end class Scoreboard_mockup

Scoreboard_mockup::Scoreboard_mockup()
{
    Wire.begin();   // defaults to 100kHz I2C
    for( auto i(0); i < 2 * digits_per_team; ++i) {
        digits[i] = '0';
        enabled[i] = false;
    }
    init_lcd();
}

void Scoreboard_mockup::set_digit(int index, int value)
{
    if ( index < 0 || index >= 2 * digits_per_team) {
        Serial.println("invalid index in set_digit");
        return;
    }

    if ( value < 0 || value > 9) {
        Serial.println("invalid value in set_digit");
        return;
    }
    
    digits[index] = value + '0';

    update_lcd();
}

void Scoreboard_mockup::enable_digit(int index, bool enable /* = true */)
{
    if ( index < 0 || index >= 2 * digits_per_team) {
        Serial.println("invalid index in enable_digit");
        return;
    }

    enabled[index] = enable;

    update_lcd();
}

void Scoreboard_mockup::init_lcd()
{
    // At this point, we don't know what state the controller is in.
    // It could think we're in either:
    //   8-bit interface mode (normal startup condition)
    //   4-bit interface mode, waiting on high nibble
    //   4-bit interface mode, waiting on low nibble

    write_raw(0);
    delay(15);  // Display controller in busy state for 10ms after power up

    // For any of the above three conditions, set to 8-bit interface, because
    // otherwise we can't differentiate between the two 4-bit cases
    for(int i(0); i < 3; ++i) {
        write_raw(0x03 << data_nibble_shift | 1 << en_pin);
        write_raw(0x03 << data_nibble_shift);
    }

    // Now, go to 4b interface (1 line 8x10) using the 8-bit command
    write_raw(0x02 << data_nibble_shift | 1 << en_pin);
    write_raw(0x02 << data_nibble_shift);

    // We're in 4-bit mode from now on
    write_lcd(0x28, false); // Function set: 4b interface, 2 lines, 5x7 font

    write_lcd(0x06, false); // Entry mode set: auto increment, no display shift
    write_lcd(0x0C, false); // Display control: Display on, cursor off, no blink
    write_lcd(0x01, false); // Clear display (and goes home)
    delay(2); // Clear is slow

    char header[lcd_width + 1];
    memset(header, ' ', lcd_width);
    strncpy(header, "Home", 4);
    strcpy(header + lcd_width - 4, "Away");
    for(auto i(0u); i < sizeof(header); ++i) {
        write_lcd(header[i], true);
    }
}

void Scoreboard_mockup::update_lcd()
{
    // Blat out the numbers - home team first
    write_lcd(0x80 | 0x41, false);
    auto i(0);
    for ( ; i < digits_per_team; ++i) {
        if (enabled[i]) {
            write_lcd(digits[i], true);
        } else {
            write_lcd(' ', true);
        }
    }

    // Away team
    write_lcd(0x80 | (0x40 + lcd_width - 1 - digits_per_team), false);
    for ( ; i < 2 * digits_per_team; ++i) {
        if (enabled[i]) {
            write_lcd(digits[i], true);
        } else {
            write_lcd(' ', true);
        }
    }
}

void Scoreboard_mockup::write_lcd(char val, bool is_data)
{
    char out_temp( (is_data ? 1 : 0) << rs_pin |
                   0 << rw_pin |
                   1 << backlight_pin );

    char high_nib( ((val >> 4) & 0xF) << data_nibble_shift | out_temp );
    char low_nib( (val & 0xF) << data_nibble_shift | out_temp );

    write_raw(high_nib | 0 << en_pin);
    write_raw(high_nib | 1 << en_pin);
    write_raw(high_nib | 0 << en_pin);
    
    write_raw(low_nib | 1 << en_pin);
    write_raw(low_nib | 0 << en_pin);
}

void Scoreboard_mockup::write_raw(char val)
{
    Wire.beginTransmission(i2c_addr);
    Wire.write(val);
    Wire.endTransmission();
}
    
///// Standard Arduino stuff below here /////

void setup() {
    Serial.begin(9600); // For debugging output
}

void loop() {
    // Make a single scoreboard object, once
    static Scoreboard_mockup scoreboard;

    // Set to Home:42 Away:07
    scoreboard.enable_digit(0, true);
    scoreboard.enable_digit(1, true);
    scoreboard.enable_digit(2, true);
    scoreboard.enable_digit(3, true);
    scoreboard.set_digit(0, 4);
    scoreboard.set_digit(1, 2);
    scoreboard.set_digit(2, 0);
    scoreboard.set_digit(3, 7);

    // Stops the Arduino's CPU
    cli();
    sleep_enable();
    sleep_cpu();
    while(1);   // Don't get here
}
