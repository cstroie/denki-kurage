#include "input_handler.h"

InputHandler::InputHandler()
    : last_btn_state(true), show_debug(false), wireframe_mode(false),
      vertical_dir(0), touched(false), touch_x(0), touch_y(0), brightness_idx(3) {}

void InputHandler::begin() {
    pinMode(BTN_PIN, INPUT_PULLUP);
    Wire.begin(21, 22);
    Wire.setClock(400000);
    prefs.begin("jellyfish", false);
}

bool InputHandler::readCST820Touch() {
    Wire.beginTransmission(0x15);
    Wire.write(0x02);
    if(Wire.endTransmission() != 0) return false;
    Wire.requestFrom(0x15, (uint8_t)6);
    
    if(Wire.available() < 6) return false;
    
    uint8_t status = Wire.read();
    uint8_t xl = Wire.read();
    uint8_t xh = Wire.read();
    uint8_t yl = Wire.read();
    uint8_t yh = Wire.read();
    Wire.read();
    
    if((status & 0x01) == 0) return false;
    
    int raw_x = (xh << 8) | xl;
    int raw_y = (yh << 8) | yl;
    
    touch_y = map(raw_y, 5000, 55000, SCREEN_HEIGHT, 0);
    touch_x = map(raw_x, 2500, 58000, 0, SCREEN_WIDTH);
    touch_x = constrain(touch_x, 0, SCREEN_WIDTH - 1);
    touch_y = constrain(touch_y, 0, SCREEN_HEIGHT - 1);

    Serial.printf("Raw: %d, %d\n", raw_x, raw_y);
    Serial.printf("Touch at: %d, %d\n", touch_x, touch_y);
    
    return true;
}

void InputHandler::loadSettings(ColorMode &mode) {
    mode = (ColorMode)prefs.getInt("color_mode", (int)PURPLE);
    wireframe_mode = prefs.getBool("wire_mode", false);
    brightness_idx = prefs.getUChar("bright_idx", 3);
}

uint8_t InputHandler::getBrightness() const {
    const uint8_t levels[] = {40, 100, 180, 255};
    return levels[brightness_idx % 4];
}

void InputHandler::update(ColorMode &mode, float &user_y_offset,
                          float &angle_y) {
    bool btn_state = digitalRead(BTN_PIN);
    bool touch_trigger = false;
    touched = readCST820Touch();

    vertical_dir = 0;

    if(touched) {
        int tx = touch_x;
        int ty = touch_y;
        
        if(tx > SCREEN_WIDTH - 40) {
            if(ty < 45) {
                if(!last_touch_state) {
                    wireframe_mode = !wireframe_mode;
                    prefs.putBool("wire_mode", wireframe_mode);
                }
            } else if(ty > SCREEN_HEIGHT - 45) {
                if(!last_touch_state)
                    show_debug = !show_debug;
            }
        }

        if(tx <= SCREEN_WIDTH - 40) {
            if(ty < 45) {
                user_y_offset -= 4.0f;
                vertical_dir = -1;
            } else if(ty > SCREEN_HEIGHT - 45) {
                user_y_offset += 4.0f;
                vertical_dir = 1;
            }
        }

        if(ty >= 45 && ty <= SCREEN_HEIGHT - 45) {
            if(tx < 80) {
                angle_y -= 0.04f;
            } else if(tx > SCREEN_WIDTH - 80) {
                angle_y += 0.04f;
            }
            else if(tx >= 80 && tx <= SCREEN_WIDTH - 80) {
                if(!last_touch_state)
                    touch_trigger = true;
            }
        }

        if(user_y_offset < -220.0f)
            user_y_offset = -220.0f;
        if(user_y_offset > 220.0f)
            user_y_offset = 220.0f;
    }

    if(btn_state == LOW && last_btn_state == HIGH) {
        brightness_idx = (brightness_idx + 1) % 4;
        prefs.putUChar("bright_idx", brightness_idx);
    }

    if(touch_trigger) {
        mode = (ColorMode)((mode + 1) % NUM_MODES);
        prefs.putInt("color_mode", (int)mode);
    }

    last_btn_state = btn_state;
    last_touch_state = touched;
}
