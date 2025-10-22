#define NOMINMAX
#include <Windows.h>
#include <mmsystem.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <limits>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../rsc/resource.h"
#include "../third_party/json.hpp"

#pragma comment(lib, "winmm.lib")

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using json = nlohmann::json;
using namespace std::string_literals;

namespace game {
namespace utils {
template <typename T> constexpr T round_to(T value, int decimals = 1) {
  T scale = std::pow(static_cast<T>(10), decimals);
  return std::round(value * scale) / scale;
}

template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
static inline T clamp(T min, T value, T max) {
  if (value < min)
    return min;
  if (value > max)
    return max;
  return value;
}
} // namespace utils

namespace render {
struct RenderState {
  void *memory = nullptr;
  i32 width = 0;
  i32 height = 0;
  BITMAPINFO bitmap_info = {};
};

struct Renderer {
  RenderState render_state;

  void clear_screen(u32 color) {
    if (!render_state.memory)
      return;
    u32 *pixels = static_cast<u32 *>(render_state.memory);
    std::fill_n(pixels,
                static_cast<size_t>(render_state.width * render_state.height),
                color);
  }

  void render_rect_pixels(i32 x0, i32 y0, i32 x1, i32 y1, u32 color) {
    x0 = utils::clamp(0, x0, render_state.width);
    x1 = utils::clamp(0, x1, render_state.width);
    y0 = utils::clamp(0, y0, render_state.height);
    y1 = utils::clamp(0, y1, render_state.height);

    if (!render_state.memory)
      return;

    for (i32 y = y0; y < y1; y++) {
      u32 *pixel =
          static_cast<u32 *>(render_state.memory) + x0 + y * render_state.width;
      for (i32 x = x0; x < x1; x++)
        *pixel++ = color;
    }
  }

  void render_rect(float x, float y, float half_x, float half_y, u32 color) {
    if (render_state.height == 0 || render_state.width == 0)
      return;
    float scale = render_state.height * 0.01f;
    x *= scale;
    y *= scale;
    half_x *= scale;
    half_y *= scale;
    x += render_state.width * 0.5f;
    y += render_state.height * 0.5f;

    i32 x0 = static_cast<i32>(std::lroundf(x - half_x));
    i32 x1 = static_cast<i32>(std::lroundf(x + half_x));
    i32 y0 = static_cast<i32>(std::lroundf(y - half_y));
    i32 y1 = static_cast<i32>(std::lroundf(y + half_y));

    render_rect_pixels(x0, y0, x1, y1, color);
  }

  static const u8 FONT_5x7[][7];

  void render_glyph_5x7(char c, float cx, float cy, float pixel_size,
                        u32 color) {
    if (render_state.height == 0 || render_state.width == 0)
      return;
    if (c < ' ' || c > 'Z')
      c = ' ';
    int idx = 0;
    if (c >= '0' && c <= '9') {
      idx = (c - '0') + 0; // digits at 0..9
    } else if (c >= 'A' && c <= 'Z') {
      idx = 10 + (c - 'A'); // letters after digits
    } else if (c == '%') {
      idx = 37;
    } else if (c == ':') {
      idx = 38;
    } else if (c == '-') {
      idx = 39;
    } else if (c == '!') {
      idx = 40;
    } else {
      idx = 36;
    }
    const u8 *glyph = FONT_5x7[idx];

    const int W = 5;
    const int H = 7;
    float total_w = W * pixel_size;
    float total_h = H * pixel_size;
    float start_x = cx - total_w * 0.5f + pixel_size * 0.5f;
    float start_y = cy - total_h * 0.5f + pixel_size * 0.5f;

    for (int ry = 0; ry < H; ++ry) {
      u8 row = glyph[ry];
      for (int rx = 0; rx < W; ++rx) {
        bool on = (row >> (W - 1 - rx)) & 1;
        if (on) {
          float px = start_x + rx * pixel_size;
          float py = start_y + ry * pixel_size;
          render_rect(px, py, pixel_size * 0.5f, pixel_size * 0.5f, color);
        }
      }
    }
  }

  void render_text(const std::string &s, float cx, float cy, float pixel_size,
                   float spacing, u32 color) {
    if (s.empty())
      return;
    float glyph_w = 5.0f * pixel_size;
    float total = static_cast<float>(s.size()) * glyph_w +
                  (static_cast<float>(s.size() - 1) * spacing);
    float start_x = cx - total * 0.5f + glyph_w * 0.5f;
    for (size_t i = 0; i < s.size(); ++i) {
      float gx = start_x + static_cast<float>(i) * (glyph_w + spacing);
      render_glyph_5x7(s[i], gx, cy, pixel_size, color);
    }
  }
};

const u8 Renderer::FONT_5x7[][7] = {
    // digits '0'..'9' (indexes 0..9)
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, // 0
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, // 1
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}, // 2
    {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E}, // 3
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, // 4
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}, // 5
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, // 6
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, // 7
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, // 8
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}, // 9

    // letters 'A'..'Z' (indexes 10..35)
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // A
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}, // B
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}, // C
    {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C}, // D
    {0x1F, 0x10, 0x10, 0x1C, 0x10, 0x10, 0x1F}, // E
    {0x1F, 0x10, 0x10, 0x1C, 0x10, 0x10, 0x10}, // F
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}, // G
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // H
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, // I
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}, // J
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, // K
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}, // L
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}, // M
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}, // N
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // O
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}, // P
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}, // Q
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}, // R
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}, // S
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, // T
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // U
    {0x11, 0x11, 0x11, 0x0A, 0x0A, 0x04, 0x04}, // V
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}, // W
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}, // X
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}, // Y
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}, // Z

    // space
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

    // '%'
    {0x19, 0x1A, 0x04, 0x08, 0x13, 0x03, 0x00},

    // ':'
    {0x00, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00},

    // '-'
    {0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00},

    // '!'
    {0x04, 0x04, 0x04, 0x04, 0x00, 0x04, 0x00}};
} // namespace render

namespace input {
struct ButtonState {
  bool is_down = false;
  bool changed = false;
};

enum Key {
  BUTTON_LEFT_ARROW,
  BUTTON_UP_ARROW,
  BUTTON_RIGHT_ARROW,
  BUTTON_DOWN_ARROW,

  BUTTON_LEFT,
  BUTTON_UP,
  BUTTON_RIGHT,
  BUTTON_DOWN,

  BUTTON_ENTER,
  BUTTON_F11,
  BUTTON_PAUSE,
  BUTTON_ESC,

  BUTTON_COUNT
};

ButtonState buttons[BUTTON_COUNT] = {};
std::unordered_map<i32, Key> kb = {
    {0x25, BUTTON_LEFT_ARROW},  {0x26, BUTTON_UP_ARROW},
    {0x27, BUTTON_RIGHT_ARROW}, {0x28, BUTTON_DOWN_ARROW},

    {0x5A, input::BUTTON_UP},   {0x53, input::BUTTON_DOWN},
    {0x51, input::BUTTON_LEFT}, {0x44, input::BUTTON_RIGHT},

    {0x0D, BUTTON_ENTER},       {0x7A, BUTTON_F11},
    {0x50, BUTTON_PAUSE},       {0x1B, BUTTON_ESC}};

inline bool is_changed(Key key) { return buttons[key].changed; }
inline bool is_down(Key key) { return buttons[key].is_down; }
inline bool is_pressed(Key key) { return is_down(key) && is_changed(key); }
inline bool is_released(Key key) { return !is_down(key) && is_changed(key); }

inline void process_button(MSG message) {
  auto it = kb.find(static_cast<i32>(message.wParam));
  if (it != kb.end()) {
    Key k = it->second;
    buttons[k].is_down = (message.message == WM_KEYDOWN);
    buttons[k].changed = true;
  }
}
} // namespace input

