#ifndef CLIENT_WEATHER_INC
#define CLIENT_WEATHER_INC

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <GL/gl.h>
#include "triangleapi.h"
#include "ref_params.h"
#include "cvardef.h"

#undef min
#undef max

// Mansion exterior weather: Reddish atmospheric fog and falling red rain
// Mansion map bounding boxes (coordinates from truth_exterior.py / map geometry)
// House interior: [-736, 208] x [-800, 288], roof z <= 350
// Outside grounds: central courtyard ground Z ~ -190, outer moat Z ~ -420

static cvar_s* cremationRedFog = nullptr;
static cvar_s* cremationRedRain = nullptr;
static cvar_s* cremationRainDensity = nullptr;

static bool weatherInitialized = false;
static bool weatherFogActive = false;
static double lastWeatherTime = 0.0;
static float nextDripSoundTime = 0.0f;

// Camera state captured from HUD_CalcRefdef
static Vector weatherCameraOrigin(0, 0, 0);
static Vector weatherCameraAngles(0, 0, 0);
static Vector weatherCameraForward(1, 0, 0);
static Vector weatherCameraRight(0, 1, 0);
static Vector weatherCameraUp(0, 0, 1);
static bool weatherHaveCamera = false;

// Fog state
static float currentFogStart = 20.0f;
static float currentFogEnd = 580.0f;
static float currentFogDensity = 0.0042f;
static float currentFogColor[3] = { 160.0f, 24.0f, 24.0f }; // Deep, saturated crimson red

// Rain particles
struct RainParticle {
    Vector pos;
    float len;
    float alpha;
    float speed;
};

static constexpr int MAX_RAIN_DROPS = 500;
static RainParticle rainDrops[MAX_RAIN_DROPS];
static int activeRainCount = 200;

// Rain splash ripples on the ground
struct RainSplash {
    Vector pos;
    float radius;
    float maxRadius;
    float life;
    float maxLife;
};

static constexpr int MAX_RAIN_SPLASHES = 48;
static RainSplash rainSplashes[MAX_RAIN_SPLASHES];
static int nextSplashIndex = 0;

// Fall velocity vector: slow, eerie descent with subtle drift
static const Vector rainDir = Vector(0.046f, -0.027f, -0.998f);
static const Vector rainVel = Vector(24.0f, -14.0f, -520.0f);

// Interior bounding checks: rain and dense fog avoid the house interior
static inline bool isInsideMansion(const Vector& pos) {
    // Main house interior
    if (pos.x >= -736.0f && pos.x <= 208.0f &&
        pos.y >= -800.0f && pos.y <= 288.0f &&
        pos.z <= 350.0f) {
        return true;
    }
    // Southwest pagoda hut
    if (pos.x >= -1920.0f && pos.x <= -1664.0f &&
        pos.y >= -1792.0f && pos.y <= -1600.0f &&
        pos.z <= 250.0f) {
        return true;
    }
    // Northeast pagoda hut
    if (pos.x >= 960.0f && pos.x <= 1216.0f &&
        pos.y >= 1088.0f && pos.y <= 1280.0f &&
        pos.z <= 250.0f) {
        return true;
    }
    return false;
}

// Ground elevation around the house
static inline float getGroundZ(float x, float y) {
    // Central courtyard / garden surrounding the house
    if (x >= -1200.0f && x <= 500.0f && y >= -1100.0f && y <= 600.0f) {
        return -190.0f;
    }
    // Outer powerup islands
    if ((x >= 900.0f && x <= 1300.0f && y >= 1000.0f && y <= 1350.0f) ||
        (x >= -2000.0f && x <= -1600.0f && y >= -1850.0f && y <= -1550.0f)) {
        return -190.0f;
    }
    // Sunken perimeter moat / outer arena
    return -420.0f;
}

static inline float weatherRandomFloat(float minVal, float maxVal) {
    float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return minVal + r * (maxVal - minVal);
}

static void spawnRainDrop(RainParticle& drop, const Vector& center, bool initialSpread) {
    drop.pos.x = center.x + weatherRandomFloat(-650.0f, 650.0f);
    drop.pos.y = center.y + weatherRandomFloat(-650.0f, 650.0f);
    if (initialSpread) {
        drop.pos.z = center.z + weatherRandomFloat(-150.0f, 480.0f);
    } else {
        drop.pos.z = center.z + weatherRandomFloat(220.0f, 480.0f);
    }
    drop.len = weatherRandomFloat(16.0f, 25.0f);
    drop.speed = weatherRandomFloat(460.0f, 580.0f);
    drop.alpha = weatherRandomFloat(0.65f, 0.95f);
}

