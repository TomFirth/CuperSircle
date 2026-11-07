#ifndef CUPERSIRCLE_JNIBRIDGE_H
#define CUPERSIRCLE_JNIBRIDGE_H

#include <jni.h>

class Game;

class JniBridge {
public:
    static void init(JavaVM *vm, jobject activityObj, Game *game);
    static void submitScore(int score);
    static void showLeaderboard();
    static void showInterstitialAd();
    static void showRewardedAd();
    static void playSfx(int type);
    static void quitApp();

    static Game *getGame() { return sGame; }
    static void setGame(Game *game) { sGame = game; }

private:
    static JavaVM *sVm;
    static jobject sActivityObj;
    static Game *sGame;
};

#endif // CUPERSIRCLE_JNIBRIDGE_H
