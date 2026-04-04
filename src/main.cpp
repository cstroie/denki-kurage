#include "config.h"
#include "input_handler.h"
#include "jellyfish.h"
#include "math_3d.h"
#include "types.h"
#include <Arduino.h>
#include <LovyanGFX.hpp>

#ifndef PI
#define PI 3.14159265358979323846
#endif

InputHandler input;

class LGFX_Parallel8Bit : public lgfx::LGFX_Device {
public:
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_Parallel8 _bus_instance;
    lgfx::Light_PWM _light_instance;

    LGFX_Parallel8Bit(void) {
        {
            auto cfg = _bus_instance.config();
            cfg.i2s_port = I2S_NUM_0;
            cfg.freq_write = 20000000;
            cfg.pin_rd = 2;
            cfg.pin_wr = 4;
            cfg.pin_rs = 16;
            cfg.pin_d0 = 15;
            cfg.pin_d1 = 13;
            cfg.pin_d2 = 12;
            cfg.pin_d3 = 14;
            cfg.pin_d4 = 27;
            cfg.pin_d5 = 25;
            cfg.pin_d6 = 33;
            cfg.pin_d7 = 32;
            _bus_instance.config(cfg);
            _panel_instance.bus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs = 17;
            cfg.pin_rst = -1;
            cfg.panel_width = 240;
            cfg.panel_height = 320;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.invert = false;
            cfg.rgb_order = false;
            _panel_instance.config(cfg);
        }
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl = 21;
            cfg.invert = false;
            cfg.freq = 5000;
            cfg.pwm_channel = 0;
            _light_instance.config(cfg);
            _panel_instance.light(&_light_instance);
        }
        setPanel(&_panel_instance);
    }
};

static LGFX_Parallel8Bit tft_local;
static lgfx::LGFX_Sprite canvas_local(&tft_local);
static lgfx::LGFX_Device* tft = &tft_local;
static lgfx::LGFX_Sprite* canvas = &canvas_local;

// Global state
ColorMode current_mode = PURPLE;
float user_y_offset = 0.0f;
float phase = 0;
float global_x_offset = 0.0f;
float global_y_offset = 0.0f;
float global_z_offset = 0.0f;
float angle_x = -0.5f, angle_y = 0.4f, angle_z = 0.0f;
float rotation_speed = 0.005f;
float target_rotation_speed = 0.005f;
unsigned long last_rotation_change = 0;
unsigned long last_frame_time = 0;
float current_fps = 0;

Particle particles[NUM_PARTICLES];
Point2D curr_bell_2d[NUM_BELL_VERTICES];
Point2D curr_tentacles_2d[NUM_TENTACLES][TENTACLE_SEGMENTS];

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("Denki Kurage - Initted");

    pinMode(LED_R_PIN, OUTPUT);
    pinMode(LED_G_PIN, OUTPUT);
    pinMode(LED_B_PIN, OUTPUT);
    digitalWrite(LED_R_PIN, HIGH);
    digitalWrite(LED_G_PIN, HIGH);
    digitalWrite(LED_B_PIN, HIGH);

    tft->init();
    tft->setRotation(0);
#ifdef CYD_INVERT_DISPLAY
    tft->invertDisplay(true);
#endif
    tft->fillScreen(CL_BG);

    canvas->setColorDepth(8);
    if(!canvas->createSprite(SCREEN_WIDTH, SCREEN_HEIGHT)) {
        Serial.println("Sprite FAILED");
    }

    input.begin();
    input.loadSettings(current_mode);

    tft->setBrightness(input.getBrightness());

    for(int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].x = random(0, SCREEN_WIDTH);
        particles[i].y = random(0, SCREEN_HEIGHT);
        particles[i].speed = random(5, 15) / 10.0f;
        particles[i].brightness = random(40, 180);
    }
}