static void spawnSplash(const Vector& pos) {
    auto& sp = rainSplashes[nextSplashIndex];
    nextSplashIndex = (nextSplashIndex + 1) % MAX_RAIN_SPLASHES;
    sp.pos = pos;
    sp.radius = 2.0f;
    sp.maxRadius = weatherRandomFloat(5.0f, 9.0f);
    sp.life = 0.18f;
    sp.maxLife = 0.18f;
}

static bool isMansionLevel() {
    const char* level = eng.pfnGetLevelName();
    if (!level || !*level) return false;
    return std::strstr(level, "cs_mansion") != nullptr;
}

static void weatherInit() {
    if (!cremationRedFog) {
        cremationRedFog = eng.pfnRegisterVariable("cremation_red_fog", "1", 1);
        cremationRedRain = eng.pfnRegisterVariable("cremation_red_rain", "1", 1);
        cremationRainDensity = eng.pfnRegisterVariable("cremation_rain_density", "200", 1);
        if (cremationRainDensity && cremationRainDensity->value > 250.0f) {
            eng.Cvar_SetValue("cremation_rain_density", 200.0f);
        }
    }

    cvar_s* glFog = eng.pfnGetCvarPointer("gl_fog");
    if (glFog && glFog->value <= 0.0f) {
        eng.Cvar_SetValue("gl_fog", 1.0f);
    }

    Vector origin(0, 0, 0);
    for (int i = 0; i < MAX_RAIN_DROPS; ++i) {
        spawnRainDrop(rainDrops[i], origin, true);
    }
    for (int i = 0; i < MAX_RAIN_SPLASHES; ++i) {
        rainSplashes[i].life = 0.0f;
    }
    weatherInitialized = true;
    lastWeatherTime = eng.GetClientTime();
}

static void weatherClear() {
    weatherHaveCamera = false;
    Vector origin(0, 0, 0);
    for (int i = 0; i < MAX_RAIN_DROPS; ++i) {
        spawnRainDrop(rainDrops[i], origin, true);
    }
    for (int i = 0; i < MAX_RAIN_SPLASHES; ++i) {
        rainSplashes[i].life = 0.0f;
    }
    currentFogStart = 20.0f;
    currentFogEnd = 580.0f;
    currentFogDensity = 0.0042f;
    currentFogColor[0] = 160.0f;
    currentFogColor[1] = 24.0f;
    currentFogColor[2] = 24.0f;
    nextDripSoundTime = 0.0f;
}

static void weatherOnRefdef(struct ref_params_s* pparams) {
    if (!pparams) return;
    weatherCameraOrigin = Vector(pparams->vieworg[0], pparams->vieworg[1], pparams->vieworg[2]);
    weatherCameraAngles = Vector(pparams->viewangles[0], pparams->viewangles[1], pparams->viewangles[2]);
    weatherCameraForward = Vector(pparams->forward[0], pparams->forward[1], pparams->forward[2]);
    weatherCameraRight = Vector(pparams->right[0], pparams->right[1], pparams->right[2]);
    weatherCameraUp = Vector(pparams->up[0], pparams->up[1], pparams->up[2]);
    weatherHaveCamera = true;
}

