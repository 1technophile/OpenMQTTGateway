/*******************************************************************************
 * Touch libraries:
 * XPT2046: https://github.com/PaulStoffregen/XPT2046_Touchscreen.git
 *
 * Capacitive touchscreen libraries
 * TouchLib: https://github.com/mmMicky/TouchLib.git
 *
 * #define CTS328_SLAVE_ADDRESS  (0x1A)
 * #define L58_SLAVE_ADDRESS     (0X5A)
 * #define CTS826_SLAVE_ADDRESS  (0X15)
 * #define CTS820_SLAVE_ADDRESS  (0X15)
 * #define CTS816S_SLAVE_ADDRESS (0X15)
 * #define FT3267_SLAVE_ADDRESS  (0x38)
 * #define FT5x06_ADDR           (0x38)
 * #define GT911_SLAVE_ADDRESS1  (0X5D)
 * #define GT911_SLAVE_ADDRESS2  (0X14)
 * #define ZTW622_SLAVE1_ADDRESS (0x20)
 * #define ZTW622_SLAVE2_ADDRESS (0x46)
 *
 ******************************************************************************/

/* uncomment for XPT2046 */
// #define TOUCH_XPT2046
// #define TOUCH_XPT2046_SCK 12
// #define TOUCH_XPT2046_MISO 13
// #define TOUCH_XPT2046_MOSI 11
// #define TOUCH_XPT2046_CS 10
// #define TOUCH_XPT2046_INT 18
// #define TOUCH_XPT2046_ROTATION 0
// #define TOUCH_XPT2046_SAMPLES 50

// uncomment for most capacitive touchscreen
// #define TOUCH_MODULES_FT5x06 // GT911 / CST_SELF / CST_MUTUAL / ZTW622 / L58 / FT3267 / FT5x06
// #define TOUCH_MODULE_ADDR FT5x06_ADDR // CTS328_SLAVE_ADDRESS / L58_SLAVE_ADDRESS / CTS826_SLAVE_ADDRESS / CTS820_SLAVE_ADDRESS / CTS816S_SLAVE_ADDRESS / FT3267_SLAVE_ADDRESS / FT5x06_ADDR / GT911_SLAVE_ADDRESS1 / GT911_SLAVE_ADDRESS2 / ZTW622_SLAVE1_ADDRESS / ZTW622_SLAVE2_ADDRESS
// #define TOUCH_SCL 5
// #define TOUCH_SDA 6
// #define TOUCH_RES -1
// #define TOUCH_INT -1

// Please fill below values from Arduino_GFX Example - TouchCalibration
bool touch_swap_xy = false;
int16_t touch_map_x1 = -1;
int16_t touch_map_x2 = -1;
int16_t touch_map_y1 = -1;
int16_t touch_map_y2 = -1;

int16_t touch_max_x = 0, touch_max_y = 0;
int16_t touch_raw_x = 0, touch_raw_y = 0;
int16_t touch_last_x = 0, touch_last_y = 0;

#if defined(TOUCH_XPT2046)
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
XPT2046_Touchscreen ts(TOUCH_XPT2046_CS, TOUCH_XPT2046_INT);

#elif defined(TOUCH_MODULE_ADDR) // TouchLib
#include <Wire.h>
#include <TouchLib.h>
TouchLib touch(Wire, TOUCH_SDA, TOUCH_SCL, TOUCH_MODULE_ADDR);

#endif // TouchLib

void touch_init(int16_t w, int16_t h, uint8_t r)
{
  touch_max_x = w - 1;
  touch_max_y = h - 1;
  if (touch_map_x1 == -1)
  {
    switch (r)
    {
    case 3:
      touch_swap_xy = true;
      touch_map_x1 = touch_max_x;
      touch_map_x2 = 0;
      touch_map_y1 = 0;
      touch_map_y2 = touch_max_y;
      break;
    case 2:
      touch_swap_xy = false;
      touch_map_x1 = touch_max_x;
      touch_map_x2 = 0;
      touch_map_y1 = touch_max_y;
      touch_map_y2 = 0;
      break;
    case 1:
      touch_swap_xy = true;
      touch_map_x1 = 0;
      touch_map_x2 = touch_max_x;
      touch_map_y1 = touch_max_y;
      touch_map_y2 = 0;
      break;
    default: // case 0:
      touch_swap_xy = false;
      touch_map_x1 = 0;
      touch_map_x2 = touch_max_x;
      touch_map_y1 = 0;
      touch_map_y2 = touch_max_y;
      break;
    }
  }

#if defined(TOUCH_XPT2046)
  SPI.begin(TOUCH_XPT2046_SCK, TOUCH_XPT2046_MISO, TOUCH_XPT2046_MOSI, TOUCH_XPT2046_CS);
  ts.begin();
  ts.setRotation(TOUCH_XPT2046_ROTATION);

#elif defined(TOUCH_MODULE_ADDR) // TouchLib
  // Reset touchscreen
#if (TOUCH_RES > 0)
  pinMode(TOUCH_RES, OUTPUT);
  digitalWrite(TOUCH_RES, 0);
  delay(200);
  digitalWrite(TOUCH_RES, 1);
  delay(200);
#endif
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  touch.init();

#endif // TouchLib
}

bool touch_has_signal()
{
#if defined(TOUCH_XPT2046)
  return ts.tirqTouched();

#elif defined(TOUCH_MODULE_ADDR) // TouchLib
  // TODO: implement TOUCH_INT
  return true;
#endif                           // TouchLib

  return false;
}

void translate_touch_raw()
{
  if (touch_swap_xy)
  {
    touch_last_x = map(touch_raw_y, touch_map_x1, touch_map_x2, 0, touch_max_x);
    touch_last_y = map(touch_raw_x, touch_map_y1, touch_map_y2, 0, touch_max_y);
  }
  else
  {
    touch_last_x = map(touch_raw_x, touch_map_x1, touch_map_x2, 0, touch_max_x);
    touch_last_y = map(touch_raw_y, touch_map_y1, touch_map_y2, 0, touch_max_y);
  }
  // Serial.printf("touch_raw_x: %d, touch_raw_y: %d, touch_last_x: %d, touch_last_y: %d\n", touch_raw_x, touch_raw_y, touch_last_x, touch_last_y);
}

bool touch_touched()
{
#if defined(TOUCH_XPT2046)
  if (ts.touched())
  {
    TS_Point p = ts.getPoint();
    touch_raw_x = p.x;
    touch_raw_y = p.y;
    int max_z = p.z;
    int count = 0;
    while ((ts.touched()) && (count < TOUCH_XPT2046_SAMPLES))
    {
      count++;

      TS_Point p = ts.getPoint();
      if (p.z > max_z)
      {
        touch_raw_x = p.x;
        touch_raw_y = p.y;
        max_z = p.z;
      }
      // Serial.printf("touch_raw_x: %d, touch_raw_y: %d, p.z: %d\n", touch_raw_x, touch_raw_y, p.z);
    }
    translate_touch_raw();
    return true;
  }
#elif defined(TOUCH_MODULE_ADDR) // TouchLib
  if (touch.read())
  {
    TP_Point t = touch.getPoint(0);
    touch_raw_x = t.x;
    touch_raw_y = t.y;

    touch_last_x = touch_raw_x;
    touch_last_y = touch_raw_y;

    translate_touch_raw();
    return true;
  }

#endif // TouchLib

  return false;
}

bool touch_released()
{
#if defined(TOUCH_XPT2046)
  return true;

#elif defined(TOUCH_MODULE_ADDR) // TouchLib
  return false;
#endif                           // TouchLib

  return false;
}
