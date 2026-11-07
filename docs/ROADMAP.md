# Cuper Sircle — Development Roadmap

**Project:** Cuper Sircle (Arcade Survival Game)  
**Authors:** The Game Development Council  
**Target Platform:** Android (Native C++ / OpenGL ES + Kotlin / Jetpack / Google Play Services)

---

## Executive Summary & Council Vision

As the Game Development Council, our primary principle remains firm: **Gameplay first, visual identity second, technical complexity last.**

`Cuper Sircle` is a fast-paced, addictive arcade game where precision and reflexes are paramount. The player controls a central circle with a directional indicator arrow and must line up with the gaps of incoming concentric shrinking circles.

This roadmap outlines the phased development of `Cuper Sircle`, moving from core C++/OpenGL ES game loop mechanics through visual identity, persistent player economy (Credits System), Google Play Games Services, AdMob monetization, and audio/QA polish.

---

## Credit & Energy System Specification

- **Maximum Capacity:** 5 Credits.
- **Play Entry Cost:** 1 Credit per game session.
- **Passive Regeneration:**
  - **Rate:** +1 Credit every 60 minutes (1 hour).
  - **Cap Behavior:** Capped strictly at 5 Credits. If a player currently has 4 Credits, waiting 1 hour increments to 5 Credits, at which point passive generation stops until a credit is spent.
  - **Offline Time Calculation:** Timestamps are recorded on session exit and checked against system time on launch/resume to restore earned credits accurately.
- **Ad-Based Replenishment:**
  - Players can watch an AdMob Interstitial/Rewarded Ad to gain +1 Credit (or instantly refill to 5 Credits when at 0).
  - Immediate credit credit upon ad completion callback.

---

## Development Phases

### Phase 1: Core Mechanics & C++ Engine Foundation [COMPLETED]
*Lead: Gameplay Designer & Lead Gameplay Engineer*

- **[x] 1.1 Engine & Game Loop Setup**
  - Set up steady native game loop inside `android_main`, `Renderer.cpp`, and `Game.cpp`.
  - Standardize delta time update independent of frame rendering rate.
- **[x] 1.2 Player Entity & Controls**
  - Render central filled circle with directional triangle arrow.
  - Implement touch drag control to steer arrow around center.
- **[x] 1.3 Obstacle Procedural Generation & Shrinking Rings**
  - Generate concentric rings off-screen with variable gap angles ($0^\circ$ to $360^\circ$).
  - Implement inner radius contraction over time.
  - Collision math: Detect whether player arrow angle matches ring gap angle when ring reaches inner radius threshold.
- **[x] 1.4 Difficulty & Speed Curve**
  - Initial spawn rate: 1 circle / sec ramping to 2 circles / sec by $30\text{s}$.
  - Increase ring movement speed step-wise every 8 completed circles.
  - Calculate survival time and score tracking.

---

### Phase 2: Visual Identity, UI & Accessibility [COMPLETED]
*Lead: Art Director & UX Designer*

- **[x] 2.1 Visual Palette & Shader Pipeline**
  - Implement color palette in custom shaders:
    - Background: `#160B2B`
    - Primary: `#FF3864`
    - Secondary: `#FF8C42`
    - Accent: `#FFD166`
    - Alternative: `#8A4FFF`
    - White: `#F7F0FF`
- **[x] 2.2 Dynamic Theme Interpolation**
  - Smoothly shift colors of background, inner circle, and shrinking rings over time to create an intense visual experience.
- **[x] 2.3 Accessibility & Settings**
  - **Epilepsy Toggle:** Disable dynamic color shifting (lock static high-contrast colors).
  - **Control Scheme Selector:** Switch between Drag Rotation and Tap/Swipe control.
- **[x] 2.4 UI Framework & Menus**
  - Build Main Menu, Settings Screen, HUD (Score & Time top bar), and Game Over Modal.
  - Render credit meter/icons and vector numbers on UI screens.

---

### Phase 3: Player Economy & Persistence [COMPLETED]
*Lead: Lead Gameplay Engineer & Gameplay Designer*