static void weatherFrame(double time) {
    if (!weatherInitialized) weatherInit();

    bool mansion = isMansionLevel();
    if (!mansion) {
        if (weatherFogActive) {
            float zeroColor[3] = { 0.0f, 0.0f, 0.0f };
            if (eng.pTriAPI && eng.pTriAPI->Fog) {
                eng.pTriAPI->Fog(zeroColor, 0.0f, 0.0f, 0);
            }
            weatherFogActive = false;
        }
        return;
    }

    float dt = static_cast<float>(time - lastWeatherTime);
    if (dt <= 0.0f || dt > 0.2f) dt = 0.016f;
    lastWeatherTime = time;

    Vector camera = weatherCameraOrigin;
    if (!weatherHaveCamera) {
        auto local = eng.GetLocalPlayer();
        if (local) camera = Vector(local->origin[0], local->origin[1], local->origin[2] + 24.0f);
    }

    bool indoors = isInsideMansion(camera);

    // Target fog parameters:
    // Outdoors: dense, ominous, thick reddish fog tightly enclosing the mansion
    // Indoors: softer fog that still brings the eerie red haze close to the windows and doors
    float targetStart = indoors ? 80.0f : 15.0f;
    float targetEnd = indoors ? 800.0f : 550.0f;
    float targetDensity = indoors ? 0.0018f : 0.0045f;
    if(mansionInvasionFog&&!indoors){targetStart=15.f;targetEnd=460.f;targetDensity=.0055f;}
    float targetColor[3] = {
        indoors ? 120.0f : 165.0f,
        indoors ? 20.0f : 25.0f,
        indoors ? 20.0f : 25.0f
    };

    // Smoothly transition between indoor and outdoor atmospheres
    float rate = std::min(1.0f, dt * 3.5f);
    currentFogStart += (targetStart - currentFogStart) * rate;
    currentFogEnd += (targetEnd - currentFogEnd) * rate;
    currentFogDensity += (targetDensity - currentFogDensity) * rate;
    for (int i = 0; i < 3; ++i) {
        currentFogColor[i] += (targetColor[i] - currentFogColor[i]) * rate;
    }

    bool enableFog = (!cremationRedFog || cremationRedFog->value > 0.0f);
    if (enableFog && eng.pTriAPI && eng.pTriAPI->Fog) {
        eng.pTriAPI->Fog(currentFogColor, currentFogStart, currentFogEnd, 1);
        if (eng.pTriAPI->FogParams) {
            eng.pTriAPI->FogParams(currentFogDensity, 1);
        }
        weatherFogActive = true;
    } else if (!enableFog && weatherFogActive) {
        float zeroColor[3] = { 0.0f, 0.0f, 0.0f };
        if (eng.pTriAPI && eng.pTriAPI->Fog) {
            eng.pTriAPI->Fog(zeroColor, 0.0f, 0.0f, 0);
        }
        weatherFogActive = false;
    }

    // Rain simulation
    bool enableRain = (!cremationRedRain || cremationRedRain->value > 0.0f);
    if (enableRain) {
        activeRainCount = cremationRainDensity ?
            (std::min)((std::max)(static_cast<int>(cremationRainDensity->value), 50), MAX_RAIN_DROPS) : 200;

        for (int i = 0; i < activeRainCount; ++i) {
            auto& drop = rainDrops[i];
            float dropVelZ = -drop.speed;
            Vector currentVel(rainVel.x, rainVel.y, dropVelZ);
            drop.pos = drop.pos + currentVel * dt;

            float ground = getGroundZ(drop.pos.x, drop.pos.y);
            bool hitGround = (drop.pos.z <= ground);
            bool outOfBounds = (std::abs(drop.pos.x - camera.x) > 680.0f ||
                                std::abs(drop.pos.y - camera.y) > 680.0f ||
                                drop.pos.z < camera.z - 220.0f);

            if (hitGround) {
                if (!isInsideMansion(Vector(drop.pos.x, drop.pos.y, ground + 2.0f))) {
                    spawnSplash(Vector(drop.pos.x, drop.pos.y, ground));
                }
                spawnRainDrop(drop, camera, false);
            } else if (outOfBounds) {
                spawnRainDrop(drop, camera, false);
            }
        }

        // Update ground splash ripples
        for (int i = 0; i < MAX_RAIN_SPLASHES; ++i) {
            auto& sp = rainSplashes[i];
            if (sp.life > 0.0f) {
                sp.life -= dt;
                float progress = 1.0f - (sp.life / sp.maxLife);
                sp.radius = 2.0f + progress * (sp.maxRadius - 2.0f);
            }
        }

        // Ambient drip audio when outside in the rain
        float clientTime = static_cast<float>(eng.GetClientTime());
        if (!indoors && clientTime >= nextDripSoundTime) {
            nextDripSoundTime = clientTime + weatherRandomFloat(1.8f, 3.2f);
            static const char* drips[] = {
                "hl2/ambient/water/rain_drip1.wav",
                "hl2/ambient/water/rain_drip2.wav",
                "hl2/ambient/water/rain_drip3.wav",
                "hl2/ambient/water/rain_drip4.wav"
            };
            int dripIndex = rand() % 4;
            eng.pfnPlaySoundByName(const_cast<char*>(drips[dripIndex]), 0.28f);
        }
    }

    static float weatherReportAt = -100.0f;
    if (time - weatherReportAt > 1.5) {
        weatherReportAt = static_cast<float>(time);
        if (FILE* f = std::fopen("Cremation/diagnostics/weather-diagnostics.txt", "w")) {
            std::fprintf(f, "level=%s mansion=%d indoors=%d fog=%d fogColor=%.1f,%.1f,%.1f fogRange=%.1f..%.1f rain=%d rainCount=%d cam=%.1f,%.1f,%.1f\n",
                eng.pfnGetLevelName() ? eng.pfnGetLevelName() : "unknown",
                isMansionLevel() ? 1 : 0, indoors ? 1 : 0,
                weatherFogActive ? 1 : 0, currentFogColor[0], currentFogColor[1], currentFogColor[2],
                currentFogStart, currentFogEnd,
                enableRain ? 1 : 0, activeRainCount,
                camera.x, camera.y, camera.z);
            std::fclose(f);
        }
    }
}

