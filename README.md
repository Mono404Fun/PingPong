# 🏓 PingPong  

**A fast, modern, and minimal C++ Ping Pong game built entirely in a single header and a main file.**  
Crafted for speed, clarity, and retro charm — no external graphics libraries, just pure Win32 and GDI magic.

---

## ✨ Overview  

**PingPong** is a lightweight 2D game written in **modern C++** using the **Win32 API** directly.  
It demonstrates how to build a complete, interactive, and visually sleek desktop game from scratch with clean architecture and efficient rendering — ideal for learning low-level graphics programming or showcasing game development craftsmanship.

---

## 🎮 Features  

- ⚡ **Smooth gameplay** — optimized frame timing and real-time control  
- 🧠 **AI opponent** — dynamic paddle movement and realistic response  
- 🎨 **Custom UI system** — buttons, menus, and text drawn via Win32 GDI  
- 🧩 **Single-header design** — clean, modular, and easy to include  
- 💡 **Low-level Win32 rendering** — no external dependencies  
- 🎵 **Ready for sound integration** (expandable)  
- 🕹️ **Responsive controls** — tight player feedback loop  
- 🌈 **Professional architecture** — modern C++ code structure  

---

## 🧱 Project Structure  

PingPong/
│
├── game.h # The entire game logic and UI system (single header)
├── main.cpp # Entry point and main loop
│
├── resource.h # Resource identifiers
├── app.rc # Win32 resource script (icon, menu, etc.)
├── app.ico # Application icon
├── app_res.o # Compiled resource object (generated)
│
├── build.bat # Build script (for quick compilation)
│
└── README.md


---

## 🧠 Controls

| Action       | Key                |
|--------------|--------------------|
| Player 1 Up  | `W` / `Z` (AZERTY) |
| Player 1 Down | `S`               |
| Player 2 Up  | Up Arrow           |
| Player 2 Down | Down Arrow         |
| Navigate Menus | Up / Down Arrows  |
| Select / Confirm | Enter            |
| Back / Pause | `P` or Enter (in-game returns to menu) |
| Exit         | `Esc`              |

---

## ⚙️ Build Instructions  

**Requirements:**
- Windows OS  
- C++17 or higher  
- MinGW, Visual Studio, or Clang toolchain  

### 🧩 Build using `build.bat`
Simply double-click `build.bat` or run it from the terminal:
```bash
build.bat
```

### Manual build (example using MinGW / g++)
```bash
g++ main.cpp app_res.o -o PingPong.exe -luser32 -lgdi32 -std=c++17
```

## 🧭 Technical Highlights

- **Single-header architecture** — easy to inspect, include, and modify.  
- **Win32 / GDI based rendering** — no external graphics frameworks.  
- **Custom pixel font renderer** — draws crisp retro text without bitmap assets.  
- **Modular namespaces** — `render`, `input`, `objects`, `ui`, `window`.  
- **Deterministic game loop** — uses `QueryPerformanceCounter` for accurate `dt`.  

---

## ⚙ Settings (menu)

Settings are available in the menu and include:

- **Ball Speed** — configurable multiplier for ball velocity  
- **Paddle Speed** — tuning for paddle responsiveness  
- **AI Difficulty** — selectable levels (Easy / Normal / Hard)  

Use **Left/Right arrows** to change values and **Enter** to confirm or go back.

---

## 🧩 Roadmap

- [ ] Add sound effects and background music  
- [ ] High score saving / local persistence  
- [ ] More polished menu transitions & animations  
- [ ] Local multiplayer / shared keyboard improvements  
- [ ] Theme support (retro neon, dark/light)  

---

## 📸 Screenshots

*(Add real screenshots to `assets/screenshots/` and update the paths below.)*

![Main Menu](assets/screenshots/menu.png)  
![Gameplay](assets/screenshots/gameplay.gif)  

---

## 👤 Author

**Zakaria Aliliche**  
C++ developer focused on low-level graphics, performance, and clean architecture.  
GitHub: [https://github.com/yourusername](https://github.com/yourusername) *(replace with your profile)*  

---

## ⚖️ License

This project is released under the **MIT License**.

> MIT License © 2025 Zakaria Aliliche

---

## 💬 Contributing

Contributions, bug reports, and suggestions are welcome. Suggested workflow:

1. Fork the repo  
2. Create a feature branch (`feature/my-change`)  
3. Make changes & test locally  
4. Open a pull request with a short description and screenshots (if applicable)

---

## 🧠 Acknowledgments

- Inspired by the original **Pong** concept — minimal, elegant gameplay.  
- Built from scratch to demonstrate how to combine modern C++ and Win32 APIs for small game projects.

---

## 🏁 Final Note

> “Simplicity is not absence of complexity — it is clarity of design.”