namespace audio {

bool enabled = false;
bool initialized = false;

float music_volume = 1.0f;
float sfx_volume = 1.0f;

ma_engine engine;
ma_sound music;

struct SoundPool {
  std::vector<ma_sound *> sounds;
  size_t next = 0;

  float cooldown = 0.105f;
  float timer = 0.0f;

  void update(float dt) { timer += dt; }

  void play() {
    if (timer < cooldown)
      return;

    if (sounds.empty())
      return;

    ma_sound *s = sounds[next];
    ma_sound_stop(s);
    ma_sound_seek_to_pcm_frame(s, 0);
    ma_sound_start(s);

    next = (next + 1) % sounds.size();
    timer = 0.0f;
  }

  void set_volume(float vol) {
    for (auto *s : sounds)
      ma_sound_set_volume(s, vol);
  }
};

std::array<std::string, 10> sound_filenames = {
    "navigation.mp3",  "paddle_hit.mp3",     "setting.mp3",        "button.mp3",
    "button_back.mp3", "go_tick.mp3",        "countdown_tick.mp3", "shine.mp3",
    "winner.mp3",      "game_timer_tick.mp3"};

std::unordered_map<std::string, SoundPool> sfx_sounds;

void load_sfx(const std::string &filename) {
  SoundPool pool;
  for (int i = 0; i < 3; ++i) {
    ma_sound *s = new ma_sound;
    if (ma_sound_init_from_file(&engine,
                                std::format("assets/sfx/{}", filename).c_str(),
                                0, nullptr, nullptr, s) == MA_SUCCESS) {
      ma_sound_set_volume(s, sfx_volume);
      pool.sounds.push_back(s);
    } else {
      delete s;
    }
  }

  sfx_sounds[filename] = std::move(pool);
}

void update(float dt) {
  if (!initialized)
    return;

  for (auto &[_, pool] : sfx_sounds)
    pool.update(dt);
}

void update_music_volume() {
  if (!initialized)
    return;
  ma_sound_set_volume(&music, music_volume);
}

void update_sfx_volume() {
  if (!initialized)
    return;

  for (auto &[_, pool] : sfx_sounds)
    pool.set_volume(sfx_volume);
}

void play_effect(const std::string &filename) {
  if (!initialized)
    return;

  auto it = sfx_sounds.find(filename);
  if (it != sfx_sounds.end())
    it->second.play();
}

void set_enabled(bool state) {
  enabled = state;
  if (!initialized)
    return;

  if (enabled)
    ma_sound_start(&music);
  else
    ma_sound_stop(&music);
}

void cleanup() {
  if (!initialized)
    return;

  for (auto &[_, pool] : sfx_sounds) {
    for (auto *s : pool.sounds) {
      ma_sound_uninit(s);
      delete s;
    }
    pool.sounds.clear();
  }
  sfx_sounds.clear();

  ma_sound_uninit(&music);
  ma_engine_uninit(&engine);

  initialized = false;
}

void init() {
  if (initialized)
    return;

  if (ma_engine_init(nullptr, &engine) != MA_SUCCESS)
    return;

  for (auto &fn : sound_filenames)
    load_sfx(fn);

  if (ma_sound_init_from_file(&engine, "assets/music/music.mp3", 0, nullptr,
                              nullptr, &music) == MA_SUCCESS) {
    update_music_volume();
    ma_sound_set_looping(&music, MA_TRUE);
    ma_sound_set_volume(&music, music_volume);
    ma_sound_start(&music);
  }

  initialized = true;
}
} // namespace audio

namespace objects {

enum AIDifficulty { Easy = 0, Medium = 1, Hard = 2, VeryHard, Unbeatable };

struct Vector2 {
  float x;
  float y;
};

struct Dimensions {
  i16 x;
  i16 y;
  u16 width;
  u16 height;

  Dimensions(i16 x = 0, i16 y = 0, u16 width = 1080, u16 height = 720)
      : x(x), y(y), width(width), height(height) {}
};

struct PlayerController {
  Vector2 pos;
  float dp;
  float ddp_speed;
  float damping;

  void init(float x, float _damping) {
    pos.x = x;
    pos.y = 0.0f;
    dp = 0.0f;
    ddp_speed = 1400;
    damping = _damping;
  }

  void update_pos_y(float dt, float ddp) {
    pos.y = pos.y + dp * dt + ddp * dt * dt * 0.5f;
  }
  void update_dp(float dt, float ddp) { dp = dp + ddp * dt; }
  void update_ddp_damping(float &ddp) { ddp -= dp * damping; }

  void update(float dt, float &ddp) {
    update_ddp_damping(ddp);
    update_pos_y(dt, ddp);
    update_dp(dt, ddp);
  }

  void move_left(float &ddp) { ddp -= ddp_speed; }
  void move_up(float &ddp) { ddp -= ddp_speed; }
  void move_right(float &ddp) { ddp += ddp_speed; }
  void move_down(float &ddp) { ddp += ddp_speed; }
};

struct Player {
  Player(render::Renderer &renderer, bool arrow_controls_)
      : renderer(&renderer), arrow_controls(arrow_controls_) {}

  render::Renderer *renderer;
  PlayerController controller;
  bool arrow_controls;
  bool ai_mode = true;

  float pulse_timer;
  float width;
  float height;

  u32 color;
  u32 score = 0;

  void increment_score() { score++; }

  void handle_input(float dt, float &ddp) {
    if (input::is_down(arrow_controls ? input::BUTTON_UP_ARROW
                                      : input::BUTTON_UP))
      controller.move_up(ddp);
    if (input::is_down(arrow_controls ? input::BUTTON_DOWN_ARROW
                                      : input::BUTTON_DOWN))
      controller.move_down(ddp);
  }

  void predict_ball(Player &self, Vector2 &ball_vel, Vector2 &ball_pos,
                    float &paddle_x, float &target_y, u8 level) {
    const float TOP = 50.0f;
    const float BOTTOM = -50.0f;
    const float HIGH_VY = 100.0f;

    bool will_predict = (fabsf(ball_vel.y) > HIGH_VY) &&
                        (fabsf(ball_vel.x) > 1e-4f) &&
                        ((paddle_x >= 0.0f && ball_vel.x > 0.0f) ||
                         (paddle_x < 0.0f && ball_vel.x < 0.0f));

    if (will_predict) {
      float t = (paddle_x - ball_pos.x) / ball_vel.x;
      if (t > 0.0f) {
        float proj_y = ball_pos.y + ball_vel.y * t;

        float span = TOP - BOTTOM;
        float shifted = proj_y - BOTTOM;
        float cycle = std::fmod(shifted, 2.0f * span);
        if (cycle < 0.0f)
          cycle += 2.0f * span;
        float final_y =
            (cycle <= span) ? (BOTTOM + cycle) : (TOP - (cycle - span));

        switch (level) {
        case 1: {
          float inacurracy =
              (static_cast<float>(rand()) / RAND_MAX) * 12.0f - 16.0f;
          final_y += inacurracy;

          if ((rand() % 15) == 0) {
            final_y = TOP;
          }

          target_y = final_y + inacurracy;
          break;
        }
        case 2: {
          float inacurracy =
              (static_cast<float>(rand()) / RAND_MAX) * 12.0f - 10.0f;
          final_y += inacurracy;

          if ((rand() % 5) == 0) {
            final_y = TOP;
          }

          target_y = final_y + inacurracy;
          break;
        }
        case 3: {
          target_y = final_y + 10.0f;
          break;
        }
        }
      } else {
        target_y = ball_pos.y;
      }
    }
  }

