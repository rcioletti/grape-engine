#pragma once

namespace grape {
class GameLoop {
public:
    void run();
private:
    void processInput(float frameTime);
    void updatePhysics(float frameTime);
    void updateGameObjects(float frameTime);
    void renderFrame();
};
}