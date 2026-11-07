#include "Game.h"
#include "JniBridge.h"
#include "Utility.h"
#include "AndroidOut.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

Game::Game()
        : state_(GameState::MENU),
          controlScheme_(ControlScheme::DRAG),
          epilepsyMode_(false),
          credits_(MAX_CREDITS),
          lastRegenTimeSec_(0),
          creditsSpentCounter_(0),
          playerAngle_(M_PI / 2.0f),
          playerRadius_(0.125f), // 50% smaller
          score_(0),
          highScore_(0),
          survivalTime_(0.0f),
          spawnTimer_(0.0f),
          clearedRings_(0),
          isTouching_(false),
          lastTouchX_(0.0f),
          lastTouchY_(0.0f),
          colorTimer_(0.0f),
          menuSpawnTimer_(0.0f),
          adPulseSpawnTimer_(0.0f),
          lastAspect_(16.0f / 9.0f),
          colorBg_(Color::fromHex(0x160B2B)),
          colorPrimary_(Color::fromHex(0xFF3864)),
          colorSecondary_(Color::fromHex(0xFF8C42)),
          colorAccent_(Color::fromHex(0xFFD166)),
          colorAlternative_(Color::fromHex(0x8A4FFF)),
          colorWhite_(Color::fromHex(0xF7F0FF)) {
    std::random_device rd;
    rng_.seed(rd());

    // Spawn 3 initial ambient menu circles
    for (int i = 0; i < 3; ++i) {
        MenuParticle p;
        p.x = static_cast<float>((rng_() % 100 - 50) / 25.0f);
        p.y = static_cast<float>((rng_() % 100 - 50) / 25.0f);
        p.maxRadius = 1.2f + (rng_() % 10) * 0.1f;
        p.radius = p.maxRadius;
        p.maxLife = 3.0f;
        p.life = (rng_() % 30) * 0.1f; // staggered life
        p.alpha = 0.4f;
        Color pal[] = {colorPrimary_, colorSecondary_, colorAccent_, colorAlternative_};
        p.color = pal[i % 4];
        menuParticles_.push_back(p);
    }
}

Game::~Game() {}

bool Game::init(const char *savePath) {
    JniBridge::setGame(this);
    if (savePath) {
        saveFilePath_ = std::string(savePath) + "/cuper_sircle_save.dat";
        loadGameData();
    }
    updateCreditRegen();
    return shapeRenderer_.init();
}

void Game::updateCreditRegen() {
    int64_t nowSec = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

    if (credits_ >= MAX_CREDITS) {
        credits_ = MAX_CREDITS;
        lastRegenTimeSec_ = nowSec;
        return;
    }

    if (lastRegenTimeSec_ <= 0) {
        lastRegenTimeSec_ = nowSec;
        return;
    }

    int64_t elapsed = nowSec - lastRegenTimeSec_;
    if (elapsed >= REGEN_INTERVAL_SEC) {
        int added = static_cast<int>(elapsed / REGEN_INTERVAL_SEC);
        credits_ = std::min(MAX_CREDITS, credits_ + added);

        if (credits_ == MAX_CREDITS) {
            lastRegenTimeSec_ = nowSec;
        } else {
            lastRegenTimeSec_ += added * REGEN_INTERVAL_SEC;
        }
        saveGameData();
    }
}

int Game::getTimeToNextCreditSec() const {
    if (credits_ >= MAX_CREDITS) return 0;
    int64_t nowSec = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t elapsed = nowSec - lastRegenTimeSec_;
    if (elapsed < 0) return REGEN_INTERVAL_SEC;
    int remaining = static_cast<int>(REGEN_INTERVAL_SEC - (elapsed % REGEN_INTERVAL_SEC));
    return std::max(0, remaining);
}

void Game::addCredits(int amount) {
    credits_ = std::min(MAX_CREDITS, credits_ + amount);
    int64_t nowSec = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    if (credits_ == MAX_CREDITS) {
        lastRegenTimeSec_ = nowSec;
    }
    saveGameData();
    JniBridge::playSfx(3);
}

