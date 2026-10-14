```
linked-list-game/
├── CMakeLists.txt              # Root build script
├── config/                     # Engine configs, window defaults, keybindings
├── assets/                     # (To add) Shaders, fonts, textures
│   ├── fonts/
│   └── shaders/
│       ├── node.vert
│       ├── node.frag
│       ├── chain.vert
│       └── chain.frag
├── external/                   # (To add) Third-party headers & loaders (GLAD, stb)
│   ├── glad/
│   │   ├── include/
│   │   └── src/glad.c
│   └── stb/
├── include/
│   └── game/
│       ├── core/               # Application, Window, Input state
│       ├── model/              # Pure data: Node, LinkedListGraph, PointerSlot
│       ├── render/             # Shaders, BatchRenderer, Camera2D, FontRenderer
│       └── physics/            # Chain springs, socket snap detection, drag logic
├── src/
│   ├── main.cpp
│   ├── core/
│   ├── model/
│   ├── render/
│   └── physics/
├── test/                       # Unit tests for the data structures (Catch2 / GTest)
└── LICENSE
```
