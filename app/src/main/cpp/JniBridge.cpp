#include "JniBridge.h"
#include "Game.h"
#include "AndroidOut.h"

JavaVM *JniBridge::sVm = nullptr;
jobject JniBridge::sActivityObj = nullptr;
Game *JniBridge::sGame = nullptr;

void JniBridge::init(JavaVM *vm, jobject activityObj, Game *game) {
    sVm = vm;
    sGame = game;
    if (sVm && activityObj) {
        JNIEnv *env = nullptr;
        if (sVm->GetEnv((void **) &env, JNI_VERSION_1_6) == JNI_OK) {
            if (sActivityObj) {
                env->DeleteGlobalRef(sActivityObj);
            }
            sActivityObj = env->NewGlobalRef(activityObj);
        }
    }
}

void JniBridge::submitScore(int score) {
    if (!sVm || !sActivityObj) return;
    JNIEnv *env = nullptr;
    jint result = sVm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        sVm->AttachCurrentThread(&env, nullptr);
    }
    if (env && sActivityObj) {
        jclass clazz = env->GetObjectClass(sActivityObj);
        jmethodID method = env->GetMethodID(clazz, "submitScoreToLeaderboard", "(I)V");
        if (method) {
            env->CallVoidMethod(sActivityObj, method, score);
        }
    }
}

void JniBridge::showLeaderboard() {
    if (!sVm || !sActivityObj) return;
    JNIEnv *env = nullptr;
    jint result = sVm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        sVm->AttachCurrentThread(&env, nullptr);
    }
    if (env && sActivityObj) {
        jclass clazz = env->GetObjectClass(sActivityObj);
        jmethodID method = env->GetMethodID(clazz, "showLeaderboardsUI", "()V");
        if (method) {
            env->CallVoidMethod(sActivityObj, method);
        }
    }
}

void JniBridge::showInterstitialAd() {
    if (!sVm || !sActivityObj) return;
    JNIEnv *env = nullptr;
    jint result = sVm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        sVm->AttachCurrentThread(&env, nullptr);
    }
    if (env && sActivityObj) {
        jclass clazz = env->GetObjectClass(sActivityObj);
        jmethodID method = env->GetMethodID(clazz, "showInterstitialAd", "()V");
        if (method) {
            env->CallVoidMethod(sActivityObj, method);
        }
    }
}

void JniBridge::showRewardedAd() {
    if (!sVm || !sActivityObj) return;
    JNIEnv *env = nullptr;
    jint result = sVm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        sVm->AttachCurrentThread(&env, nullptr);
    }
    if (env && sActivityObj) {
        jclass clazz = env->GetObjectClass(sActivityObj);
        jmethodID method = env->GetMethodID(clazz, "showRewardedAd", "()V");
        if (method) {
            env->CallVoidMethod(sActivityObj, method);
        }
    }
}

void JniBridge::playSfx(int type) {
    if (!sVm || !sActivityObj) return;
    JNIEnv *env = nullptr;
    jint result = sVm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        sVm->AttachCurrentThread(&env, nullptr);
    }
    if (env && sActivityObj) {
        jclass clazz = env->GetObjectClass(sActivityObj);
        jmethodID method = env->GetMethodID(clazz, "playSfx", "(I)V");
        if (method) {
            env->CallVoidMethod(sActivityObj, method, type);
        }
    }
}

void JniBridge::quitApp() {
    if (!sVm || !sActivityObj) return;
    JNIEnv *env = nullptr;
    jint result = sVm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        sVm->AttachCurrentThread(&env, nullptr);
    }
    if (env && sActivityObj) {
        jclass clazz = env->GetObjectClass(sActivityObj);
        jmethodID method = env->GetMethodID(clazz, "quitApp", "()V");
        if (method) {
            env->CallVoidMethod(sActivityObj, method);
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_beardmachinegames_cupersircle_MainActivity_nativeInitJni(JNIEnv *env, jobject thiz) {
    JavaVM *vm = nullptr;
    env->GetJavaVM(&vm);
    JniBridge::init(vm, thiz, JniBridge::getGame());
}

extern "C" JNIEXPORT void JNICALL
Java_com_beardmachinegames_cupersircle_MainActivity_nativeAddCredits(JNIEnv *env, jobject thiz, jint amount) {
    Game *game = JniBridge::getGame();
    if (game) {
        game->addCredits(amount);
    }
}
