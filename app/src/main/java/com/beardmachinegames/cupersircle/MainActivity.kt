package com.beardmachinegames.cupersircle

import android.media.AudioManager
import android.media.ToneGenerator
import android.os.Bundle
import android.util.Log
import android.view.WindowInsets
import android.view.WindowInsetsController
import com.google.android.gms.ads.AdRequest
import com.google.android.gms.ads.LoadAdError
import com.google.android.gms.ads.MobileAds
import com.google.android.gms.ads.interstitial.InterstitialAd
import com.google.android.gms.ads.interstitial.InterstitialAdLoadCallback
import com.google.android.gms.ads.rewarded.RewardedAd
import com.google.android.gms.ads.rewarded.RewardedAdLoadCallback
import com.google.android.gms.games.PlayGames
import com.google.android.gms.games.PlayGamesSdk
import com.google.androidgamesdk.GameActivity

class MainActivity : GameActivity() {

    companion object {
        private const val TAG = "CuperSircleActivity"
        private const val TEST_INTERSTITIAL_AD_UNIT_ID = "ca-app-pub-3940256099942544/1033173712"
        private const val TEST_REWARDED_AD_UNIT_ID = "ca-app-pub-3940256099942544/5224354917"
        private const val LEADERBOARD_ID = "CgkI_sample_leaderboard_id"

        init {
            System.loadLibrary("cupersircle")
        }
    }

    private var interstitialAd: InterstitialAd? = null
    private var rewardedAd: RewardedAd? = null
    private var toneGenerator: ToneGenerator? = null

    private external fun nativeInitJni()
    private external fun nativeAddCredits(amount: Int)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        try {
            toneGenerator = ToneGenerator(AudioManager.STREAM_MUSIC, 80)
        } catch (e: Exception) {
            Log.e(TAG, "ToneGenerator init failed", e)
        }

        // Initialize Google Play Games Services SDK
        try {
            PlayGamesSdk.initialize(this)
        } catch (e: Exception) {
            Log.e(TAG, "Play Games SDK initialization failed", e)
        }

        // Initialize Google Mobile Ads (AdMob) SDK
        MobileAds.initialize(this) {
            loadInterstitialAd()
            loadRewardedAd()
        }

        nativeInitJni()
    }

    override fun onDestroy() {
        toneGenerator?.release()
        toneGenerator = null
        super.onDestroy()
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) {
            hideSystemUi()
        }
    }

    private fun hideSystemUi() {
        window.insetsController?.apply {
            hide(WindowInsets.Type.systemBars())
            systemBarsBehavior = WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }
    }

    fun playSfx(type: Int) {
        try {
            when (type) {
                1 -> toneGenerator?.startTone(ToneGenerator.TONE_PROP_BEEP, 40)
                2 -> toneGenerator?.startTone(ToneGenerator.TONE_SUP_ERROR, 120)
                3 -> toneGenerator?.startTone(ToneGenerator.TONE_PROP_ACK, 70)
            }
        } catch (e: Exception) {
            Log.e(TAG, "playSfx error", e)
        }
    }

    fun quitApp() {
        finish()
    }

    // --- Google Play Games Services Leaderboards ---
    fun submitScoreToLeaderboard(score: Int) {
        try {
            PlayGames.getLeaderboardsClient(this)
                .submitScore(LEADERBOARD_ID, score.toLong())
        } catch (e: Exception) {
            Log.e(TAG, "Failed to submit score to leaderboard", e)
        }
    }

    fun showLeaderboardsUI() {
        try {
            PlayGames.getLeaderboardsClient(this)
                .allLeaderboardsIntent
                .addOnSuccessListener { intent ->
                    startActivityForResult(intent, 9002)
                }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to launch leaderboard UI", e)
        }
    }

    // --- Google AdMob Interstitial Ads ---
    private fun loadInterstitialAd() {
        val adRequest = AdRequest.Builder().build()
        InterstitialAd.load(
            this,
            TEST_INTERSTITIAL_AD_UNIT_ID,
            adRequest,
            object : InterstitialAdLoadCallback() {
                override fun onAdLoaded(ad: InterstitialAd) {
                    interstitialAd = ad
                }

                override fun onAdFailedToLoad(error: LoadAdError) {
                    interstitialAd = null
                }
            }
        )
    }

    fun showInterstitialAd() {
        runOnUiThread {
            if (interstitialAd != null) {
                interstitialAd?.show(this)
                loadInterstitialAd() // Reload for next trigger
            } else {
                loadInterstitialAd()
            }
        }
    }

    // --- Google AdMob Rewarded Ads for Credits ---
    private fun loadRewardedAd() {
        val adRequest = AdRequest.Builder().build()
        RewardedAd.load(
            this,
            TEST_REWARDED_AD_UNIT_ID,
            adRequest,
            object : RewardedAdLoadCallback() {
                override fun onAdLoaded(ad: RewardedAd) {
                    rewardedAd = ad
                }

                override fun onAdFailedToLoad(error: LoadAdError) {
                    rewardedAd = null
                }
            }
        )
    }

    fun showRewardedAd() {
        runOnUiThread {
            if (rewardedAd != null) {
                rewardedAd?.show(this) { _ ->
                    // Award 5 credits (capped at max 5) upon video completion!
                    nativeAddCredits(5)
                }
                loadRewardedAd() // Reload for next use
            } else {
                // If ad not ready, grant fallback credits or retry loading
                nativeAddCredits(5)
                loadRewardedAd()
            }
        }
    }
}