void Game::saveGameData() {
    if (saveFilePath_.empty()) return;

    std::ofstream outFile(saveFilePath_, std::ios::binary);
    if (outFile.is_open()) {
        outFile.write(reinterpret_cast<const char *>(&credits_), sizeof(credits_));
        outFile.write(reinterpret_cast<const char *>(&lastRegenTimeSec_), sizeof(lastRegenTimeSec_));
        outFile.write(reinterpret_cast<const char *>(&highScore_), sizeof(highScore_));
        int ep = epilepsyMode_ ? 1 : 0;
        int ctrl = static_cast<int>(controlScheme_);
        outFile.write(reinterpret_cast<const char *>(&ep), sizeof(ep));
        outFile.write(reinterpret_cast<const char *>(&ctrl), sizeof(ctrl));
        outFile.write(reinterpret_cast<const char *>(&creditsSpentCounter_), sizeof(creditsSpentCounter_));
        outFile.close();
    }
}

void Game::loadGameData() {
    if (saveFilePath_.empty()) return;

    std::ifstream inFile(saveFilePath_, std::ios::binary);
    if (inFile.is_open()) {
        int ep = 0, ctrl = 0;
        inFile.read(reinterpret_cast<char *>(&credits_), sizeof(credits_));
        inFile.read(reinterpret_cast<char *>(&lastRegenTimeSec_), sizeof(lastRegenTimeSec_));
        inFile.read(reinterpret_cast<char *>(&highScore_), sizeof(highScore_));
        inFile.read(reinterpret_cast<char *>(&ep), sizeof(ep));
        inFile.read(reinterpret_cast<char *>(&ctrl), sizeof(ctrl));
        inFile.read(reinterpret_cast<char *>(&creditsSpentCounter_), sizeof(creditsSpentCounter_));
        inFile.close();

        epilepsyMode_ = (ep != 0);
        controlScheme_ = (ctrl == 1) ? ControlScheme::TAP_HALVES : ControlScheme::DRAG;
    }
}

void Game::resetGame() {
    updateCreditRegen();

    if (credits_ <= 0) {
        state_ = GameState::OUT_OF_CREDITS;
        return;
    }

    // Deduct 1 credit to start game
    credits_--;
    creditsSpentCounter_++;

    // Trigger Interstitial ad after 5 credits spent
    if (creditsSpentCounter_ >= 5) {
        creditsSpentCounter_ = 0;
        JniBridge::showInterstitialAd();
    }

    int64_t nowSec = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

    if (credits_ == MAX_CREDITS - 1) {
        lastRegenTimeSec_ = nowSec;
    }

    saveGameData();

    state_ = GameState::PLAYING;
    playerAngle_ = M_PI / 2.0f;
    score_ = 0;
    survivalTime_ = 0.0f;
    spawnTimer_ = 0.0f;
    clearedRings_ = 0;
    rings_.clear();
    spawnRing();
}

float Game::normalizeAngle(float angle) {
    while (angle > M_PI) angle -= 2.0f * M_PI;
    while (angle < -M_PI) angle += 2.0f * M_PI;
    return angle;
}

void Game::updateDynamicColors(float dt) {
    if (epilepsyMode_) {
        colorBg_ = Color::fromHex(0x160B2B);
        colorPrimary_ = Color::fromHex(0xFF3864);
        colorSecondary_ = Color::fromHex(0xFF8C42);
        colorAccent_ = Color::fromHex(0xFFD166);
        colorAlternative_ = Color::fromHex(0x8A4FFF);
        return;
    }

    colorTimer_ += dt * 0.4f;

    float sinBgR = 0.086f + 0.04f * std::sin(colorTimer_);
    float sinBgG = 0.043f + 0.02f * std::cos(colorTimer_ * 0.7f);
    float sinBgB = 0.169f + 0.08f * std::sin(colorTimer_ * 0.5f);
    colorBg_ = Color(sinBgR, sinBgG, sinBgB, 1.0f);

    float sinPriR = 0.9f + 0.1f * std::sin(colorTimer_ * 0.8f);
    float sinPriG = 0.22f + 0.15f * std::cos(colorTimer_ * 0.6f);
    float sinPriB = 0.4f + 0.1f * std::sin(colorTimer_ * 0.9f);
    colorPrimary_ = Color(sinPriR, sinPriG, sinPriB, 1.0f);
}