  void smash_ball(float &dist_x, float &target_y, float &paddle_y,
                  Vector2 &ball_vel, PlayerController &controller, float &ddp) {
    if (dist_x <= 10.0f) {
      float diff = target_y - paddle_y;
      if (fabsf(diff) < 1.0f) {
        if (ball_vel.y > 0.0f) {
          controller.move_down(ddp);
        } else {
          controller.move_up(ddp);
        }
      } else {
        int dir = (diff > 0.0f) ? 1 : -1;
        if (dir > 0)
          controller.move_down(ddp);
        else
          controller.move_up(ddp);
      }
      return;
    }
  }

  void follow_ball(float &target_y, float &paddle_y,
                   PlayerController &controller, float &ddp) {
    float diff = target_y - paddle_y;
    if (diff > 5.0f) {
      controller.move_down(ddp);
    } else if (diff < -5.0f) {
      controller.move_up(ddp);
    }
  }

  void run_ai_mode(Player &self, Vector2 &ball_pos, Vector2 &ball_vel,
                   float &ddp, AIDifficulty difficulty) {

    if (!((self.controller.pos.x > 0.0f && ball_pos.x > 0.0f) ||
          (self.controller.pos.x < 0.0f && ball_pos.x < 0.0f))) {
      return;
    }

    float paddle_x = self.controller.pos.x;
    float paddle_y = self.controller.pos.y;
    float dist_x = fabsf(paddle_x - ball_pos.x);
    float target_y = ball_pos.y;

    switch (difficulty) {
    case Easy: {
      follow_ball(target_y, paddle_y, controller, ddp);
      break;
    }
    case Medium: {
      smash_ball(dist_x, target_y, paddle_y, ball_vel, controller, ddp);
      follow_ball(target_y, paddle_y, controller, ddp);
      break;
    }
    case Hard: {
      predict_ball(self, ball_vel, ball_pos, paddle_x, target_y, 1);
      smash_ball(dist_x, target_y, paddle_y, ball_vel, controller, ddp);
      follow_ball(target_y, paddle_y, controller, ddp);
      break;
    }
    case VeryHard: {
      predict_ball(self, ball_vel, ball_pos, paddle_x, target_y, 2);
      smash_ball(dist_x, target_y, paddle_y, ball_vel, controller, ddp);
      follow_ball(target_y, paddle_y, controller, ddp);
      break;
    }
    case Unbeatable: {
      predict_ball(self, ball_vel, ball_pos, paddle_x, target_y, 3);
      smash_ball(dist_x, target_y, paddle_y, ball_vel, controller, ddp);
      follow_ball(target_y, paddle_y, controller, ddp);
      break;
    };
    }
  }

  void init(float x, bool ai_mode_, float &speed, float &damping) {
    controller.init(x, damping * 9.0f);
    controller.ddp_speed = speed * 1700.0f;
    ai_mode = ai_mode_;
    width = 2.0f;
    height = 12.0f;
    color = arrow_controls ? 0x004DABF7 : 0x00FF6B6B;
  }

  void reset() {
    controller.pos.y = 0.0f;
    controller.dp = 0.0f;
  }

  void add_collision() {
    if (controller.pos.y + height > 50.0f) {
      controller.pos.y = 50.0f - height;
      controller.dp *= -0.5f;
    }

    if (controller.pos.y - height < -50.0f) {
      controller.pos.y = -50.0f + height;
      controller.dp *= -0.5f;
    }
  }

  u32 lighten_color(u32 c, float intensity) {
    u8 r = (c >> 16) & 0xFF;
    u8 g = (c >> 8) & 0xFF;
    u8 b = c & 0xFF;

    r = static_cast<u8>(r + (255 - r) * intensity);
    g = static_cast<u8>(g + (255 - g) * intensity);
    b = static_cast<u8>(b + (255 - b) * intensity);

    return (r << 16) | (g << 8) | b;
  }

  void update(float dt, Vector2 &ball_pos, Vector2 &ball_vel,
              AIDifficulty difficulty = Medium) {
    float ddp = 0.0f;

    if (ai_mode)
      run_ai_mode(*this, ball_pos, ball_vel, ddp, difficulty);
    else
      handle_input(dt, ddp);
    controller.update(dt, ddp);
    add_collision();

    if (pulse_timer > 0.0f) {
      pulse_timer -= dt;
      if (pulse_timer < 0.0f)
        pulse_timer = 0.0f;
    }

    u32 base_color = color;
    u32 pulse_color = 0xFFFF80;

    float t = pulse_timer / 0.5f;
    u32 final_color = (t > 0.0f) ? lighten_color(color, t) : color;

    renderer->render_rect(controller.pos.x, controller.pos.y, width, height,
                          final_color);
  }
};

struct Ball;

struct BallController {
  Vector2 pos;
  Player *player1 = nullptr;
  Player *player2 = nullptr;

  Vector2 vel;
  float size = 1.2f;

  bool scored = false;
  int winner = 0;

  void init(Player *player1, Player *player2) {
    pos.x = 0.0f;
    pos.y = 0.0f;

    vel.x = 95.0f;
    vel.y = 0.0f;

    size = 1.2f;

    this->player1 = player1;
    this->player2 = player2;

    scored = false;
    winner = 0;
  }

  void add_collision(Player *player, Ball &ball);
  void update_physics(float dt);
  void update(float dt, Ball &ball);
};

struct Particle {
  Vector2 pos;
  Vector2 vel;
  float life;
};

struct ParticleBurst {
  std::vector<Particle> particles;
  bool active = false;
  render::Renderer *renderer = nullptr;

  int count = 80;
  float lifetime = 1.0f;

  Vector2 pos;

  float speed_min = 30.0f;
  float speed_max = 80.0f;

  ParticleBurst() = default;
  ParticleBurst(render::Renderer &r) : renderer(&r) {}

  void start(float x, float y) {
    if (!renderer)
      return;

    pos.x = x;
    pos.y = y;

    particles.clear();
    active = true;
    particles.reserve(count);

    for (int i = 0; i < count; i++) {
      float angle = ((float)std::rand() / RAND_MAX) * 2.0f * 3.14159265f;
      float speed =
          speed_min + ((float)std::rand() / RAND_MAX) * (speed_max - speed_min);
      particles.push_back(
          {x, y, std::cos(angle) * speed, std::sin(angle) * speed,
           lifetime * (0.5f + ((float)std::rand() / RAND_MAX) * 0.5f)});
    }
  }

  void update(float dt) {
    if (!active)
      return;
    for (auto &p : particles) {
      p.pos.x += p.vel.x * dt;
      p.pos.y += p.vel.y * dt;
      p.life -= dt;
    }
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
                       [](const Particle &p) { return p.life <= 0.0f; }),
        particles.end());
    if (particles.empty())
      active = false;
  }

  void render() {
    if (!active || !renderer)
      return;
    for (auto &p : particles) {
      float a = utils::clamp(0.0f, p.life / lifetime, 1.0f);
      u8 r = static_cast<u8>((pos.x > 0.0f ? 77 : 255) * a);
      u8 g = static_cast<u8>((pos.x > 0.0f ? 171 : 107) * a);
      u8 b = static_cast<u8>((pos.x > 0.0f ? 247 : 107) * a);
      u32 color = (r << 16) | (g << 8) | b;

      renderer->render_rect(p.pos.x, p.pos.y, 1.0f, 1.0f, color);
    }
  }

  bool finished() const { return !active; }
};

