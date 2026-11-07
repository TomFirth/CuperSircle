#ifndef CUPERSIRCLE_GAME_H
#define CUPERSIRCLE_GAME_H

#include "ShapeRenderer.h"
#include <vector>
#include <random>

enum class GameState {
    MENU,
    PLAYING,
    GAME_OVER,
    OUT_OF_CREDITS
};

enum class ControlScheme {
    DRAG,
    TAP_HALVES
};

struct Ring {
    float radius;       // current radius of inner edge
    float thickness;    // ring wall thickness
    float gapCenter;    // angle in radians [-PI, PI]
    float gapWidth;     // angular gap size in radians
    bool checked;       // true if collision check done
    Color color;
};

struct MenuParticle {
    float x, y;
    float radius;
    float maxRadius;
    float alpha;
    float life;
    float maxLife;
    Color color;
};

struct AdPulseParticle {
    float x, y;
    float radius;
    float maxRadius;
    float alpha;
    float life;
    float maxLife;
    Color color;
};

class Game {
public:
    Game();
    ~Game();

    bool init(const char *savePath = nullptr);
    void update(float dt);
    void render(int width, int height);

    void handleTouchDown(float x, float y, int width, int height);
    void handleTouchMove(float x, float y, int width, int height);
    void handleTouchUp();

    void resetGame();

    GameState getState() const { return state_; }
    int getScore() const { return score_; }
    int getHighScore() const { return highScore_; }
    float getSurvivalTime() const { return survivalTime_; }

    int getCredits() const { return credits_; }
    int getTimeToNextCreditSec() const;
    void addCredits(int amount);

    bool isEpilepsyMode() const { return epilepsyMode_; }
    void toggleEpilepsyMode() {
        epilepsyMode_ = !epilepsyMode_;
        saveGameData();
    }

    ControlScheme getControlScheme() const { return controlScheme_; }
    void toggleControlScheme() {
        controlScheme_ = (controlScheme_ == ControlScheme::DRAG) ? ControlScheme::TAP_HALVES : ControlScheme::DRAG;
        saveGameData();
    }

private:
    void spawnRing();
    void updateTouchAngle(float x, float y, int width, int height);
    static float normalizeAngle(float angle);
    void updateDynamicColors(float dt);

    void updateCreditRegen();
    void saveGameData();
    void loadGameData();

    std::string saveFilePath_;

    GameState state_;
    ControlScheme controlScheme_;
    bool epilepsyMode_;

    int credits_;
    int64_t lastRegenTimeSec_;
    int creditsSpentCounter_;
    static constexpr int MAX_CREDITS = 5;
    static constexpr int REGEN_INTERVAL_SEC = 3600; // 1 hour per credit

    float playerAngle_;       // radians, starts at PI/2 (North)
    float playerRadius_;      // 0.25 world units
    int score_;
    int highScore_;
    float survivalTime_;
    float spawnTimer_;
    int clearedRings_;

    bool isTouching_;
    float lastTouchX_;
    float lastTouchY_;

    float colorTimer_;        // timer for dynamic color shifts
    float menuSpawnTimer_;    // timer for ambient menu circles
    float adPulseSpawnTimer_; // timer for ad button pulse particles
    float lastAspect_;        // last screen aspect ratio

    std::vector<Ring> rings_;
    std::vector<MenuParticle> menuParticles_;
    std::vector<AdPulseParticle> adPulseParticles_;
    ShapeRenderer shapeRenderer_;
    std::mt19937 rng_;

    // Color Theme Palette
    Color colorBg_;
    Color colorPrimary_;
    Color colorSecondary_;
    Color colorAccent_;
    Color colorAlternative_;
    Color colorWhite_;
};

#endif // CUPERSIRCLE_GAME_H
