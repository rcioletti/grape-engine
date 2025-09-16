# 🍇 Grape Engine

C++ game engine for quick prototypes, game jams and tinkering with graphics and tools.  
Grape Engine focuses on Vulkan rendering, a lightweight editor, and a straightforward game object system to make experimentation easy.

## Features

- Vulkan-based renderer
- Windowing and input via [GLFW](https://www.glfw.org/)
- Physics integration with NVIDIA PhysX
- Editor UI using [Dear ImGui](https://github.com/ocornut/imgui)
- GameObject / entity management for clearer code structure
- Support for 3D model import and point lights

## Demo

https://github.com/user-attachments/assets/11e37a9f-fc65-441a-a758-834a666a7f6c

A short demo showing the engine working.

## Roadmap

- [ ] More physics features: joints, constraints, ragdolls
- [ ] Asset importer & pipeline improvements
- [ ] Scene serialization and prefab support
- [ ] Editor UX improvements (inspector, hierarchy, scene view)
- [ ] Audio subsystem
- [ ] Performance profiling and optimization
- [ ] Example projects / sample games

## Credits / Third-Party Libraries

| Library | Link | Purpose |
|---|---|---|
| Vulkan | https://www.vulkan.org/ | Graphics API / renderer |
| GLFW | https://www.glfw.org/ | Windowing & input |
| GLM | https://github.com/g-truc/glm | Math library | (add) | (add) |
| stb (nothings/stb) | https://github.com/nothings/stb | Image loading / utilities | 
| tinyobjloader | https://github.com/tinyobjloader/tinyobjloader | Model import |
| Assimp | https://github.com/assimp/assimp | Model import |
| Dear ImGui | https://github.com/ocornut/imgui | Editor UI | 
| NVIDIA PhysX | https://github.com/NVIDIA/PhysX | Physics engine |

## License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.

---