struct FlashEffect {
  render::Renderer *renderer = nullptr;
  float alpha = 0.0f;
  bool active = false;
  float fade_speed = 3.0f;

  FlashEffect() = default;
  FlashEffect(render::Renderer &r) : renderer(&r) {}

  void start() {
    alpha = 1.0f;
    active = true;
  }

  void update(float dt) {
    if (!active)
      return;
    alpha -= dt * fade_speed;
    if (alpha <= 0.0f) {
      alpha = 0.0f;
      active = false;
    }
  }

  void render() {
    if (!active || !renderer)
      return;
    u8 a = (u8)(utils::clamp(0.0f, alpha, 1.0f) * 255.0f);
    u8 r = (u8)(255 * (0.3f + 0.7f * alpha));
    u8 g = r;
    u8 b = r;
    u32 color = (r << 16) | (g << 8) | b;
    renderer->render_rect(0.0f, 0.0f, 100.0f, 100.0f, color);
  }

  bool finished() const { return !active; }
};

struct Ball {
  render::Renderer *renderer = nullptr;
  BallController controller;
  u32 color = 0x0000FFFF;

  Ball() = default;
  Ball(render::Renderer &renderer_) : renderer(&renderer_) {}

  void init(Player &player1, Player &player2, float &speed) {
    controller.init(&player1, &player2);
    controller.vel.x = speed * 100.0f;
    color = 0x0000FFFF;
  }

  void reset() {
    controller.pos.x = 0.0f;
    controller.pos.y = 0.0f;

    controller.vel.x = -controller.vel.x;
    controller.vel.y = 0.0f;

    controller.scored = false;
    controller.winner = 0;
  }

  void render() {
    if (!renderer)
      return;
    renderer->render_rect(controller.pos.x, controller.pos.y, controller.size,
                          controller.size, color);
  }

  void update(float dt) {
    controller.update(dt, *this);
    render();

    renderer->render_text(std::to_string(controller.player1->score), -10.0f,
                          40.0f, 0.7f, 0.7f, 0xbbffbb);
    renderer->render_text(std::to_string(controller.player2->score), 10.0f,
                          40.0f, 0.7f, 0.7f, 0xbbffbb);
  }
};

inline void BallController::add_collision(Player *player, Ball &ball) {
  if (!player)
    return;

  if (pos.y + size > 50.0f) {
    pos.y = 50.0f - size;
    vel.y = -vel.y;
  }

  if (pos.y - size < -50.0f) {
    pos.y = -50.0f + size;
    vel.y = -vel.y;
  }

  if (pos.x + size > 80.0f) {
    scored = true;
    winner = 1;
    pos.x = 80.0f + size;
    audio::play_effect("shine.mp3");
    player1->increment_score();
    return;
  }

  if (pos.x - size < -80.0f) {
    scored = true;
    winner = 2;
    pos.x = -80.0f - size;
    audio::play_effect("shine.mp3");
    player2->increment_score();
    return;
  }

  float px = player->controller.pos.x;
  float py = player->controller.pos.y;
  float width = player->width;
  float height = player->height;
  float dp = player->controller.dp;

  bool overlap_x = (fabsf(pos.x - px) <= (width + size));
  bool overlap_y = (fabsf(pos.y - py) <= (height + size));

  if (overlap_x && overlap_y) {
    if (vel.x < 0.0f) {
      pos.x = px + width + size;
    } else {
      pos.x = px - width - size;
    }

    player->pulse_timer = 0.5f;

    vel.x = -vel.x + 0.0001f;

    float hit = (pos.y - py) / height;
    float hit_influence = hit * 38.0f;
    float paddle_influence = dp * 0.20f;

    vel.y += hit_influence + paddle_influence + 0.0001f;
    audio::play_effect("paddle_hit.mp3");
  }
}

inline void BallController::update_physics(float dt) {
  pos.x += vel.x * dt;
  pos.y += vel.y * dt;
}

inline void BallController::update(float dt, Ball &ball) {
  if (scored)
    return;

  update_physics(dt);

  if (player1)
    add_collision(player1, ball);
  if (scored)
    return;
  if (player2)
    add_collision(player2, ball);
}
} // namespace objects

struct World {
  World(render::Renderer &renderer) : renderer(&renderer) { total_time = 0.0f; }

  render::Renderer *renderer;
  float total_time = 0.0f;

  void draw_background(float elapsed_time) {
    float pulse = 0.5f + 0.5f * sinf(elapsed_time * 0.5f);

    u8 base_r = 0x30;
    u8 base_g = 0x30;
    u8 base_b = 0x50;

    float brightness = 0.4f + 0.4f * pulse;
    u8 r = (u8)utils::clamp(0.0f, base_r * brightness, 255.0f);
    u8 g = (u8)utils::clamp(0.0f, base_g * brightness, 255.0f);
    u8 b = (u8)utils::clamp(0.0f, base_b * brightness, 255.0f);

    u32 bg_color = (r << 16) | (g << 8) | b;
    renderer->clear_screen(bg_color);
  }

  void draw_scanline_bands(float elapsed_time) {
    float offset = sinf(elapsed_time * 0.5f) * 20.0f;
    for (int i = 0; i <= 10; i++) {
      u32 band_color = (i % 2 == 0) ? 0x00282838 : 0x00202030;
      renderer->render_rect(0, (i - 5) * 20.0f + offset, 60.f, 10.0f,
                            band_color);
    }
  }

  void draw_light_sweep(float elapsed_time) {
    float pulse = 0.5f + 0.5f * sinf(elapsed_time * 2.0f);
    u8 pulse_intensity = (u8)(80 + 100 * pulse);
    u32 grid_color = (pulse_intensity << 16) | (pulse_intensity << 8) | 255;

    for (int i = -50; i <= 50; i += 10) {
      renderer->render_rect(0.0f, (float)i, 0.5f, 4.0f, grid_color);
    }
  }

  void draw(float dt) {
    total_time += dt;
    draw_background(total_time);
    draw_scanline_bands(total_time);
    draw_light_sweep(total_time);
  }

  void draw_simple(float dt) {
    total_time += dt;
    draw_background(total_time);
    draw_scanline_bands(total_time);
  }
};

struct Config {
  std::string filename;
  json data;

  float paddle_speed = 1.0f;
  float paddle_damping = 1.0f;
  float ball_speed = 1.4f;
  float game_duration_secs = 30.0f;
  objects::AIDifficulty ai_difficulty = static_cast<objects::AIDifficulty>(1);

  Config(const std::string &filename_) : filename(filename_) {
    data = json::object();
    data["settings"] = {{"paddle_speed", paddle_speed},
                        {"paddle_friction", paddle_damping},
                        {"ball_speed", ball_speed},
                        {"ai_difficulty", static_cast<int>(ai_difficulty)},
                        {"music_enabled", audio::enabled},
                        {"music_volume", audio::music_volume},
                        {"sfx_volume", audio::sfx_volume},
                        {"game_duration_secs", game_duration_secs}};
  }