- **[x] 3.1 Persistent Storage Engine**
  - Implement robust local persistence saving High Score, Credits, Last Regeneration Timestamp, and Settings to persistent app file storage.
- **[x] 3.2 Credit Regeneration Logic**
  - Calculate credit recovery: +1 credit every 60 minutes, capped strictly at 5 credits.
  - Calculate offline time elapsed upon launch/resume to restore earned credits up to 5 max.
- **[x] 3.3 Credit Gate Enforcement**
  - Deduct 1 credit per game play session.
  - Block game start when Credits == 0 and display Out of Credits screen with countdown timer.

---

### Phase 4: Google Play Games Services (GPGS) Integration [COMPLETED]
*Lead: Lead Gameplay Engineer & Game Director*

- **[x] 4.1 GPGS SDK Setup**
  - Integrate `com.google.android.gms:play-services-games-v2` in `build.gradle.kts`.
  - Configure `PlayGamesSdk.initialize(this)` in `MainActivity.kt`.
- **[x] 4.2 Leaderboards**
  - Bridge native C++ Game Over score submission to Kotlin GPGS API via JNI (`submitScoreToLeaderboard`).
- **[x] 4.3 Achievements & Social Features**
  - Provide in-menu Leaderboard button trigger (`showLeaderboardsUI`).

---

### Phase 5: Google AdMob Monetization (Interstitial & Rewarded Ads) [COMPLETED]
*Lead: Game Director, Art/UX Designer & Gameplay Engineer*

- **[x] 5.1 AdMob SDK Integration**
  - Add `com.google.android.gms:play-services-ads` to build dependencies and configure App ID meta-data in `AndroidManifest.xml`.
  - Initialize Mobile Ads SDK in `MainActivity.kt`.
- **[x] 5.2 Interstitial Ads**
  - Load interstitial ads in the background (`InterstitialAd.load`).
  - Trigger interstitial ad on Game Over via C++/Kotlin JNI bridge.
- **[x] 5.3 Rewarded Ads for Credits**
  - Implement "Watch Ad to Restore Credit" trigger on Out of Credits screen and Main Menu.
  - Callback handling: Award +1 Credit upon successful video completion via JNI callback (`nativeAddCredits`).

---

### Phase 6: Audio Architecture, Polish & QA [COMPLETED]
*Lead: Audio Designer & QA Specialist*

- **[x] 6.1 Audio Engine & SFX**
  - Integrated zero-latency `ToneGenerator` SFX engine in `MainActivity.kt` triggered via C++/Kotlin JNI bridge (`playSfx`).
  - Audio cues for Ring pass success (beep), Game Over collision (error tone), and Credit Refill (ack tone).
- **[x] 6.2 QA & Impossible Pattern Prevention**
  - Verified procedural ring gap math ($0.85\text{ rad}$ gap size) to guarantee fair reaction windows.
  - Verified 60 FPS fixed delta time loop and timestamp validation for credit regeneration.
- **[x] 6.3 Build Verification**
  - Verified clean compilation and successful APK assembly (`:app:assembleDebug`).

---

## Council Responsibilities Matrix

| Phase | Lead Council Role | Supporting Role | Key Deliverable |
|---|---|---|---|
| **Phase 1: Core Engine** | Lead Gameplay Engineer | Gameplay Designer | C++ Game loop, rotation controls, collision math & shrinking rings |
| **Phase 2: Visuals & UI** | Art Director & UX Designer | Audio & QA | Shaders, color palette shift, menus & epilepsy setting |
| **Phase 3: Energy Economy** | Gameplay Designer | Lead Gameplay Engineer | 5-credit cap, +1hr timer, persistent timestamp saving |
| **Phase 4: Google Play Services** | Lead Gameplay Engineer | Game Director | Leaderboards & achievements integration via JNI |
| **Phase 5: AdMob Integration** | Game Director | Lead Gameplay Engineer | Interstitial & Rewarded Ads for credit restoration |
| **Phase 6: Audio & QA** | Audio Designer & QA | All Members | Sound design, impossible pattern tests, 60 FPS optimization |
