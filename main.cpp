#include "game.hpp"

i32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, i32 nCmdShow) {
  game::window::Window game_window = {};
  game_window.mainloop();

  return 0;
}