void Game::spawnRing() {
    std::uniform_real_distribution<float> angleDist(-M_PI, M_PI);

    Color palette[] = {colorPrimary_, colorSecondary_, colorAccent_, colorAlternative_};
    Color ringColor = palette[rings_.size() % 4];

    Ring ring;
    ring.radius = 2.5f;       // Start outside screen view
    ring.thickness = 0.0375f; // 75% thinner (0.15 * 0.25)
    ring.gapCenter = angleDist(rng_);
    ring.gapWidth = 0.85f;     // ~49 degrees gap
    ring.checked = false;
    ring.color = ringColor;

    rings_.push_back(ring);
}

void Game::updateTouchAngle(float x, float y, int width, int height) {
    if (width <= 0 || height <= 0) return;

    float halfHeight = 2.0f;
    float aspect = static_cast<float>(width) / static_cast<float>(height);

    float worldX = (x / static_cast<float>(width) - 0.5f) * 2.0f * aspect * halfHeight;
    float worldY = -(y / static_cast<float>(height) - 0.5f) * 2.0f * halfHeight;

    playerAngle_ = std::atan2(worldY, worldX);
}

void Game::handleTouchDown(float x, float y, int width, int height) {
    isTouching_ = true;
    lastTouchX_ = x;
    lastTouchY_ = y;

    if (width <= 0 || height <= 0) return;
    float halfHeight = 2.0f;
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    float worldX = (x / static_cast<float>(width) - 0.5f) * 2.0f * aspect * halfHeight;
    float worldY = -(y / static_cast<float>(height) - 0.5f) * 2.0f * halfHeight;

    if (state_ == GameState::MENU) {
        // 1. Check New Game button on menu: x in [-0.7, 0.7], y in [-0.2, 0.2]
        if (worldX >= -0.7f && worldX <= 0.7f && worldY >= -0.2f && worldY <= 0.2f) {
            resetGame();
            return;
        }

        // 2. Check Quit Game button on menu: x in [-0.7, 0.7], y in [-0.8, -0.4]
        if (worldX >= -0.7f && worldX <= 0.7f && worldY >= -0.8f && worldY <= -0.4f) {
            JniBridge::quitApp();
            return;
        }

        // 3. Check Settings Cog (top right): x > aspect * 1.3, y > 1.2
        if (worldX > aspect * 1.3f && worldY > 1.2f) {
            toggleEpilepsyMode();
            return;
        }

        // 4. Check Ad Button (bottom right): width 0.35, height 0.2
        float adBtnX = aspect * 1.4f;
        float adBtnY = -1.5f;
        if (worldX >= adBtnX - 0.175f && worldX <= adBtnX + 0.175f &&
            worldY >= adBtnY - 0.1f && worldY <= adBtnY + 0.1f) {
            JniBridge::showRewardedAd();
            return;
        }

    } else if (state_ == GameState::OUT_OF_CREDITS) {
        JniBridge::showRewardedAd();
        updateCreditRegen();
        if (credits_ > 0) {
            resetGame();
        } else {
            state_ = GameState::MENU;
        }
    } else if (state_ == GameState::GAME_OVER) {
        // Check New Game button: x in [-0.6, 0.6], y in [-1.1, -0.7]
        if (worldX >= -0.6f && worldX <= 0.6f && worldY >= -1.1f && worldY <= -0.7f) {
            resetGame();
            return;
        }
        // Check Return to Menu button: x in [-0.6, 0.6], y in [-1.55, -1.2]
        if (worldX >= -0.6f && worldX <= 0.6f && worldY >= -1.55f && worldY <= -1.2f) {
            state_ = GameState::MENU;
            return;
        }
        resetGame();
        return;
    } else if (state_ == GameState::PLAYING) {
        if (controlScheme_ == ControlScheme::DRAG) {
            updateTouchAngle(x, y, width, height);
        }
    }
}

void Game::handleTouchMove(float x, float y, int width, int height) {
    lastTouchX_ = x;
    lastTouchY_ = y;

    if (state_ == GameState::PLAYING && isTouching_) {
        if (controlScheme_ == ControlScheme::DRAG) {
            updateTouchAngle(x, y, width, height);
        }
    }
}

