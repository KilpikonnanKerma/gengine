#include <Engine/game.h>
#include <Engine/sceneManager.h>

class GameMain : Game {
public:

    GameMain();

    void Start();
    void Update(float dt);

    SceneManager scene;
};