static void weatherApplyFogOpenGL() {
    if (!wglGetCurrentContext() || !isMansionLevel()) return;
    if (cremationRedFog && cremationRedFog->value <= 0.0f) {
        glDisable(GL_FOG);
        return;
    }
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    float fc[4] = {
        currentFogColor[0] / 255.0f,
        currentFogColor[1] / 255.0f,
        currentFogColor[2] / 255.0f,
        1.0f
    };
    glFogfv(GL_FOG_COLOR, fc);
    glFogf(GL_FOG_START, currentFogStart);
    glFogf(GL_FOG_END, currentFogEnd);
    glFogf(GL_FOG_DENSITY, currentFogDensity);
}

static void drawWeatherRain() {
    if (!wglGetCurrentContext() || !isMansionLevel()) return;
    if (cremationRedRain && cremationRedRain->value <= 0.0f) return;

    Vector camera = weatherCameraOrigin;
    if (!weatherHaveCamera) {
        auto local = eng.GetLocalPlayer();
        if (local) camera = Vector(local->origin[0], local->origin[1], local->origin[2] + 24.0f);
    }

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_LINE_BIT);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glLineWidth(1.8f);

    // Render falling red rain streaks
    glBegin(GL_LINES);
    for (int i = 0; i < activeRainCount; ++i) {
        const auto& drop = rainDrops[i];
        if (isInsideMansion(drop.pos)) continue;

        Vector tail = drop.pos - rainDir * drop.len;
        if (isInsideMansion(tail)) continue;

        float distSq = (drop.pos.x - camera.x) * (drop.pos.x - camera.x) +
                       (drop.pos.y - camera.y) * (drop.pos.y - camera.y) +
                       (drop.pos.z - camera.z) * (drop.pos.z - camera.z);
        if (distSq > 720.0f * 720.0f) continue;

        float distFade = 1.0f;
        if (distSq > 500.0f * 500.0f) {
            distFade = 1.0f - (std::sqrt(distSq) - 500.0f) / 220.0f;
            if (distFade < 0.0f) distFade = 0.0f;
        }

        // Tail: darker, more translucent red
        GLubyte tailAlpha = static_cast<GLubyte>(drop.alpha * distFade * 75.0f);
        glColor4ub(165, 20, 25, tailAlpha);
        glVertex3fv(&tail.x);

        // Head: bright crimson red
        GLubyte headAlpha = static_cast<GLubyte>(drop.alpha * distFade * 215.0f);
        glColor4ub(235, 28, 32, headAlpha);
        glVertex3fv(&drop.pos.x);
    }

    // Render red splash ripples on wet ground
    for (int i = 0; i < MAX_RAIN_SPLASHES; ++i) {
        const auto& sp = rainSplashes[i];
        if (sp.life <= 0.0f) continue;
        if (isInsideMansion(sp.pos)) continue;

        float a = sp.life / sp.maxLife;
        GLubyte alpha = static_cast<GLubyte>(a * 150.0f);
        glColor4ub(220, 30, 35, alpha);

        float r = sp.radius;
        float z = sp.pos.z + 1.2f;

        glVertex3f(sp.pos.x - r, sp.pos.y, z);
        glVertex3f(sp.pos.x + r, sp.pos.y, z);
        glVertex3f(sp.pos.x, sp.pos.y - r, z);
        glVertex3f(sp.pos.x, sp.pos.y + r, z);
    }
    glEnd();

    glPopAttrib();
}

#endif // CLIENT_WEATHER_INC