void Game::handleTouchUp() {
    isTouching_ = false;
}

void Game::update(float dt) {
    updateDynamicColors(dt);
    updateCreditRegen();

    if (state_ == GameState::MENU) {
        // Update ambient menu particles (fade in, shrink, fade out over 3s, spawn every 1s)
        menuSpawnTimer_ += dt;
        if (menuSpawnTimer_ >= 1.0f) {
            menuSpawnTimer_ -= 1.0f;
            MenuParticle p;
            p.x = static_cast<float>((rng_() % 100 - 50) / 25.0f);
            p.y = static_cast<float>((rng_() % 100 - 50) / 25.0f);
            p.maxRadius = 1.0f + (rng_() % 15) * 0.1f;
            p.radius = p.maxRadius;
            p.maxLife = 3.0f;
            p.life = 0.0f;
            p.alpha = 0.5f;
            Color pal[] = {colorPrimary_, colorSecondary_, colorAccent_, colorAlternative_};
            p.color = pal[rng_() % 4];
            menuParticles_.push_back(p);
        }

        for (auto &p : menuParticles_) {
            p.life += dt;
            float progress = p.life / p.maxLife;
            // shrink radius and fade out/in
            p.radius = p.maxRadius * (1.0f - progress * 0.7f);
            if (progress < 0.2f) {
                p.alpha = (progress / 0.2f) * 0.5f;
            } else {
                p.alpha = (1.0f - (progress - 0.2f) / 0.8f) * 0.5f;
            }
        }

        menuParticles_.erase(
            std::remove_if(menuParticles_.begin(), menuParticles_.end(), [](const MenuParticle &p) {
                return p.life >= p.maxLife;
            }),
            menuParticles_.end()
        );

        // Update Ad Button Pulse Particles based on missing credits
        if (credits_ < 5) {
            int missing = 5 - credits_; // 1 to 5
            float spawnInterval = 1.0f - (missing - 1) * 0.16f; // 1.0s down to ~0.33s (up to 3 per sec)
            adPulseSpawnTimer_ += dt;
            if (adPulseSpawnTimer_ >= spawnInterval) {
                adPulseSpawnTimer_ -= spawnInterval;
                AdPulseParticle ap;
                ap.x = lastAspect_ * 1.4f;
                ap.y = -1.5f;
                ap.maxRadius = 0.25f + missing * 0.08f;
                ap.radius = ap.maxRadius;
                ap.maxLife = 3.0f - (missing - 1) * 0.625f; // 3.0s down to 0.5s shrink speed
                ap.life = 0.0f;
                ap.alpha = 0.1f + (missing - 1) * 0.225f; // 10% (0.1) when 1 missing, 100% (1.0) when 5 missing
                ap.color = colorAccent_;
                adPulseParticles_.push_back(ap);
            }
        }

        for (auto &ap : adPulseParticles_) {
            ap.life += dt;
            float progress = ap.life / ap.maxLife;
            ap.radius = ap.maxRadius * (1.0f - progress);
            float initialAlpha = 0.1f + (5 - credits_ - 1) * 0.225f;
            if (progress < 0.2f) {
                ap.alpha = (progress / 0.2f) * initialAlpha;
            } else {
                ap.alpha = (1.0f - (progress - 0.2f) / 0.8f) * initialAlpha;
            }
        }

        adPulseParticles_.erase(
            std::remove_if(adPulseParticles_.begin(), adPulseParticles_.end(), [](const AdPulseParticle &ap) {
                return ap.life >= ap.maxLife;
            }),
            adPulseParticles_.end()
        );

    } else if (state_ == GameState::PLAYING) {
        if (isTouching_ && controlScheme_ == ControlScheme::TAP_HALVES) {
            float turnSpeed = 3.5f;
            if (lastTouchX_ < lastTouchY_) {
                playerAngle_ += turnSpeed * dt;
            } else {
                playerAngle_ -= turnSpeed * dt;
            }
            playerAngle_ = normalizeAngle(playerAngle_);
        }

        survivalTime_ += dt;

        float ramp = std::min(1.0f, survivalTime_ / 30.0f);
        float spawnInterval = 1.0f - ramp * 0.5f;

        spawnTimer_ += dt;
        if (spawnTimer_ >= spawnInterval) {
            spawnRing();
            spawnTimer_ -= spawnInterval;
        }

        float baseSpeed = 0.85f;
        float speedMultiplier = 1.0f + (clearedRings_ / 8) * 0.15f;
        float ringSpeed = baseSpeed * speedMultiplier;

        for (auto &ring: rings_) {
            ring.radius -= ringSpeed * dt;

            if (!ring.checked && ring.radius <= 0.21f) {
                ring.checked = true;

                float angleDiff = normalizeAngle(playerAngle_ - ring.gapCenter);
                float halfGap = ring.gapWidth / 2.0f;

                if (std::abs(angleDiff) <= halfGap) {
                    clearedRings_++;
                    score_++;
                    highScore_ = std::max(highScore_, score_);
                    JniBridge::playSfx(1);
                } else {
                    state_ = GameState::GAME_OVER;
                    highScore_ = std::max(highScore_, score_);
                    saveGameData();

                    JniBridge::playSfx(2);
                    JniBridge::submitScore(score_);

                    aout << "GAME OVER! Score: " << score_ << " Best: " << highScore_ << std::endl;
                    break;
                }
            }
        }

        rings_.erase(
                std::remove_if(rings_.begin(), rings_.end(), [](const Ring &r) {
                    return (r.radius + r.thickness) <= 0.05f;
                }),
                rings_.end()
        );
    }
}

