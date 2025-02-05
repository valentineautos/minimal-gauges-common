#include <Arduino.h>
#include <lvgl.h>
#include <stdio.h>

// Color palette
lv_color_t PALETTE_BLACK        = LV_COLOR_MAKE(0, 0, 0);
lv_color_t PALETTE_WHITE        = LV_COLOR_MAKE(255, 255, 255);
lv_color_t PALETTE_GREY         = LV_COLOR_MAKE(90, 90, 90);
lv_color_t PALETTE_DARK_GREY    = LV_COLOR_MAKE(60, 60, 60);
lv_color_t PALETTE_AMBER        = LV_COLOR_MAKE(250, 140, 0);
lv_color_t PALETTE_RED          = LV_COLOR_MAKE(255, 0, 0);

// Meter parts
typedef struct struct_icon_parts {
  int horz_pos;           // Horizontal position
  int vert_pos;           // Vertical position
  int vert_offset;        // Additional if needed for UI balancing when labels are live
  float min;              // Min range value
  float max;              // Max range value
  float alert;            // What value to set alert (-1 if unused)
  float warning;          // What value to set warning  (-1 if unused)
  bool flag_when;         // is state applied ABOVE or BELOW alert / warning value?
  char unit[4];           // eg V, psi, °C
} struct_icon_parts;

// Data from the buttons
typedef struct struct_buttons {
  uint8_t flag;
  uint8_t button;
  uint8_t press_type;
} struct_buttons;

// Data for channel switching
typedef struct struct_set_channel {
  uint8_t flag;
  uint8_t channel_id;
} struct_set_channel;

// Data from the startup ping
typedef struct struct_startup {
  uint8_t flag;
} struct_startup;

int ICON_MOVEMENT              = 20; // How far is the icon moved up when active
int LABEL_LOWER                = 20; // How much lower is the label

// IDs for gauges displays
#define GAUGE_SMALL_SPEEDO     0
#define GAUGE_SMALL_LEVELS     1
#define GAUGE_SMALL_LOCATION   2

// Design elements - change to adjust the UI
bool DO_SPLASH                 = true; // Do the splash screen
uint8_t TICK_WIDTH             = 2;
uint8_t TICK_LENGTH            = 15;
uint8_t TICK_TEXT_OFFSET       = 30;
uint8_t OUTLINE_WIDTH          = 2;
uint8_t NEEDLE_WIDTH           = 5;
int NEEDLE_OFFSET              = -10;
lv_color_t NEEDLE_COLOR        = PALETTE_RED;
uint8_t HALF_METER_TICKS       = 5; // How many ticks on a half (side) meter
char DEFAULT_LABEL[4]          = "---"; // Label text at load

#define STARTUP_OVERRIDE_TIMER 5000 // how long to wait for startup message before forcing

// Switchable attributes
bool is_track_mode;  // true if track mode, false if daily
bool is_show_num;    // true to show numbers and units
uint8_t dimmer_lv;   // dimmed level from 0 - 9

// ESPNow data sources
#define FLAG_CANBUS             0
#define FLAG_GPS                1
#define FLAG_BUTTONS            2
#define FLAG_OIL_PRESSURE       3
#define FLAG_STARTUP            4
#define FLAG_SET_CHANNEL        5
#define FLAG_FUEL               6
#define FLAG_ONLINE             7

// Console button IDs
#define BUTTON_SETTING          0
#define BUTTON_MODE             1
#define BUTTON_BRIGHTNESS_UP    2
#define BUTTON_BRIGHTNESS_DOWN  3

// Button events
#define CLICK_EVENT_CLICK       0
#define CLICK_EVENT_DOUBLE      1
#define CLICK_EVENT_HOLD        2

// Greater or less than for alerts
bool ABOVE = true;
bool BELOW = false;

// Compare current values to alert / warning ranges and return color
lv_color_t get_state_color(struct_icon_parts obj, float value, bool is_icon) {
  // check for -1 initialisation values and return default
  if (value == -1) {
    return (is_show_num || !is_icon) ? PALETTE_WHITE : PALETTE_GREY;
  }
  // check is passed value flags an alert
  if (obj.flag_when == ABOVE) {
    // Check warning first, if defined
    if (obj.warning >= 0 && value > obj.warning) {
        return PALETTE_RED;
    }
    // Check alert if defined
    if (obj.alert >= 0 && value > obj.alert) {
        return PALETTE_AMBER;
    }
  } else if (obj.flag_when == BELOW) {
    // Check warning first, if defined
    if (obj.warning >= 0 && value < obj.warning) {
        return PALETTE_RED;
    }
    // Check alert if defined
    if (obj.alert >= 0 && value < obj.alert) {
        return PALETTE_AMBER;
    }
  }

  // If number showing or not an icon return default white
  if (is_show_num || !is_icon) {
    return PALETTE_WHITE;
  }
  
  // Otherwise, return grey
  return PALETTE_GREY;
}