  void init() {
    using namespace std::filesystem;
    if (exists(filename)) {
      try {
        load_from_file(filename);
      } catch (...) {
        sync_json_from_members();
        save_to_file(filename);
      }
    } else {
      path p = path(filename).parent_path();
      if (!p.empty() && !exists(p))
        create_directories(p);

      if (exists("config/default.json")) {
        try {
          load_from_file("config/default.json");
        } catch (...) {
          sync_json_from_members();
        }
      }
      save_to_file(filename);
    }
  }

  void save_to_file(const std::string &filename_) {
    paddle_speed = utils::round_to(paddle_speed, 1);
    paddle_damping = utils::round_to(paddle_damping, 1);
    ball_speed = utils::round_to(ball_speed, 1);
    audio::music_volume = utils::round_to(audio::music_volume, 1);
    audio::sfx_volume = utils::round_to(audio::sfx_volume, 1);

    data["settings"]["paddle_speed"] = paddle_speed;
    data["settings"]["paddle_friction"] = paddle_damping;
    data["settings"]["ball_speed"] = ball_speed;
    data["settings"]["ai_difficulty"] = static_cast<int>(ai_difficulty);
    data["settings"]["music_enabled"] = audio::enabled;
    data["settings"]["music_volume"] = audio::music_volume;
    data["settings"]["sfx_volume"] = audio::sfx_volume;
    data["settings"]["game_duration_secs"] = game_duration_secs;

    std::ofstream file(filename_, std::ios::trunc);
    if (!file) {
      return;
    }
    file << data.dump(4);
  }

  void set_paddle_speed(float value) {
    paddle_speed = utils::round_to(value, 1);
    data["settings"]["paddle_speed"] = paddle_speed;
  }

  void set_paddle_damping(float value) {
    paddle_damping = utils::round_to(value, 1);
    data["settings"]["paddle_friction"] = paddle_damping;
  }

  void set_ball_speed(float value) {
    ball_speed = utils::round_to(value, 1);
    data["settings"]["ball_speed"] = ball_speed;
  }

  void set_ai_difficulty(objects::AIDifficulty value) {
    ai_difficulty = value;
    data["settings"]["ai_difficulty"] = static_cast<int>(ai_difficulty);
  }

  void set_music_enabled(bool enabled) {
    audio::set_enabled(enabled);
    data["settings"]["music_enabled"] = enabled;
  }

  void set_music_volume(float value) {
    audio::music_volume = utils::round_to(value, 1);
    data["settings"]["music_volume"] = audio::music_volume;
  }

  void set_sfx_volume(float value) {
    audio::sfx_volume = utils::round_to(value, 1);
    data["settings"]["sfx_volume"] = audio::sfx_volume;
  }

  void set_game_duration_secs(u16 value) {
    game_duration_secs = value;
    data["settings"]["game_duration_secs"] = value;
  }

  void load_from_file(const std::string &filename_) {
    std::ifstream file(filename_);
    if (!file)
      throw std::runtime_error("Could not open config file");

    json loaded;
    file >> loaded;

    if (!loaded.is_object() || !loaded.contains("settings")) {
      throw std::runtime_error("Invalid config json");
    }

    data = std::move(loaded);
    auto settings = data["settings"];

    if (settings.contains("paddle_speed"))
      paddle_speed = settings["paddle_speed"].get<float>();
    if (settings.contains("paddle_friction"))
      paddle_damping = settings["paddle_friction"].get<float>();
    if (settings.contains("ball_speed"))
      ball_speed = settings["ball_speed"].get<float>();
    if (settings.contains("ai_difficulty"))
      ai_difficulty = static_cast<objects::AIDifficulty>(
          settings["ai_difficulty"].get<int>());
    if (settings.contains("music_enabled"))
      audio::set_enabled(settings["music_enabled"].get<bool>());
    if (settings.contains("music_volume"))
      audio::music_volume = settings["music_volume"].get<float>();
    if (settings.contains("sfx_volume"))
      audio::sfx_volume = settings["sfx_volume"].get<float>();
    if (settings.contains("game_duration_secs"))
      game_duration_secs = settings["game_duration_secs"].get<u16>();

    paddle_speed = utils::round_to(paddle_speed, 1);
    paddle_damping = utils::round_to(paddle_damping, 1);
    ball_speed = utils::round_to(ball_speed, 1);
    audio::music_volume = utils::round_to(audio::music_volume, 1);
    audio::sfx_volume = utils::round_to(audio::sfx_volume, 1);

    sync_json_from_members();
  }

private:
  void sync_json_from_members() {
    data["settings"]["paddle_speed"] = paddle_speed;
    data["settings"]["paddle_friction"] = paddle_damping;
    data["settings"]["ball_speed"] = ball_speed;
    data["settings"]["ai_difficulty"] = static_cast<int>(ai_difficulty);
    data["settings"]["music_enabled"] = audio::enabled;
    data["settings"]["music_volume"] = audio::music_volume;
    data["settings"]["sfx_volume"] = audio::sfx_volume;
    data["settings"]["game_duration_secs"] = game_duration_secs;
  }
};

namespace window {
class Window {
  enum MenuState {
    MENU_MAIN = 0,
    MENU_SETTINGS = 1,
    MENU_PLAYING = 2,
    MENU_BACK = 3
  };

public:
  Window()
      : dimensions{0, 0, 1080, 720}, title("Ping Pong Game"), icon_path(""),
        renderer(), world(renderer), player1(renderer, false),
        player2(renderer, true), ball(renderer), particle_burst(renderer),
        flash() {
    std::srand((unsigned)std::time(nullptr));
  }

  Window(objects::Dimensions dimensions, std::string title,
         std::string icon_path)
      : dimensions(dimensions), title(title), icon_path(icon_path), renderer(),
        world(renderer), player1(renderer, false), player2(renderer, true),
        ball(renderer), particle_burst(renderer), flash(renderer) {
    std::srand((unsigned)std::time(nullptr));
  }