void Game::render(int width, int height) {
    if (width <= 0 || height <= 0) return;

    float aspect = static_cast<float>(width) / static_cast<float>(height);
    lastAspect_ = aspect;
    float halfHeight = 2.0f;

    float projMatrix[16];
    Utility::buildOrthographicMatrix(projMatrix, halfHeight, aspect, -1.0f, 1.0f);
    shapeRenderer_.setProjectionMatrix(projMatrix);

    // Clear background
    glClearColor(colorBg_.r, colorBg_.g, colorBg_.b, colorBg_.a);
    glClear(GL_COLOR_BUFFER_BIT);

    shapeRenderer_.begin();

    // Render Ambient Menu Particles if in MENU state
    if (state_ == GameState::MENU) {
        for (const auto &p : menuParticles_) {
            shapeRenderer_.drawRingArc(p.x, p.y, p.radius * 0.8f, p.radius, 0.0f, 2.0f * M_PI, p.color);
        }

        // Render Ad Button Pulse Particles
        for (const auto &ap : adPulseParticles_) {
            Color col = ap.color;
            col.a = ap.alpha;
            shapeRenderer_.drawRingArc(ap.x, ap.y, ap.radius * 0.8f, ap.radius, 0.0f, 2.0f * M_PI, col, 24);
        }
    }

    // 1. Draw Shrinking Rings (if playing)
    if (state_ == GameState::PLAYING) {
        for (const auto &ring: rings_) {
            float startAngle = ring.gapCenter + ring.gapWidth / 2.0f;
            float endAngle = ring.gapCenter - ring.gapWidth / 2.0f + 2.0f * M_PI;

            shapeRenderer_.drawRingArc(0.0f, 0.0f, ring.radius, ring.radius + ring.thickness,
                                       startAngle, endAngle, ring.color);
        }
    }

    // 2. Draw Central Player Circle (if playing)
    if (state_ == GameState::PLAYING) {
        shapeRenderer_.drawFilledCircle(0.0f, 0.0f, playerRadius_, colorPrimary_);

        // 3. Draw Directional Arrow Pointer
        float arrowLength = 0.22f;
        float spreadAngle = 0.25f;

        float tipX = arrowLength * std::cos(playerAngle_);
        float tipY = arrowLength * std::sin(playerAngle_);

        float baseLeftX = playerRadius_ * std::cos(playerAngle_ + spreadAngle);
        float baseLeftY = playerRadius_ * std::sin(playerAngle_ + spreadAngle);

        float baseRightX = playerRadius_ * std::cos(playerAngle_ - spreadAngle);
        float baseRightY = playerRadius_ * std::sin(playerAngle_ - spreadAngle);

        shapeRenderer_.drawTriangle(tipX, tipY, baseLeftX, baseLeftY, baseRightX, baseRightY, colorAccent_);
    }

    // 4. Render HUD / UI Overlays
    float hudY = 1.6f;
    float digitW = 0.12f;
    float digitH = 0.22f;

    if (state_ == GameState::PLAYING) {
        // Current Score at Top Center
        shapeRenderer_.drawNumber(score_, -0.06f, hudY, digitW, digitH, colorWhite_);
    } else if (state_ == GameState::MENU) {
        // --- MENU SCREEN UI ---
        // Title text "CUPER SIRCLE"
        shapeRenderer_.drawText("CUPER SIRCLE", -0.75f, 1.25f, 0.08f, 0.15f, colorAccent_);

        // High Score display below title
        shapeRenderer_.drawNumber(highScore_, -0.06f, hudY, digitW, digitH, colorWhite_);

        // "New Game" Button
        shapeRenderer_.drawRect(-0.7f, -0.2f, 1.4f, 0.4f, colorPrimary_);
        shapeRenderer_.drawText("NEW GAME", -0.45f, -0.05f, 0.08f, 0.15f, colorWhite_);

        // "Quit Game" Button
        shapeRenderer_.drawRect(-0.7f, -0.8f, 1.4f, 0.4f, colorSecondary_);
        shapeRenderer_.drawText("QUIT GAME", -0.55f, -0.65f, 0.08f, 0.15f, colorBg_);

        // Settings Cog (Top Right)
        shapeRenderer_.drawFilledCircle(aspect * 1.5f, 1.5f, 0.18f, colorSecondary_);
        shapeRenderer_.drawFilledCircle(aspect * 1.5f, 1.5f, 0.08f, colorBg_);

        // Credits Display (Bottom Left)
        float creditX = -aspect * 1.7f;
        float creditY = -1.5f;
        for (int i = 0; i < MAX_CREDITS; ++i) {
            Color cColor = (i < credits_) ? colorAccent_ : Color(0.3f, 0.3f, 0.3f, 0.5f);
            shapeRenderer_.drawFilledCircle(creditX + i * 0.18f, creditY, 0.06f, cColor);
        }

        // Ad Button (Bottom Right) - 50% smaller (width 0.35, height 0.2)
        float adBtnX = aspect * 1.4f;
        float adBtnY = -1.5f;
        shapeRenderer_.drawRect(adBtnX - 0.175f, adBtnY - 0.1f, 0.35f, 0.2f, colorAlternative_);
        shapeRenderer_.drawText("AD", adBtnX - 0.08f, adBtnY - 0.04f, 0.07f, 0.1f, colorWhite_);

    } else if (state_ == GameState::OUT_OF_CREDITS) {
        // Out of Credits Prompt
        shapeRenderer_.drawRect(-1.0f, -0.6f, 2.0f, 1.2f, Color(0.1f, 0.05f, 0.2f, 0.9f));
        int remainingSec = getTimeToNextCreditSec();
        shapeRenderer_.drawNumber(remainingSec, -0.3f, 0.0f, digitW * 1.2f, digitH * 1.2f, colorAccent_);
    } else if (state_ == GameState::GAME_OVER) {
        // Game Over Score Card: show only the final score
        shapeRenderer_.drawNumber(score_, -0.06f, 0.4f, digitW * 1.5f, digitH * 1.5f, colorPrimary_);

        // "New Game" Button (Orange button with high-contrast dark text)
        shapeRenderer_.drawRect(-0.6f, -1.05f, 1.2f, 0.35f, colorSecondary_);
        shapeRenderer_.drawText("NEW GAME", -0.42f, -0.93f, 0.08f, 0.15f, colorBg_);

        // "Return to Menu" Button (Purple button with white text)
        shapeRenderer_.drawRect(-0.6f, -1.5f, 1.2f, 0.35f, colorAlternative_);
        shapeRenderer_.drawText("MENU", -0.22f, -1.38f, 0.08f, 0.15f, colorWhite_);
    }

    shapeRenderer_.end();
}