void loop() {
    input.update(current_mode, user_y_offset, angle_y);
    tft->setBrightness(input.getBrightness());

    if(canvas->width() > 0) {
        canvas->fillScreen(CL_BG);
    }

    phase += 0.08f;

    if(millis() - last_rotation_change > 45000) {
        target_rotation_speed = (random(-150, 150) / 10000.0f);
        last_rotation_change = millis();
    }
    rotation_speed += (target_rotation_speed - rotation_speed) * 0.005f;
    angle_y += rotation_speed;

    global_x_offset = sinf(phase * 0.15f) * 35.0f + cosf(phase * 0.25f) * 15.0f;
    global_z_offset = sinf(phase * 0.1f) * 45.0f;
    float drift_y = sinf(phase * 0.4f) * 15.0f + cosf(phase * 0.2f) * 10.0f;
    float lift = cosf(phase) * 12.0f;
    global_y_offset = drift_y - lift + user_y_offset;

    int v_dir = input.getVerticalDir();
    for(int i = 0; i < NUM_PARTICLES; i++) {
        float p_speed = particles[i].speed;
        if(v_dir == -1)
            p_speed *= 4.0f;
        else if(v_dir == 1)
            p_speed *= -2.5f;

        particles[i].y -= p_speed;
        particles[i].x += sinf(phase + i) * 0.3f;

        if(particles[i].y < 0) {
            particles[i].y = SCREEN_HEIGHT;
            particles[i].x = random(0, SCREEN_WIDTH);
        } else if(particles[i].y > SCREEN_HEIGHT) {
            particles[i].y = 0;
            particles[i].x = random(0, SCREEN_WIDTH);
        }

        uint16_t p_color =
            tft->color565(0, particles[i].brightness, particles[i].brightness);
        if(canvas->width() > 0)
            canvas->drawPixel((int)particles[i].x, (int)particles[i].y, p_color);
    }

    updateRotationParams(angle_x, angle_y, angle_z);
    float expansion = 1.0f + sinf(phase) * 0.25f;

    Point3D p3 = {0, -35.0f, 0};
    curr_bell_2d[0] = project(rotateFast(p3), global_x_offset, global_y_offset,
                              global_z_offset);

    for(int r = 0; r < BELL_RINGS; r++) {
        float normalized_r = (float)(r + 1) / (float)BELL_RINGS;
        float ring_y = -20.0f + (float)r * 18.0f;
        float ring_radius = 120.0f * sinf(normalized_r * PI * 0.5f) * expansion;

        for(int i = 0; i < BELL_POINTS_PER_RING; i++) {
            float theta = (float)i * 2.0f * PI / (float)BELL_POINTS_PER_RING;
            p3 = {ring_radius * cosf(theta), ring_y, ring_radius * sinf(theta)};
            curr_bell_2d[1 + r * BELL_POINTS_PER_RING + i] =
                project(rotateFast(p3), global_x_offset, global_y_offset,
                        global_z_offset);
        }
    }

    for(int t = 0; t < NUM_TENTACLES; t++) {
        float theta = (float)t * 2.0f * PI / (float)NUM_TENTACLES;
        float base_x = 35.0f * cosf(theta) * expansion;
        float base_z = 35.0f * sinf(theta) * expansion;
        float base_y = 30.0f;
        for(int s = 0; s < TENTACLE_SEGMENTS; s++) {
            float wave = sinf(phase - (float)s * 0.7f) * 20.0f;
            p3 = {base_x + wave * cosf(theta), base_y + (float)s * 35.0f,
                  base_z + wave * sinf(theta)};
            curr_tentacles_2d[t][s] = project(rotateFast(p3), global_x_offset,
                                              global_y_offset, global_z_offset);
        }
    }

    if(canvas->width() > 0) {
        bool wireframe = input.isWireframeMode();
        drawJellyfish(canvas, curr_bell_2d, curr_tentacles_2d, current_mode,
                      wireframe);

        if(input.isDebugMode()) {
            uint16_t dbg_col = 0xFFE0;
            for(int x = 0; x < SCREEN_WIDTH; x += 4) {
                canvas->drawPixel(x, 45, dbg_col);
                canvas->drawPixel(x, SCREEN_HEIGHT - 45, dbg_col);
            }
            for(int y = 45; y < SCREEN_HEIGHT - 45; y += 4) {
                canvas->drawPixel(80, y, dbg_col);
                canvas->drawPixel(160, y, dbg_col);
            }
            for(int y = 0; y < 45; y += 4)
                canvas->drawPixel(SCREEN_WIDTH - 40, y, dbg_col);
            for(int y = SCREEN_HEIGHT - 45; y < SCREEN_HEIGHT; y += 4)
                canvas->drawPixel(SCREEN_WIDTH - 40, y, dbg_col);

            canvas->setTextColor(dbg_col);
            canvas->setTextSize(1);
            canvas->setTextDatum(MC_DATUM);
            canvas->drawString("T", 100, 22);
            canvas->drawString("TR", 220, 22);
            canvas->drawString("ML", 40, 160);
            canvas->drawString("MC", 120, 160);
            canvas->drawString("MR", 200, 160);
            canvas->drawString("B", 100, 297);
            canvas->drawString("BR", 220, 297);

            canvas->setTextDatum(TL_DATUM);
            canvas->setCursor(5, SCREEN_HEIGHT - 60);
            canvas->printf("FPS: %.1f", current_fps);
            canvas->setCursor(5, SCREEN_HEIGHT - 70);
            canvas->printf("MODE: %s", wireframe ? "Wireframe" : "Solid");
            canvas->setCursor(5, SCREEN_HEIGHT - 80);
            canvas->printf("YAW: %.2f", angle_y);
            canvas->setCursor(5, SCREEN_HEIGHT - 90);
            canvas->printf("Y_OFF: %.0f", user_y_offset);
            canvas->setCursor(5, SCREEN_HEIGHT - 100);
            canvas->printf("BRI: %d", input.getBrightness());
        }

        canvas->pushSprite(0, 0);
    }

    unsigned long now = millis();
    if(now > last_frame_time) {
        current_fps = 1000.0f / (now - last_frame_time);
    }
    last_frame_time = now;

    delay(10);
}