  i16 mainloop() {
    if (!init())
      return 0;

    timeBeginPeriod(1);

    while (running) {
      MSG message;

      for (i32 i = 0; i < input::BUTTON_COUNT; i++)
        input::buttons[i].changed = false;

      while (PeekMessageA(&message, window, 0, 0, PM_REMOVE)) {

        switch (message.message) {
        case WM_KEYDOWN:
        case WM_KEYUP: {
          input::process_button(message);
        } break;

        default:
          TranslateMessage(&message);
          DispatchMessageA(&message);
        }
      }

      if (renderer.render_state.memory && renderer.render_state.width > 0 &&
          renderer.render_state.height > 0) {
        LARGE_INTEGER current_counter;
        QueryPerformanceCounter(&current_counter);
        float dt = (float)(current_counter.QuadPart - last_counter.QuadPart) /
                   (float)frequency.QuadPart;
        last_counter = current_counter;

        audio::update(dt);

        if (input::is_pressed(input::BUTTON_F11))
          toggle_fullscreen();

        if (menu_state == MENU_MAIN) {
          world.draw_simple(dt);

          float title_y = -22.0f;
          float title_size = 1.5f;
          float title_spacing = 0.8f;
          renderer.render_text("PING PONG", 0.0f, title_y, title_size,
                               title_spacing, 0x00FFFFFF);

          const float start_y = 0.0f;
          const float gap = 9.0f;
          for (size_t i = 0; i < menu_items.size(); ++i) {
            float y = start_y + static_cast<float>(i) * gap;
            u32 color = (i == (size_t)menu_index) ? 0x00FFCC66 : 0x00666666;
            renderer.render_rect(0.0f, y, 33.0f, 4.0f, 0x00102030);
            renderer.render_text(menu_items[i], 0.0f, y, 0.6f, 0.6f, color);
            if (i == (size_t)menu_index)
              renderer.render_rect(-29.0f, y, 1.2f, 1.2f, 0x00FFFFFF);
          }

          if (input::is_pressed(input::BUTTON_UP_ARROW) ||
              input::is_pressed(input::BUTTON_UP)) {
            audio::play_effect("navigation.mp3");
            menu_index--;
            if (menu_index < 0)
              menu_index = static_cast<int>(menu_items.size()) - 1;
          }
          if (input::is_pressed(input::BUTTON_DOWN_ARROW) ||
              input::is_pressed(input::BUTTON_DOWN)) {
            audio::play_effect("navigation.mp3");
            menu_index++;
            if (menu_index >= static_cast<int>(menu_items.size()))
              menu_index = 0;
          }
          if (input::is_pressed(input::BUTTON_ENTER)) {
            if (menu_index == 0) {
              player1.init(-70.0f, false, game_config.paddle_speed,
                           game_config.paddle_damping);
              player2.init(70.0f, true, game_config.paddle_speed,
                           game_config.paddle_damping);
              player1.ai_mode = false;
              player2.ai_mode = true;
              player1.score = 0;
              player2.score = 0;

              ball.init(player1, player2, game_config.ball_speed);
              countdown_value = 3;
              countdown_time = 0.0f;
              in_countdown = true;
              menu_state = MENU_PLAYING;
            } else if (menu_index == 1) {
              player1.init(-70.0f, false, game_config.paddle_speed,
                           game_config.paddle_damping);
              player2.init(70.0f, false, game_config.paddle_speed,
                           game_config.paddle_damping);
              player1.ai_mode = false;
              player2.ai_mode = false;
              player1.score = 0;
              player2.score = 0;
              ball.init(player1, player2, game_config.ball_speed);
              countdown_value = 3;
              countdown_time = 0.0f;
              in_countdown = true;
              menu_state = MENU_PLAYING;
            } else if (menu_index == 2) {
              menu_state = MENU_SETTINGS;
            } else if (menu_index == 3) {
              running = false;
            }

            audio::play_effect("button.mp3");
          }
        } else if (menu_state == MENU_SETTINGS) {
          static i32 settings_index = 0;
          static float _ball_speed = game_config.ball_speed;
          static float _paddle_speed = game_config.paddle_speed;
          static float _paddle_damping = game_config.paddle_damping;
          static u16 _game_duration_secs = game_config.game_duration_secs;
          static i32 _ai_difficulty =
              static_cast<i32>(game_config.ai_difficulty);

          std::vector<std::string> setting_labels = {
              "BALL SPEED",    "PADDLE SPEED",  "PADDLE FRICTION",
              "AI DIFFICULTY", "ENABLE MUSIC",  "MUSIC VOLUME",
              "SFX VOLUME",    "GAME DURATION", "BACK"};

          world.draw_simple(dt);
          renderer.render_text("SETTINGS", 0.0f, -42.0f, 1.2f, 0.7f,
                               0x00FFFFFF);

          const float start_y = -30.0f;
          const float gap = 9.0f;

          for (size_t i = 0; i < setting_labels.size(); ++i) {
            float y = start_y + static_cast<float>(i) * gap;
            u32 color = (i == (size_t)settings_index) ? 0x00FFCC66 : 0x00666666;
            renderer.render_rect(0.0f, y, 52.0f, 4.0f, 0x00102030);

            float game_duration = 30.0f;
            std::string label = setting_labels[i];
            std::string value;

            if (label == "BALL SPEED") {
              value = std::format("{:.1f}", _ball_speed);
            } else if (label == "PADDLE SPEED") {
              value = std::format("{:.1f}", _paddle_speed);
            } else if (label == "AI DIFFICULTY") {
              switch (_ai_difficulty) {
              case 0:
                value = "EASY";
                break;
              case 1:
                value = "NORMAL";
                break;
              case 2:
                value = "HARD";
                break;
              case 3:
                value = "VERYHARD";
                break;
              default:
                value = "UNBEATABLE";
                break;
              }
            } else if (label == "PADDLE FRICTION") {
              value = std::format("{:.1f}", _paddle_damping);
            } else if (label == "ENABLE MUSIC") {
              value = audio::enabled ? "ON" : "OFF";
            } else if (label == "MUSIC VOLUME") {
              value = std::format("{}%", static_cast<int>(std::round(
                                             audio::music_volume * 100.0f)));
            } else if (label == "SFX VOLUME") {
              value = std::format("{}%", static_cast<int>(std::round(
                                             audio::sfx_volume * 100.0f)));
            } else if (label == "GAME DURATION") {
              value = std::format("{}S", _game_duration_secs);
            } else {
              value = "";
            }

            renderer.render_text(label, -14.0f, y, 0.6f, 0.6f, color);
            if (!value.empty())
              renderer.render_text(value, 32.0f, y, 0.6f, 0.6f, 0x00AAAAAA);
            if (i == (size_t)settings_index) {
              renderer.render_rect(-44.0f, y, 1.2f, 1.2f, 0x00FFFFFF);
            }
          }

          if (input::is_pressed(input::BUTTON_UP_ARROW) ||
              input::is_pressed(input::BUTTON_UP)) {
            audio::play_effect("navigation.mp3");
            settings_index--;
            if (settings_index < 0)
              settings_index = static_cast<int>(setting_labels.size()) - 1;
          }

          if (input::is_pressed(input::BUTTON_DOWN_ARROW) ||
              input::is_pressed(input::BUTTON_DOWN)) {
            audio::play_effect("navigation.mp3");
            settings_index++;
            if (settings_index >= static_cast<int>(setting_labels.size()))
              settings_index = 0;
          }

          if (input::is_pressed(input::BUTTON_LEFT_ARROW)) {
            audio::play_effect("setting.mp3");
            if (settings_index == 0) {
              _ball_speed =
                  std::max(0.5f, utils::round_to(_ball_speed - 0.1f, 1));
            } else if (settings_index == 1) {
              _paddle_speed =
                  std::max(0.5f, utils::round_to(_paddle_speed - 0.1f, 1));
            } else if (settings_index == 2) {
              _paddle_damping =
                  std::max(0.8f, utils::round_to(_paddle_damping - 0.1f, 1));
            } else if (settings_index == 3) {
              _ai_difficulty = std::max(0, _ai_difficulty - 1);
            } else if (settings_index == 4) {
              game_config.set_music_enabled(false);
            } else if (settings_index == 5) {
              float newVol = std::max(
                  0.0f, utils::round_to(audio::music_volume - 0.1f, 1));
              game_config.set_music_volume(newVol);
              audio::update_music_volume();
            } else if (settings_index == 6) {
              float newVol =
                  std::max(0.0f, utils::round_to(audio::sfx_volume - 0.1f, 1));
              game_config.set_sfx_volume(newVol);
              audio::update_sfx_volume();
            } else if (settings_index == 7) {
              _game_duration_secs = std::max(5, _game_duration_secs - 1);
            }
          }

          if (input::is_pressed(input::BUTTON_RIGHT_ARROW)) {
            audio::play_effect("setting.mp3");
            if (settings_index == 0) {
              _ball_speed =
                  std::min(3.0f, utils::round_to(_ball_speed + 0.1f, 1));
            } else if (settings_index == 1) {
              _paddle_speed =
                  std::min(3.0f, utils::round_to(_paddle_speed + 0.1f, 1));
            } else if (settings_index == 2) {
              _paddle_damping =
                  std::min(2.0f, utils::round_to(_paddle_damping + 0.1f, 1));
            } else if (settings_index == 3) {
              _ai_difficulty = std::min(4, _ai_difficulty + 1);
            } else if (settings_index == 4) {
              game_config.set_music_enabled(true);
            } else if (settings_index == 5) {
              float newVol = std::min(
                  1.0f, utils::round_to(audio::music_volume + 0.1f, 1));
              game_config.set_music_volume(newVol);
              audio::update_music_volume();
            } else if (settings_index == 6) {
              float newVol =
                  std::min(1.0f, utils::round_to(audio::sfx_volume + 0.1f, 1));
              game_config.set_sfx_volume(newVol);
              audio::update_sfx_volume();
            } else if (settings_index == 7) {
              _game_duration_secs = std::min(600, _game_duration_secs + 1);
            }
          }

          if (input::is_pressed(input::BUTTON_ENTER)) {
            if (settings_index == static_cast<int>(setting_labels.size() - 1)) {
              audio::play_effect("button_back.mp3");

              game_config.set_ball_speed(_ball_speed);
              game_config.set_paddle_speed(_paddle_speed);
              game_config.set_paddle_damping(_paddle_damping);
              game_config.set_ai_difficulty(
                  static_cast<objects::AIDifficulty>(_ai_difficulty));
              game_config.set_game_duration_secs(_game_duration_secs);

              game_config.save_to_file("config/config.json");

              menu_state = MENU_MAIN;
            }
          } else {
            game_config.paddle_speed = _paddle_speed;
            game_config.paddle_damping = _paddle_damping;
            game_config.ball_speed = _ball_speed;
            game_config.ai_difficulty =
                static_cast<objects::AIDifficulty>(_ai_difficulty);
            game_config.game_duration_secs = _game_duration_secs;
          }
        } else if (menu_state == MENU_PLAYING) {
          static i8 paused_index = 0;
          std::vector<std::string> paused_items = {"RESUME", "RESTART",
                                                   "MAIN MENU"};

          if (in_countdown) {
            world.draw_simple(dt);
            countdown_time += dt;

            if (countdown_time >= 0.35f) {
              countdown_time = 0.0f;
              countdown_value--;

              if (countdown_value > 0)
                audio::play_effect("countdown_tick.mp3");
              else if (countdown_value == 0)
                audio::play_effect("go_tick.mp3");
            }

            std::string text;
            float color;
            if (countdown_value > 0) {
              text = std::to_string(countdown_value);
              color = 0x00FFFFFF;
            } else {
              text = "GO!";
              color = 0x00FFCC66;
            }

            renderer.render_text(text, 0.0f, 0.0f, 2.0f, 1.0f, color);

            if (countdown_value < 0) {
              in_countdown = false;
              game_running = true;
              paused = false;

              game_timer_active = true;
              game_time_elapsed = 0.0f;
              time_up_state = false;
            }
          }

          else if (in_celebration) {
            celebration_time += dt;
            flash.update(dt);
            particle_burst.update(dt);

            if (flash.finished() && particle_burst.finished()) {
              ball.reset();
              player1.reset();
              player2.reset();
              in_celebration = false;
            }

            world.draw(0.0f);
            player1.update(dt, ball.controller.pos, ball.controller.vel);
            player2.update(dt, ball.controller.pos, ball.controller.vel);
            ball.render();
          }

          else if (!in_celebration) {
            if (confirm_modal) {
              world.draw_simple(dt);
              renderer.render_text("PAUSED", 0.0f, -17.0f, 1.2f, 0.7f,
                                   0x00FFFFFF);

              for (size_t i = 0; i < paused_items.size(); ++i) {
                float y = static_cast<float>(i) * 9.0f;
                u32 color =
                    (i == (size_t)paused_index) ? 0x00FFCC66 : 0x00666666;
                renderer.render_rect(0.0f, y, 33.0f, 4.0f, 0x00102030);
                renderer.render_text(paused_items[i], 0.0f, y, 0.6f, 0.6f,
                                     color);
                if (i == (size_t)paused_index)
                  renderer.render_rect(-29.0f, y, 1.2f, 1.2f, 0x00FFFFFF);
              }

              if (input::is_pressed(input::BUTTON_UP_ARROW) ||
                  input::is_pressed(input::BUTTON_UP)) {
                audio::play_effect("navigation.mp3");
                paused_index = (paused_index - 1 + paused_items.size()) %
                               paused_items.size();
              }

              if (input::is_pressed(input::BUTTON_DOWN_ARROW) ||
                  input::is_pressed(input::BUTTON_DOWN)) {
                audio::play_effect("navigation.mp3");
                paused_index = (paused_index + 1) % paused_items.size();
              }
            } else {
              if (!time_up_state) {
                world.draw(dt);
                player1.update(dt, ball.controller.pos, ball.controller.vel,
                               game_config.ai_difficulty);
                player2.update(dt, ball.controller.pos, ball.controller.vel,
                               game_config.ai_difficulty);
                ball.update(dt);
              } else {
                world.draw_simple(dt);
              }

              if (game_timer_active && !paused && !confirm_modal &&
                  !in_countdown && !time_up_state) {
                game_time_elapsed += dt;

                float time_left = std::max(
                    0.0f, game_config.game_duration_secs - game_time_elapsed);
                int minutes = static_cast<int>(time_left) / 60;
                int seconds = static_cast<int>(time_left) % 60;

                char timer_text[16];
                sprintf(timer_text, "%02d:%02d", minutes, seconds);

                if (minutes == 0 && seconds <= 5) {
                  renderer.render_text(timer_text, 0.0f, -40.0f, 0.8f, 0.8f,
                                       0xFF0000);

                  tick_timer += dt;

                  if (tick_timer >= 1.0f) {
                    tick_timer = 0.0f;
                    audio::play_effect("game_timer_tick.mp3");
                  }
                } else {
                  renderer.render_text(timer_text, 0.0f, -40.0f, 0.8f, 0.8f,
                                       0x00FFFFFF);
                }

                if (game_time_elapsed >= game_config.game_duration_secs) {
                  game_timer_active = false;
                  time_up_state = true;
                  time_up_delay = 0.0f;
                  audio::play_effect("winner.mp3");
                }
              }

              if (time_up_state) {
                std::string winner;
                float color;
                if (player1.score > player2.score) {
                  winner = "PLAYER 1 WINS!";
                  color = player1.color;
                } else if (player2.score > player1.score) {
                  winner = "PLAYER 2 WINS!";
                  color = player2.color;
                } else {
                  winner = "DRAW!";
                  color = 0x00FFCC66;
                }

                renderer.render_text("TIME IS UP!", 0.0f, -10.0f, 0.8f, 0.7f,
                                     0x00FFFFFF);
                renderer.render_text(winner, 0.0f, 0.0f, 1.2f, 0.8f, color);

                time_up_delay += dt;
                if (time_up_delay >= 2.5f) {
                  menu_state = MENU_MAIN;
                  time_up_state = false;
                  continue;
                }
              }

              if (!time_up_state && ball.controller.scored) {
                in_celebration = true;
                celebration_time = 0.0f;
                float px = ball.controller.pos.x;
                float py = ball.controller.pos.y;
                particle_burst.start(px, py);
                flash.start();
              }
            }
          }

          if (input::is_pressed(input::BUTTON_ESC))
            confirm_modal = !confirm_modal;

          if (confirm_modal) {
            if (input::is_pressed(input::BUTTON_ENTER) ||
                input::is_pressed(input::BUTTON_PAUSE)) {
              audio::play_effect("button.mp3");
              if (paused_index == 0) {
                confirm_modal = false;
              } else if (paused_index == 1) {
                audio::play_effect("button_back.mp3");
                player1.reset();
                player2.reset();
                ball.reset();
                player1.score = 0;
                player2.score = 0;
                particle_burst.active = false;
                flash.active = false;
                in_celebration = false;
                countdown_value = 3;
                countdown_time = 0.0f;
                in_countdown = true;
                game_time_elapsed = 0.0f;
                game_timer_active = true;
                game_running = false;
                paused = false;
                confirm_modal = false;
                menu_state = MENU_PLAYING;
              } else if (paused_index == 2) {
                ball.reset();
                player1.reset();
                player2.reset();
                particle_burst.active = false;
                flash.active = false;
                game_running = false;
                paused = false;
                in_countdown = false;
                confirm_modal = false;
                menu_state = MENU_MAIN;
              }
            }
          }
        }
        particle_burst.render();
        flash.render();
      }

      StretchDIBits(
          hdc, 0, 0, renderer.render_state.width, renderer.render_state.height,
          0, 0, renderer.render_state.width, renderer.render_state.height,
          renderer.render_state.memory, &renderer.render_state.bitmap_info,
          DIB_RGB_COLORS, SRCCOPY);
      // Sleep(1);
    }

    timeEndPeriod(1);
    audio::cleanup();
    destroy();

    return 1;
  }

private:
  static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam,
                                        LPARAM lParam) {
    Window *self;
    if (msg == WM_NCCREATE) {
      CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
      self = reinterpret_cast<Window *>(cs->lpCreateParams);
      SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)self);
      self->window = hwnd;
    }

    self = reinterpret_cast<Window *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (self) {
      return self->WndProc(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  LRESULT WndProc(HWND hwnd, u32 uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE:
    case WM_DESTROY:
      running = false;
      return 0;

    case WM_SIZE: {
      RECT rect;

      GetClientRect(hwnd, &rect);

      i32 new_width = rect.right - rect.left;
      i32 new_height = rect.bottom - rect.top;

      if (new_width <= 0 || new_height <= 0) {
        renderer.render_state.width = renderer.render_state.height = 0;
        return 0;
      }

      if (renderer.render_state.memory) {
        VirtualFree(renderer.render_state.memory, 0, MEM_RELEASE);
        renderer.render_state.memory = nullptr;
      }

      renderer.render_state.width = new_width;
      renderer.render_state.height = new_height;
      const SIZE_T size = static_cast<SIZE_T>(renderer.render_state.width) *
                          static_cast<SIZE_T>(renderer.render_state.height) *
                          sizeof(u32);

      renderer.render_state.memory =
          VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

      if (!renderer.render_state.memory) {
        renderer.render_state.width = renderer.render_state.height = 0;
        return 0;
      }

      ZeroMemory(&renderer.render_state.bitmap_info,
                 sizeof(renderer.render_state.bitmap_info));
      BITMAPINFOHEADER &h = renderer.render_state.bitmap_info.bmiHeader;

      h.biSize = sizeof(h);
      h.biWidth = renderer.render_state.width;
      h.biHeight = -renderer.render_state.height;
      h.biPlanes = 1;
      h.biBitCount = 32;
      h.biCompression = BI_RGB;
      h.biSizeImage = static_cast<DWORD>(size);
    }
      return 0;
    }

    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
  }

  HICON getIcon() {
    return LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APP_ICON));
  }

  inline void destroy() {
    if (renderer.render_state.memory) {
      VirtualFree(renderer.render_state.memory, 0, MEM_RELEASE);
      renderer.render_state.memory = nullptr;
    }

    if (hdc) {
      ReleaseDC(window, hdc);
      hdc = nullptr;
    }

    if (window) {
      DestroyWindow(window);
      window = nullptr;
    }

    if (class_registered) {
      UnregisterClassA(window_class.lpszClassName, window_class.hInstance);
      class_registered = false;
      window_class = {};
    }
  }

  void toggle_fullscreen() {
    DWORD style = GetWindowLong(window, GWL_STYLE);

    if (!is_fullscreen) {
      MONITORINFO mi = {sizeof(mi)};
      if (GetWindowPlacement(window, &prev_wnd_place) &&
          GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY),
                         &mi)) {

        SetWindowLong(window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(window, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                     mi.rcMonitor.right - mi.rcMonitor.left,
                     mi.rcMonitor.bottom - mi.rcMonitor.top,
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
      }
      is_fullscreen = true;
    } else {
      SetWindowLong(window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
      SetWindowPlacement(window, &prev_wnd_place);
      SetWindowPos(window, nullptr, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER |
                       SWP_FRAMECHANGED);
      is_fullscreen = false;
    }
  }

  i32 init() {
    window_class = {};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpszClassName = "Game Window Class";
    window_class.hInstance = GetModuleHandle(nullptr);
    window_class.lpfnWndProc = Window::WndProcStatic;
    window_class.hIcon = getIcon();
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassA(&window_class))
      return 0;
    class_registered = true;

    window = CreateWindowA(window_class.lpszClassName, title.c_str(),
                           WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
                           CW_USEDEFAULT, dimensions.width, dimensions.height,
                           0, 0, window_class.hInstance, this);

    if (!window)
      return 0;

    hdc = GetDC(window);

    particle_burst.renderer = &renderer;
    flash.renderer = &renderer;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&last_counter);

    toggle_fullscreen();

    audio::init();
    game_config.init();

    audio::update_music_volume();
    audio::update_sfx_volume();

    return 1;
  }

  objects::Dimensions dimensions = {};
  std::string title = "Ping Pong Game";
  std::string icon_path = "";

  Config game_config = {"config/config.json"};

  WNDCLASSA window_class = {};
  HWND window = {};
  HDC hdc = {};
  WINDOWPLACEMENT prev_wnd_place = {sizeof(prev_wnd_place)};

  render::Renderer renderer = {};
  World world = {renderer};

  MenuState menu_state = MENU_MAIN;

  objects::Player player1 = {renderer, false};
  objects::Player player2 = {renderer, true};
  objects::Ball ball = {renderer};

  objects::ParticleBurst particle_burst = {renderer};
  objects::FlashEffect flash = {renderer};

  bool running = true;
  bool class_registered = false;
  bool is_fullscreen = false;

  std::vector<std::string> menu_items = {"PLAY VS AI", "PLAY VS FRIEND",
                                         "SETTINGS", "EXIT"};
  int menu_index = 0;
  float countdown_timer = 3.0f;

  bool game_running = false;
  bool paused = false;
  bool confirm_modal = false;

  bool in_countdown = false;
  float countdown_time = 0.0f;
  int countdown_value = 3;

  float game_time_elapsed = 0.0f;
  bool game_timer_active = false;
  bool time_up_state = false;
  float time_up_delay = 0.0f;
  float tick_timer = 0.0f;

  bool in_celebration = false;
  float celebration_time = 0.0f;

  LARGE_INTEGER frequency = {};
  LARGE_INTEGER last_counter = {};
};
} // namespace window
} // namespace game
