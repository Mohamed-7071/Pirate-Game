#include <vector>
#include <ctime>
#include "TextureBuilder.h"
#include "Model_3DS.h"
#include "GLTexture.h"
#include <glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <mmsystem.h>
#include <string>

// Link the Windows Multimedia library for sound
#pragma comment(lib, "winmm.lib")

#define WIDTH 1280
#define HEIGHT 720
#define PI 3.1415926535f

// ===== GAME STATE ENUMERATION =====
enum GameState { MENU, LEVEL_1, WIN, LOSE, LEVEL_2 };
GameState gameState = MENU; // Start in Menu

// ---------------- SOUND CONFIGURATION ----------------
static const wchar_t* ALIAS_MUSIC1 = L"music_lvl1";
static const wchar_t* ALIAS_MUSIC2 = L"music_lvl2";

static const wchar_t* ALIAS_WIN = L"s_win";
static const wchar_t* ALIAS_LOSE = L"s_lose";

static const wchar_t* ALIAS_COIN_PICKUP = L"s_coin";
static const wchar_t* ALIAS_MAP_PICKUP = L"s_ma_pickup";
static const wchar_t* ALIAS_COLLISION = L"s_collision";
static const wchar_t* ALIAS_GAMEOVER = L"s_gameover";
static const wchar_t* ALIAS_NPC_INTERACT = L"s_npc_interact";
static const wchar_t* ALIAS_BOAT_INTERACT = L"s_boat_interact";
static const wchar_t* ALIAS_JUMP = L"s_jump";
static const wchar_t* ALIAS_OOF = L"s_oof";

// --- SOUND PATHS ---
static const wchar_t* PATH_OOF = L"SFX/oof.mp3";
static const wchar_t* PATH_MUSIC1 = L"SFX/music_one.mp3";
static const wchar_t* PATH_MUSIC2 = L"SFX/music_two.mp3";
static const wchar_t* PATH_WIN = L"SFX/win.mp3";
static const wchar_t* PATH_LOSE = L"SFX/lose.mp3";
static const wchar_t* PATH_COIN_PICKUP = L"SFX/Coin-Gem Found.mp3";
static const wchar_t* PATH_MAP_PICKUP = L"SFX/Map Found.mp3";
static const wchar_t* PATH_NPC_INTERACT = L"SFX/Interact With Npc.mp3";
static const wchar_t* PATH_BOAT_INTERACT = L"SFX/Boat-Interact.mp3";
static const wchar_t* PATH_JUMP = L"SFX/Jump.mp3";
static const wchar_t* PATH_COLLISION = L"SFX/hit-soundvideo-game-type-230510.mp3";
static const wchar_t* PATH_GAMEOVER = L"SFX/game-over-38511.mp3";

static bool showBoatInsufficient = false;
static bool soundsInitialized = false;
static bool gameOverSoundPlayed = false;
static bool winSoundPlayed = false;
static bool loseSoundPlayed = false;

static int lastCollisionTime = 0;
const int COLLISION_SOUND_COOLDOWN = 300;

// ---------------- SOUND HELPER FUNCTIONS ----------------
static void MciOpen(const wchar_t* alias, const wchar_t* path) {
    std::wstring cmd = L"open \""; cmd += path; cmd += L"\" type mpegvideo alias "; cmd += alias;
    mciSendStringW(cmd.c_str(), nullptr, 0, nullptr);
}
static void MciClose(const wchar_t* alias) {
    std::wstring cmd = L"close "; cmd += alias; mciSendStringW(cmd.c_str(), nullptr, 0, nullptr);
}
static void MciStop(const wchar_t* alias) {
    std::wstring cmd = L"stop "; cmd += alias; mciSendStringW(cmd.c_str(), nullptr, 0, nullptr);
}
static void MciPlayLoop(const wchar_t* alias) {
    std::wstring cmd = L"play "; cmd += alias; cmd += L" repeat"; mciSendStringW(cmd.c_str(), nullptr, 0, nullptr);
}
static void MciPlayOnce(const wchar_t* alias) {
    std::wstring stopCmd = L"stop "; stopCmd += alias;
    mciSendStringW(stopCmd.c_str(), nullptr, 0, nullptr);
    std::wstring seekCmd = L"seek "; seekCmd += alias; seekCmd += L" to start";
    mciSendStringW(seekCmd.c_str(), nullptr, 0, nullptr);
    std::wstring playCmd = L"play "; playCmd += alias;
    mciSendStringW(playCmd.c_str(), nullptr, 0, nullptr);
}

static void Sound_Init() {
    if (soundsInitialized) return;
    MciOpen(ALIAS_MUSIC1, PATH_MUSIC1);
    MciOpen(ALIAS_MUSIC2, PATH_MUSIC2);
    MciOpen(ALIAS_WIN, PATH_WIN);
    MciOpen(ALIAS_LOSE, PATH_LOSE);

    MciOpen(ALIAS_COIN_PICKUP, PATH_COIN_PICKUP);
    MciOpen(ALIAS_MAP_PICKUP, PATH_MAP_PICKUP);
    MciOpen(ALIAS_COLLISION, PATH_COLLISION);
    MciOpen(ALIAS_GAMEOVER, PATH_GAMEOVER);
    MciOpen(ALIAS_NPC_INTERACT, PATH_NPC_INTERACT);
    MciOpen(ALIAS_BOAT_INTERACT, PATH_BOAT_INTERACT);
    MciOpen(ALIAS_JUMP, PATH_JUMP);
    MciOpen(ALIAS_OOF, PATH_OOF);
    soundsInitialized = true;
}
static void Sound_Shutdown() {
    MciClose(ALIAS_GAMEOVER); MciClose(ALIAS_COLLISION); MciClose(ALIAS_BOAT_INTERACT);
    MciClose(ALIAS_NPC_INTERACT); MciClose(ALIAS_MAP_PICKUP); MciClose(ALIAS_COIN_PICKUP);
    MciClose(ALIAS_JUMP);
    MciClose(ALIAS_WIN);
    MciClose(ALIAS_LOSE);
    MciClose(ALIAS_MUSIC1);
    MciClose(ALIAS_MUSIC2);
    MciClose(ALIAS_OOF);
}


// ---------------- GLOBAL VARIABLES ----------------
GLuint tex;
GLTexture skyboxTexture;
GLTexture groundTexture;
GLTexture tex_menu_bg;
GLTexture tex_play_btn;

GLTexture tex_chest;
GLTexture tex_sun;
GLTexture tex_coin;
GLTexture tex_gem;

GLTexture tex_win_bg;
GLTexture tex_lose_bg;

char title[] = "Pirate's Run - Multi-Level";
static const float LAND_SIZE = 300.0f;
static const float WORLD_SIZE = LAND_SIZE + 100.0f;
static const float GROUND_Y = 0.0f;
static const float WATER_Y = -2.0f;

// --- SUN MOVEMENT VARIABLES ---
float sunAngle = 0.0f;      // Rotation angle
float sunSpeed = 0.3f;      // Speed of day/night cycle
float sunRadius = 250.0f;   // Distance from center

// ---------------- LEVEL 1 OBJECTS ----------------
struct MapInstance { float x, y, z; float scale; float spinDeg; bool placed; };
static MapInstance g_mapRoad = { 0.0f, 1.0f, 0.0f, 2.0f, 0.0f, false };
static bool hasMap = false;

struct NPCInstance { float x, y, z; float yawDeg; float scale; bool placed; };
static NPCInstance g_npc = { 5.0f, 0.0f, 5.0f, 180.0f, 0.02f, true };
struct TreasureInstance { float x, y, z; float yawDeg; float scale; };
static std::vector<TreasureInstance> g_treasures;

static bool showInteractPrompt = false;
static bool showNPCDialogue = false;
static float dialogueTimer = 0.0f;
static const float DIALOGUE_DURATION = 5.0f;
static const float NPC_INTERACT_DISTANCE = 5.0f;

struct BoatInstance { float x, y, z; float yawDeg; float scale; bool placed; };
static BoatInstance g_boat = { 0.0f, GROUND_Y, 0.0f, 0.0f, 1.0f, false };

static bool showBoatPrompt = false;
static bool showBoatDialogue = false;
static bool showBoatDialogue2 = false;
static float boatDialogueTimer = 0.0f;

static const float BOAT_INTERACT_DISTANCE = 30.0f;

static bool paidForBoat = false;
static const int BOAT_COST = 10;

// Level 1 Environment Instances
struct TreeInstance { float x, y, z; float yawDeg; float scale; };
static std::vector<TreeInstance> g_trees;
struct HouseInstance { float x, y, z; float yawDeg; float scale; };
static std::vector<HouseInstance> g_houses;
struct RockInstance { float x, y, z; float yawDeg; float scale; int modelIndex; };
static std::vector<RockInstance> g_rocks;
struct CoinInstance { float x, y, z; float spinDeg; float scale; bool active; };
static std::vector<CoinInstance> g_coins;
struct GemInstance { float x, y, z; float spinDeg; float scale; bool active; };
static std::vector<GemInstance> g_gems;


// Level 1 Platforms
struct Platform { float x, y, z, size; };
static constexpr int PLATFORM_COUNT = 9;
static Platform g_platforms[PLATFORM_COUNT] = {
    { (WORLD_SIZE * 0.5f) - 8.0f, 2.0f, (WORLD_SIZE * 0.5f) - 6.0f, 2.5f },
    { (WORLD_SIZE * 0.5f) + 6.0f, 4.0f, (WORLD_SIZE * 0.5f) - 3.0f, 2.5f },
    { (WORLD_SIZE * 0.5f) - 5.0f, 6.4f, (WORLD_SIZE * 0.5f) + 4.0f, 2.5f },
    { (WORLD_SIZE * 0.5f) + 8.0f, 8.8f, (WORLD_SIZE * 0.5f) + 2.0f, 2.5f },
    { (WORLD_SIZE * 0.5f) - 10.0f, 10.2f, (WORLD_SIZE * 0.5f) - 1.0f, 2.5f },
    { (WORLD_SIZE * 0.5f) + 10.0f, 12.6f, (WORLD_SIZE * 0.5f) - 7.0f, 2.5f },
    { (WORLD_SIZE * 0.5f) - 7.0f, 14.0f, (WORLD_SIZE * 0.5f) + 8.0f, 2.5f },
    { (WORLD_SIZE * 0.5f) + 2.0f, 16.4f, (WORLD_SIZE * 0.5f) + 10.0f, 2.5f },
    { (WORLD_SIZE * 0.5f) - 12.0f, 18.8f, (WORLD_SIZE * 0.5f) + 12.0f, 2.5f },
};

// ---------------- LEVEL 2 OBJECTS ----------------
struct Lvl2Platform { float x, z, width, length, y; };
static std::vector<Lvl2Platform> lvl2_platforms;

struct Pendulum {
    float pivotX, pivotY, pivotZ;
    float length;
    float currentAngle;
    float maxAngle;
    float speed;
    bool axisZ; // true = side-to-side, false = front-to-back
};
static std::vector<Pendulum> lvl2_pendulums;

// Level 1 & 2 Game Objects
struct GameObject { float x, y, z; bool active; float r, g, b; };
GameObject lvl2_key = { 40.0f, 2.0f, 80.0f, true, 1, 1, 0 }; // Level 2 Key

struct Chest { float x, y, z; bool isOpen; };
Chest lvl2_chest = { 0.0f, 2.0f, 130.0f, false }; // Level 2 Chest

bool hasLvl2Key = false;
int score = 1000; // Start at 1000
// --- COIN STARTING VALUE ---
int coinsCollected = 0;
float gameTimer = 0.0f;
int gemsCollected = 0;
// --- LIVES SYSTEM ---
int lives = 5;

// Helper to handle player death (lose a life)
static void HandlePlayerDeath() {
    lives -= 1;
    if (lives < 0) lives = 0;
    // Scoring penalty on death
    score -= 15;
    if (score < 0) score = 0;
    // Play collision sound
    MciPlayOnce(ALIAS_OOF);
    MciPlayOnce(ALIAS_COLLISION);
    if (lives == 0) {
        gameState = LOSE;
        // --- UPDATED: Stop Music on Game Over ---
        MciStop(ALIAS_MUSIC1);
        MciStop(ALIAS_MUSIC2);
        if (!loseSoundPlayed) {
            MciPlayOnce(ALIAS_LOSE); loseSoundPlayed = true;
        }
    }
}

// ---------------- TRANSITION STATE ----------------
static float fadeAlpha = 0.0f;
static bool isFadingOut = false;
static bool isFadingIn = false;

// ---------------- CAMERA & PLAYER STATE ----------------
float playerX = 2.0f;
float playerZ = 2.0f;
float playerY = 0.0f;
float playerYaw = 0.0f;

float camYaw = 0.0f;
float camPitch = 15.0f;
float camDistance = 15.0f;
bool isFirstPerson = false;
bool rotatingCamera = false;
int lastMouseX = -1;
int lastMouseY = -1;
bool isTopDown = false;

float velX = 0.0f, velZ = 0.0f;
float velY = 0.0f;
const float accel = 100.0f;
const float maxSpeed = 24.0f;
const float frictionGround = 100.0f;
const float frictionAir = 75.0f;
const float gravity = 15.0f;
const float jumpImpulse = 7.0f;
const float maxFallSpeed = -15.0f;

bool keyW = false, keyA = false, keyS = false, keyD = false;
bool spaceTrigger = false;
bool grounded = true;
int jumpCount = 0;

// ---------------- MODELS ----------------
Model_3DS model_pirate;
Model_3DS model_key;
Model_3DS model_rocks[5];
Model_3DS model_houses;
Model_3DS model_tree;
Model_3DS model_map;
Model_3DS model_boat;
Model_3DS model_palet;
Model_3DS model_spike;
Model_3DS model_torch;
Model_3DS model_chest_3d;
Model_3DS model_test;

// ---------------- HELPER FUNCTIONS & COLLISION ----------------

static float frand(float minV, float maxV) { return minV + (maxV - minV) * (rand() / (float)RAND_MAX); }

static bool IsOnLevel2Platform(float x, float z) {
    for (const auto& p : lvl2_platforms) {
        float halfW = p.width / 2.0f;
        float halfL = p.length / 2.0f;
        if (x >= (p.x - halfW) && x <= (p.x + halfW) && z >= (p.z - halfL) && z <= (p.z + halfL)) return true;
    }
    return false;
}

static inline bool IsOverLand(float x, float z) {
    if (gameState == LEVEL_2) return IsOnLevel2Platform(x, z);
    return x >= 0.0f && x <= LAND_SIZE && z >= 0.0f && z <= LAND_SIZE;
}

static inline bool CollidesWithTree(float x, float z, float radius) {
    if (gameState != LEVEL_1) return false;
    const float minDist2 = pow(radius + 1.5f, 2);
    for (const auto& t : g_trees) if ((pow(x - t.x, 2) + pow(z - t.z, 2)) < minDist2) return true;
    return false;
}
//static inline bool CollidesWithTree(float x, float z, float radius) {
    //if (gameState != LEVEL_1) return false;
    //const float minDist2 = pow(radius + 1.5f, 2);
    //for (const auto& t : g_trees) if ((pow(x - t.x, 2) + pow(z - t.z, 2)) < minDist2) return true;
    //return false;
//}
static inline bool CollidesWithRock(float x, float z, float radius) {
    if (gameState != LEVEL_1) return false;
    const float minDist2 = pow(radius + 2.0f, 2);
    for (const auto& r : g_rocks) if ((pow(x - r.x, 2) + pow(z - r.z, 2)) < minDist2) return true;
    return false;
}
static inline bool CollidesWithHouse(float x, float z, float radius) {
    if (gameState != LEVEL_1) return false;
    const float minDist2 = pow(radius + 3.5f, 2);
    for (const auto& h : g_houses) if ((pow(x - h.x, 2) + pow(z - h.z, 2)) < minDist2) return true;
    return false;
}
static inline bool CollidesWithBoat(float x, float z, float radius) {
    if (gameState != LEVEL_1 || !g_boat.placed) return false;
    return ((pow(x - g_boat.x, 2) + pow(z - g_boat.z, 2)) < pow(radius + 4.0f, 2));
}
static inline bool CollidesWithNPC(float x, float z, float radius) {
    if (gameState != LEVEL_1 || !g_npc.placed) return false;
    return ((pow(x - g_npc.x, 2) + pow(z - g_npc.z, 2)) < pow(radius + 1.0f, 2));
}

static inline bool CollidesWithAnyObject(float x, float z, float radius) {
    return CollidesWithTree(x, z, radius) || CollidesWithRock(x, z, radius) || CollidesWithHouse(x, z, radius);
}
static inline bool CollidesWithNPCSilent(float x, float z, float radius) {
    return CollidesWithNPC(x, z, radius);
}
static inline bool CollidesWithBoatSilent(float x, float z, float radius) {
    return CollidesWithBoat(x, z, radius);
}

// Helper: Level 2 Pendulum Collision
static bool CollidesWithPendulum(float x, float z, float y, float radius) {
    if (gameState != LEVEL_2) return false;
    for (const auto& p : lvl2_pendulums) {
        float rad = p.currentAngle * PI / 180.0f;
        float spikeX = p.pivotX;
        float spikeY = p.pivotY - p.length * cos(rad);
        float spikeZ = p.pivotZ;

        if (p.axisZ) spikeX += p.length * sin(rad);
        else spikeZ += p.length * sin(rad);

        float dist = sqrt(pow(x - spikeX, 2) + pow(y + 1.0f - spikeY, 2) + pow(z - spikeZ, 2));

        // Extended hitbox radius
        if (dist < (3.5f + radius)) return true;
    }
    return false;
}

// ---------------- LEVEL 1 PLACEMENT FUNCTIONS ----------------

static void PlaceBoatAtEdge() {
    g_boat.x = LAND_SIZE / 2.0f;
    g_boat.z = LAND_SIZE - 5.0f;
    g_boat.y = GROUND_Y + 0.5f;
    g_boat.yawDeg = 180.0f;
    g_boat.scale = 1.0f;
    g_boat.placed = true;
}
static void placeTreasurePiles() {
    g_treasures.clear();
	g_treasures.push_back({ 150.0f, 0.0f, 150.0f, 45.0f, 0.05f });

}

static inline bool InStreetZone(float x, float z) {
    const float centerX = WORLD_SIZE * 0.5f;
    const float centerZ = WORLD_SIZE * 0.5f;
    const float halfX = (16.0f * 0.5f) + 12.0f + 6.0f;
    const float halfZ = (6.0f * 2.0f) + 3.0f;
    return (x >= (centerX - halfX) && x <= (centerX + halfX) && z >= (centerZ - halfZ) && z <= (centerZ + halfZ));
}

static void PlacePirateMapInRoad() {
    const float centerX = (WORLD_SIZE * 0.5f) - 12.0f;
    const float centerZ = (WORLD_SIZE * 0.5f) + 12.0f;
    g_mapRoad.x = centerX; g_mapRoad.z = centerZ; g_mapRoad.y = 20.8f;
    g_mapRoad.scale = 0.1f; g_mapRoad.spinDeg = 0.0f; g_mapRoad.placed = true;
}

void PlaceTreesRandom(int count) {
    g_trees.clear();
    std::vector<TreeInstance> fixedTrees = {
        {20.0f, 0.0f, 20.0f, 45.0f, 0.40f}, {280.0f, 0.0f, 20.0f, 90.0f, 0.40f},
        {20.0f, 0.0f, 280.0f, 135.0f, 0.40f}, {280.0f, 0.0f, 280.0f, 180.0f, 0.40f},
        {35.0f, 0.0f, 45.0f, 225.0f, 0.40f}, {65.0f, 0.0f, 30.0f, 270.0f, 0.40f},
        {50.0f, 0.0f, 75.0f, 315.0f, 0.40f}, {235.0f, 0.0f, 45.0f, 0.0f, 0.40f},
        {265.0f, 0.0f, 30.0f, 45.0f, 0.40f}, {250.0f, 0.0f, 75.0f, 90.0f, 0.40f},
        {35.0f, 0.0f, 255.0f, 135.0f, 0.40f}, {65.0f, 0.0f, 270.0f, 180.0f, 0.40f},
        {50.0f, 0.0f, 225.0f, 225.0f, 0.40f}, {235.0f, 0.0f, 255.0f, 270.0f, 0.40f},
        {265.0f, 0.0f, 270.0f, 315.0f, 0.40f}, {250.0f, 0.0f, 225.0f, 0.0f, 0.40f},
        {30.0f, 0.0f, 100.0f, 45.0f, 0.40f}, {40.0f, 0.0f, 150.0f, 90.0f, 0.40f},
        {30.0f, 0.0f, 200.0f, 135.0f, 0.40f}, {270.0f, 0.0f, 100.0f, 180.0f, 0.40f},
        {260.0f, 0.0f, 150.0f, 225.0f, 0.40f}, {270.0f, 0.0f, 200.0f, 270.0f, 0.40f},
        {80.0f, 0.0f, 25.0f, 315.0f, 0.40f}, {120.0f, 0.0f, 35.0f, 0.0f, 0.40f},
        {160.0f, 0.0f, 30.0f, 45.0f, 0.40f}, {200.0f, 0.0f, 25.0f, 90.0f, 0.40f},
        {220.0f, 0.0f, 40.0f, 135.0f, 0.40f}, {80.0f, 0.0f, 275.0f, 180.0f, 0.40f},
        {120.0f, 0.0f, 285.0f, 225.0f, 0.40f}, {160.0f, 0.0f, 280.0f, 270.0f, 0.40f},
        {200.0f, 0.0f, 275.0f, 315.0f, 0.40f}, {220.0f, 0.0f, 290.0f, 0.0f, 0.40f},
        {90.0f, 0.0f, 90.0f, 45.0f, 0.40f}, {210.0f, 0.0f, 90.0f, 90.0f, 0.40f},
        {90.0f, 0.0f, 210.0f, 135.0f, 0.40f}, {210.0f, 0.0f, 210.0f, 180.0f, 0.40f},
        {75.0f, 0.0f, 130.0f, 225.0f, 0.40f}, {225.0f, 0.0f, 130.0f, 270.0f, 0.40f},
        {75.0f, 0.0f, 170.0f, 315.0f, 0.40f}, {225.0f, 0.0f, 170.0f, 0.0f, 0.40f},
        {110.0f, 0.0f, 60.0f, 45.0f, 0.40f}, {190.0f, 0.0f, 60.0f, 90.0f, 0.40f},
        {110.0f, 0.0f, 240.0f, 135.0f, 0.40f}, {190.0f, 0.0f, 240.0f, 180.0f, 0.40f},
        {140.0f, 0.0f, 100.0f, 225.0f, 0.40f}, {160.0f, 0.0f, 200.0f, 270.0f, 0.40f},
        {55.0f, 0.0f, 110.0f, 315.0f, 0.40f}, {245.0f, 0.0f, 110.0f, 0.0f, 0.40f},
        {55.0f, 0.0f, 190.0f, 45.0f, 0.40f}, {245.0f, 0.0f, 190.0f, 90.0f, 0.40f}
    };
    for (size_t i = 0; i < fixedTrees.size() && i < (size_t)count; ++i) { g_trees.push_back(fixedTrees[i]); }
}

void PlaceCoinsRandom(int count) {
    g_coins.clear();
    const float minDistFromObjects = 2.0f;
    const float minDistBetweenCoins = 3.0f;
    for (int i = 0; i < count; ++i) {
        const int maxTries = 200;
        float x = 0.0f, z = 0.0f;
        bool found = false;
        for (int t = 0; t < maxTries; ++t) {
            x = frand(2.0f, LAND_SIZE - 2.0f); z = frand(2.0f, LAND_SIZE - 2.0f);
            if (!IsOverLand(x, z)) continue;
            if (CollidesWithRock(x, z, minDistFromObjects) || CollidesWithTree(x, z, minDistFromObjects) || CollidesWithHouse(x, z, minDistFromObjects)) continue;
            auto OverlapsAnyCoin = [&](float cx, float cz, float minD) {
                for (const auto& c : g_coins) { float dx = cx - c.x, dz = cz - c.z; if ((dx * dx + dz * dz) < (minD * minD)) return true; } return false;
                };
            if (OverlapsAnyCoin(x, z, minDistBetweenCoins)) continue;
            float dx = x - playerX, dz = z - playerZ; if ((dx * dx + dz * dz) < (minDistFromObjects * minDistFromObjects)) continue;
            found = true; break;
        }
        if (!found) { if (x < 2.0f) x = 2.0f; else if (x > (LAND_SIZE - 2.0f)) x = LAND_SIZE - 2.0f; if (z < 2.0f) z = 2.0f; else if (z > (LAND_SIZE - 2.0f)) z = LAND_SIZE - 2.0f; }
        g_coins.push_back(CoinInstance{ x, 1.0f, z, 0.0f, 0.01f, true });
    }
}

// ---------------- RENDERING PRIMITIVES ----------------

// Simple low-poly gem (octahedron) with texture
static void DrawGem() {
    glEnable(GL_TEXTURE_2D);
    tex_gem.Use();
    glEnable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);

    const float r = 0.6f;   // radius
    const float h = 0.8f;   // half height

    // Top pyramid
    glBegin(GL_TRIANGLES);
    // Four faces around top
    glTexCoord2f(0.5f, 1.0f); glVertex3f(0, h, 0);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(-r, 0, 0);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(0, 0, r);

    glTexCoord2f(0.5f, 1.0f); glVertex3f(0, h, 0);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(0, 0, r);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(r, 0, 0);

    glTexCoord2f(0.5f, 1.0f); glVertex3f(0, h, 0);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(r, 0, 0);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(0, 0, -r);

    glTexCoord2f(0.5f, 1.0f); glVertex3f(0, h, 0);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(0, 0, -r);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(-r, 0, 0);
    glEnd();

    // Bottom pyramid
    glBegin(GL_TRIANGLES);
    glTexCoord2f(0.5f, 0.0f); glVertex3f(0, -h, 0);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(0, 0, r);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(-r, 0, 0);

    glTexCoord2f(0.5f, 0.0f); glVertex3f(0, -h, 0);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(r, 0, 0);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(0, 0, r);

    glTexCoord2f(0.5f, 0.0f); glVertex3f(0, -h, 0);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(0, 0, -r);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(r, 0, 0);

    glTexCoord2f(0.5f, 0.0f); glVertex3f(0, -h, 0);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(-r, 0, 0);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(0, 0, -r);
    glEnd();
}

static void PlaceHousesStreet() {
    g_houses.clear();
    const float centerX = WORLD_SIZE * 0.5f; const float centerZ = WORLD_SIZE * 0.5f;
    const float roadWidth = 16.0f; const float houseOffsetX = 12.0f; const float houseSpacingZ = 6.0f;
    const float rowLeftX = centerX - (roadWidth * 0.5f) - houseOffsetX;
    const float rowRightX = centerX + (roadWidth * 0.5f) + houseOffsetX;
    const float startZ = centerZ - (houseSpacingZ * 2.0f);
    const float y = -0.2f;
    auto yawForIndex = [](int i) -> float { return 0.0f; };
    for (int i = 0; i < 5; ++i) {
        float z = startZ + i * houseSpacingZ; float yaw = yawForIndex(i);
        g_houses.push_back(HouseInstance{ rowLeftX, y, z, yaw, 0.01f });
        g_houses.push_back(HouseInstance{ rowRightX, y, z, yaw, 0.01f });
    }
}

void PlaceRocksRandom(int countPerModel) {
    g_rocks.clear();
    std::vector<RockInstance> fixedRocks = {
        {25.0f, 0.0f, 35.0f, 30.0f, 0.01f, 0}, {275.0f, 0.0f, 35.0f, 60.0f, 0.01f, 1},
        {25.0f, 0.0f, 265.0f, 90.0f, 0.01f, 2}, {275.0f, 0.0f, 265.0f, 120.0f, 0.01f, 3},
        {70.0f, 0.0f, 25.0f, 150.0f, 0.01f, 4}, {150.0f, 0.0f, 30.0f, 180.0f, 0.01f, 0},
        {230.0f, 0.0f, 25.0f, 210.0f, 0.01f, 1}, {70.0f, 0.0f, 275.0f, 240.0f, 0.01f, 2},
        {150.0f, 0.0f, 280.0f, 270.0f, 0.01f, 3}, {230.0f, 0.0f, 275.0f, 300.0f, 0.01f, 4},
        {25.0f, 0.0f, 100.0f, 330.0f, 0.01f, 0}, {30.0f, 0.0f, 180.0f, 0.0f, 0.01f, 1},
        {275.0f, 0.0f, 100.0f, 30.0f, 0.01f, 2}, {280.0f, 0.0f, 180.0f, 60.0f, 0.01f, 3},
        {85.0f, 0.0f, 80.0f, 90.0f, 0.01f, 4}, {215.0f, 0.0f, 80.0f, 120.0f, 0.01f, 0},
        {85.0f, 0.0f, 220.0f, 150.0f, 0.01f, 1}, {215.0f, 0.0f, 220.0f, 180.0f, 0.01f, 2},
        {100.0f, 0.0f, 120.0f, 210.0f, 0.01f, 3}, {200.0f, 0.0f, 120.0f, 240.0f, 0.01f, 4},
        {100.0f, 0.0f, 180.0f, 270.0f, 0.01f, 0}, {200.0f, 0.0f, 180.0f, 300.0f, 0.01f, 1},
        {50.0f, 0.0f, 140.0f, 330.0f, 0.01f, 2}, {250.0f, 0.0f, 140.0f, 0.0f, 0.01f, 3},
        {50.0f, 0.0f, 160.0f, 30.0f, 0.01f, 4}, {250.0f, 0.0f, 160.0f, 60.0f, 0.01f, 0},
        {120.0f, 0.0f, 55.0f, 90.0f, 0.01f, 1}, {180.0f, 0.0f, 55.0f, 120.0f, 0.01f, 2},
        {120.0f, 0.0f, 245.0f, 150.0f, 0.01f, 3}, {180.0f, 0.0f, 245.0f, 180.0f, 0.01f, 4}
    };
    for (const auto& rock : fixedRocks) { g_rocks.push_back(rock); }
}

// ---------------- LEVEL 2 PLACEMENT FUNCTION ----------------

void InitLevel2() {
    lvl2_platforms.clear();
    lvl2_pendulums.clear();

    // 1. STARTING PLATFORM (Center at 0, 0)
    lvl2_platforms.push_back({ 0.0f, 0.0f, 15.0f, 15.0f, 0.0f });

    // 2. THE SPIKE CORRIDOR (Straight path with swings)
    lvl2_platforms.push_back({ 0.0f, 40.0f, 8.0f, 60.0f, 0.0f });

    // Swings (Side-to-Side)
    lvl2_pendulums.push_back({ 0.0f, 15.0f, 20.0f, 12.0f, 0.0f, 60.0f, 3.0f, true });
    lvl2_pendulums.push_back({ 0.0f, 15.0f, 40.0f, 12.0f, 0.0f, 60.0f, 4.0f, true });
    lvl2_pendulums.push_back({ 0.0f, 15.0f, 60.0f, 12.0f, 0.0f, 60.0f, 2.5f, true });

    // 3. THE SPLIT (Safe platform)
    lvl2_platforms.push_back({ 0.0f, 80.0f, 20.0f, 15.0f, 0.0f });

    // 4. THE KEY PATH (Side Platform)
    lvl2_platforms.push_back({ 25.0f, 80.0f, 25.0f, 8.0f, 0.0f }); // Bridge to right
    lvl2_platforms.push_back({ 40.0f, 80.0f, 15.0f, 15.0f, 0.0f }); // Key Platform

    // Set Key Position
    lvl2_key.x = 40.0f; lvl2_key.y = 2.0f; lvl2_key.z = 80.0f;

    // 5. THE FINAL GAUNTLET (To Chest)
    lvl2_platforms.push_back({ 0.0f, 110.0f, 8.0f, 40.0f, 0.0f });

    // More Pendulums (Slightly slower for last two)
    lvl2_pendulums.push_back({ 0.0f, 15.0f, 100.0f, 12.0f, 0.0f, 70.0f, 3.5f, true });
    lvl2_pendulums.push_back({ 0.0f, 15.0f, 120.0f, 12.0f, 0.0f, 70.0f, 3.5f, true });

    // 6. CHEST PLATFORM
    lvl2_platforms.push_back({ 0.0f, 135.0f, 20.0f, 10.0f, 1.0f });
    lvl2_chest.x = 0.0f; lvl2_chest.y = 2.0f; lvl2_chest.z = 135.0f;

    // --- PLACE 10 GEMS (2 per platform on the first 5 platforms) ---
    g_gems.clear();
    const int platformsForGems = 5; // first 5 platforms
    const float margin = 1.0f;
    int placed = 0;
    for (int i = 0; i < (int)lvl2_platforms.size() && i < platformsForGems; ++i) {
        const auto& p = lvl2_platforms[i];
        float hw = p.width * 0.5f - margin;
        float hl = p.length * 0.5f - margin;
        // Two positions per platform: left/right along X, centered on Z
        float gx1 = p.x - hw * 0.5f;
        float gz1 = p.z;
        float gx2 = p.x + hw * 0.5f;
        float gz2 = p.z;
        g_gems.push_back(GemInstance{ gx1, p.y + 1.0f, gz1, 0.0f, 1.0f, true }); placed++;
        g_gems.push_back(GemInstance{ gx2, p.y + 1.0f, gz2, 0.0f, 1.0f, true }); placed++;
        if (placed >= 10) break;
    }
}

// ---------------- RENDERING PRIMITIVES ----------------

// Custom coin rendering function
void DrawCustomCoin() {
    // Bind coin texture and ensure texturing is enabled
    glEnable(GL_TEXTURE_2D);
    tex_coin.Use();
    glColor3f(1.0f, 1.0f, 1.0f); // keep texture colors unchanged

    // Lighting can stay enabled for specular highlights
    glEnable(GL_LIGHTING);

    const float radius = 0.5f;
    const float thickness = 0.15f;
    const int slices = 20;

    // Use a single quadric with texture coordinates enabled
    GLUquadricObj* quadric = gluNewQuadric();
    gluQuadricDrawStyle(quadric, GLU_FILL);
    gluQuadricNormals(quadric, GLU_SMOOTH);
    gluQuadricTexture(quadric, GL_TRUE); // generate texture coords

    // Rim (cylinder)
    gluCylinder(quadric, radius, radius, thickness, slices, 1);

    // Front face (+Z)
    glPushMatrix();
    glTranslatef(0, 0, thickness);
    gluDisk(quadric, 0, radius, slices, 1);
    glPopMatrix();

    // Back face (-Z)
    glPushMatrix();
    glRotatef(180, 1, 0, 0);
    gluDisk(quadric, 0, radius, slices, 1);
    glPopMatrix();

    gluDeleteQuadric(quadric);
}

static void RenderFullScreenTexture(GLTexture& tex) {
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    tex.Use(); glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, WIDTH, 0, HEIGHT);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(1, 0); glVertex2f(WIDTH, 0);
    glTexCoord2f(1, 1); glVertex2f(WIDTH, HEIGHT);
    glTexCoord2f(0, 1); glVertex2f(0, HEIGHT);
    glEnd();
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}

void RenderText(float x, float y, const char* string) {
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    int w = glutGet(GLUT_WINDOW_WIDTH); int h = glutGet(GLUT_WINDOW_HEIGHT);
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glColor3f(1.0f, 1.0f, 1.0f); glRasterPos2f(x, y);
    for (const char* c = string; *c != '\0'; c++) { glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c); }
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glEnable(GL_TEXTURE_2D); glEnable(GL_LIGHTING);
}

// ---------------- SCENE INITIALIZATION & ASSET LOADING ----------------

void myInit(void) {
    glClearColor(0.5f, 0.8f, 0.9f, 0.0f);

    // --- FIX FOR JUMBLED TEXTURES ---
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // -------------------------------

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(45.0, (GLdouble)WIDTH / (GLdouble)HEIGHT, 0.1, 1000);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    // --- ENABLE LIGHTING ---
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0); // Sun Light
    glEnable(GL_LIGHT1); // Torch Light (Player)

    GLfloat a[] = { 0.5f, 0.5f, 0.5f, 1.0f }; glLightfv(GL_LIGHT0, GL_AMBIENT, a);
    GLfloat d[] = { 0.8f, 0.8f, 0.8f, 1.0f }; glLightfv(GL_LIGHT0, GL_DIFFUSE, d);
    glEnable(GL_DEPTH_TEST); glEnable(GL_NORMALIZE); glEnable(GL_COLOR_MATERIAL); glShadeModel(GL_SMOOTH);
    setvbuf(stdout, NULL, _IONBF, 0);
}

void LoadAssets() {
    // ---------------------------------------------------------
    // 1. LOAD PIRATE (Manual Texture Assignment)
    // ---------------------------------------------------------
    model_pirate.Load("models/pirate/Pirate.3ds");
	model_test.Load("models/pirate/Pirate.3ds");    

    // We loop through the materials inside the 3DS file and manually
    // assign the correct BMP image to each index.
    // IF TEXTURES ARE STILL JUMBLED: Swap the filenames below!
    // (e.g., if the Hat looks like a Parrot, swap the file in i==1 with i==3)
    for (int i = 0; i < model_pirate.numMaterials; i++) {
        model_pirate.Materials[i].textured = true; // FORCE TEXTURE ON

        if (i == 1) {
            // Material Index 0: Usually the Main Body or Hat
            // CHANGED ORDER: Trying Hat first based on common export issues
            model_pirate.Materials[i].tex.Load("models/pirate/14051_Pirate_Captain_hat_diff.bmp");
        }
        else if (i == 0) {
            // Material Index 1: Usually Body if 0 was Hat
            model_pirate.Materials[i].tex.Load("models/pirate/14051_Pirate_Captain_body_diff.bmp");
        }
        else if (i == 3) {
            // Material Index 2: Usually the Weapon (Sabre)
            // CHANGED ORDER: Trying Parrot here
            model_pirate.Materials[i].tex.Load("models/pirate/14051_Pirate_Captain_parrot_diff.bmp");
        }
        else if (i == 2) {
            // Material Index 3: Usually the Parrot
            // CHANGED ORDER: Trying Sabre here
            model_pirate.Materials[i].tex.Load("models/pirate/14051_Pirate_Captain_sabre_diff.bmp");
        }
        else {
            // Fallback for any extra geometry (Index 4+)
            model_pirate.Materials[i].tex.Load("models/pirate/14051_Pirate_Captain_body_diff.bmp");
        }
    }
    // ---------------------------------------------------------

    model_key.Load("models/key/Key9.3DS");
    model_map.Load("models/map/map1.3ds");
    model_rocks[0].Load("models/rocks/Rock0.3ds"); model_rocks[1].Load("models/rocks/Rock1.3ds");
    model_rocks[2].Load("models/rocks/Rock2.3ds"); model_rocks[3].Load("models/rocks/Rock3.3ds");
    model_rocks[4].Load("models/rocks/Rock4.3ds");
    model_boat.Load("models/Boat/pirateships.3ds");
    model_palet.Load("models/palet/palet.3ds");
    model_houses.Load("models/medieval-structures-wip/MedievalHouses.3ds");
    //int streetMatIndex = -1;
    //for (int i = 0; i < model_houses.numMaterials; ++i) {
    //   
    //    model_houses.Materials[i].textured = true;
    //    
    //}

    //streetMatIndex = (streetMatIndex == -1 ? 0 : streetMatIndex);


    //GLTexture streetTex;
    //streetTex.Load("textures/ground.bmp"); // your custom street texture
    //model_houses.Materials[streetMatIndex].tex = streetTex;
    //model_houses.Materials[streetMatIndex].textured = true;

    model_tree.Load("models/tree/Tree1.3ds");

    model_torch.Load("models/torch/torch.3ds");

    // --- LOAD SPIKE MODEL FOR LEVEL 2 ---
    model_spike.Load("models/spike/spike.3ds");

    // ---------------------------------------------------------
    // 2. LOAD CHEST (Force Texture)
    // ---------------------------------------------------------
    model_chest_3d.Load("models/chest/chest.3ds");

    // Load the texture into our global GLTexture object
    tex_chest.Load("models/chest/13449_Treasure_Chest_v1_l1.bmp");

    // Force this texture onto EVERY material of the chest
    for (int i = 0; i < model_chest_3d.numMaterials; i++) {
        model_chest_3d.Materials[i].tex = tex_chest;
        model_chest_3d.Materials[i].textured = true;
    }
    // ---------------------------------------------------------

    // --- LOAD SUN MODEL FOR LIGHT SOURCE ---


    skyboxTexture.Load("textures/blu-sky-3.bmp");
    groundTexture.Load("textures/Dirt1.bmp");
    tex_menu_bg.Load("textures/menu_bg.bmp");
    tex_play_btn.Load("textures/play_btn.bmp");

    // --- LOAD SUN SPHERE TEXTURE ---
    tex_sun.Load("textures/sun.bmp");

    tex_coin.Load("textures/gold.bmp");

    tex_gem.Load("textures/gem.bmp");
    gemsCollected = 0;

    tex_win_bg.Load("textures/WIN.bmp");
    tex_lose_bg.Load("textures/LOSE.bmp");

    srand((unsigned)time(nullptr));
    PlaceRocksRandom(6);
    PlaceHousesStreet();
    PlaceTreesRandom(50);
    PlaceCoinsRandom(20);
    PlacePirateMapInRoad();
    PlaceBoatAtEdge();
    placeTreasurePiles();
    InitLevel2();
}

// ---------------- LOGIC UPDATES ----------------

// Reset player state for Level 2 transition/death
void ResetPlayerLvl2() {
    playerX = 0.0f;
    playerZ = 0.0f;
    playerY = 0.0f;
    velX = 0; velZ = 0; velY = 0;
    jumpCount = 0; grounded = true;
}

// --- RESTORED MOMENTUM PHYSICS UPDATE ---
void UpdateMovement(float dt) {
    if (isFadingOut) return;

    // 1. Calculate Input Direction
    float inputX = 0.0f, inputZ = 0.0f;
    const float yawRad = playerYaw * PI / 180.0f;
    const float fx = sinf(yawRad);
    const float fz = cosf(yawRad);
    const float lx = -fz;
    const float lz = fx;

    // Accumulate input
    if (keyW) { inputX -= fx; inputZ -= fz; }
    if (keyS) { inputX += fx; inputZ += fz; }
    if (keyA) { inputX += lx; inputZ += lz; }
    if (keyD) { inputX -= lx; inputZ -= lz; }

    // Normalize input
    float inputLen = sqrt(inputX * inputX + inputZ * inputZ);
    if (inputLen > 0.001f) {
        inputX /= inputLen;
        inputZ /= inputLen;

        // Apply ACCELERATION
        velX += inputX * accel * dt;
        velZ += inputZ * accel * dt;
    }
    else {
        // Apply FRICTION
        float currentSpeed = sqrt(velX * velX + velZ * velZ);
        if (currentSpeed > 0.001f) {
            float frictionToUse = grounded ? frictionGround : frictionAir;
            float drop = frictionToUse * dt;
            float newSpeed = currentSpeed - drop;
            if (newSpeed < 0) newSpeed = 0;

            velX *= (newSpeed / currentSpeed);
            velZ *= (newSpeed / currentSpeed);
        }
        else {
            velX = 0;
            velZ = 0;
        }
    }

    // Cap velocity
    float currentSpeed = sqrt(velX * velX + velZ * velZ);
    if (currentSpeed > maxSpeed) {
        velX *= (maxSpeed / currentSpeed);
        velZ *= (maxSpeed / currentSpeed);
    }

    // 2. Calculate Move Amount
    float moveX = velX * dt;
    float moveZ = velZ * dt;

    // 3. Collision Logic
    bool hitObstacle = false;

    // Check X axis
    float testX = playerX + moveX;
    bool hitWallX = (gameState == LEVEL_1) && (testX < 0.0f || testX > LAND_SIZE);
    bool hitNPC_X = CollidesWithNPCSilent(testX, playerZ, 0.0f) || CollidesWithBoatSilent(testX, playerZ, 0.0f);

    if (CollidesWithAnyObject(testX, playerZ, 0.0f) || hitNPC_X || hitWallX) {
        velX = 0; // Stop momentum on collision
        if (hitWallX) {
            if (testX < 0.0f) playerX = 0.0f;
            if (testX > LAND_SIZE) playerX = LAND_SIZE;
        }
        else if (!hitNPC_X) {
            hitObstacle = true;
        }
    }
    else { playerX = testX; }

    // Check Z axis
    float testZ = playerZ + moveZ;
    bool hitWallZ = (gameState == LEVEL_1) && (testZ < 0.0f || testZ > LAND_SIZE);
    bool hitNPC_Z = CollidesWithNPCSilent(playerX, testZ, 0.0f) || CollidesWithBoatSilent(playerX, testZ, 0.0f);

    if (CollidesWithAnyObject(playerX, testZ, 0.0f) || hitNPC_Z || hitWallZ) {
        velZ = 0; // Stop momentum on collision
        if (hitWallZ) {
            if (testZ < 0.0f) playerZ = 0.0f;
            if (testZ > LAND_SIZE) playerZ = LAND_SIZE;
        }
        else if (!hitNPC_Z) {
            hitObstacle = true;
        }
    }
    else { playerZ = testZ; }

    if (hitObstacle) {
        int currentTime = glutGet(GLUT_ELAPSED_TIME);
        if (currentTime - lastCollisionTime > COLLISION_SOUND_COOLDOWN) {
            MciPlayOnce(ALIAS_COLLISION);
            MciPlayOnce(ALIAS_OOF);
            lastCollisionTime = currentTime;
        }
    }

    // Vertical movement
    velY -= gravity * dt;
    if (velY < maxFallSpeed) velY = maxFallSpeed;

    if (spaceTrigger && jumpCount < 2) {
        velY = jumpImpulse; grounded = false; jumpCount++; spaceTrigger = false; MciPlayOnce(ALIAS_JUMP);
    }

    playerY += velY * dt;

    // Ground/Water/Platform Landing
    if (playerY < GROUND_Y) {
        if (IsOverLand(playerX, playerZ)) {
            playerY = GROUND_Y; velY = 0.0f; grounded = true; jumpCount = 0;
        }
        else {
            grounded = false; velY = (velY > -5.0f) ? velY : -5.0f;

            // --- FIX FOR LEVEL 2 VOID DEATH ---
            if (gameState == LEVEL_2 && playerY < -20.0f) {
                HandlePlayerDeath(); // LOSE A HEART!
                if (lives > 0) ResetPlayerLvl2(); // Only respawn if alive
            }
        }
    }

    // Level 1 Platforms Landing
    if (gameState == LEVEL_1) {
        for (int i = 0; i < PLATFORM_COUNT; ++i) {
            const Platform& p = g_platforms[i]; const float s = p.size; const float topY = p.y;
            if (playerX >= (p.x - s) && playerX <= (p.x + s) && playerZ >= (p.z - s) && playerZ <= (p.z + s)) {
                if (playerY >= topY - 0.5f && playerY <= topY + 1.0f && velY <= 0.0f) {
                    playerY = topY; velY = 0.0f; grounded = true; jumpCount = 0;
                }
            }
        }
    }

    // Level 2 Collision
    if (gameState == LEVEL_2 && CollidesWithPendulum(playerX, playerZ, playerY, 0.0f)) {
        // --- FIX FOR LEVEL 2 LIVES ---
        HandlePlayerDeath(); // LOSE A HEART!
        if (lives > 0) ResetPlayerLvl2(); // Only respawn if alive
    }
}

void CheckGameLogic() {
    float pickupDist = 2.0f;

    if (gameState == LEVEL_1) {
        // Coin/Key/Map Pickups (Level 1)
        for (auto& coin : g_coins) {
            if (coin.active) {
                float d = sqrt(pow(playerX - coin.x, 2) + pow(playerZ - coin.z, 2));
                if (d < pickupDist) { coin.active = false; score += 1; coinsCollected++; MciPlayOnce(ALIAS_COIN_PICKUP); }
            }
        }
        if (g_mapRoad.placed) {
            float d = sqrt(pow(playerX - g_mapRoad.x, 2) + pow(playerZ - g_mapRoad.z, 2));
            if (d < pickupDist) { g_mapRoad.placed = false; hasMap = true; score += 10; MciPlayOnce(ALIAS_MAP_PICKUP); }
        }

        // NPC Interaction Check
        showInteractPrompt = (g_npc.placed && sqrt(pow(playerX - g_npc.x, 2) + pow(playerZ - g_npc.z, 2)) < NPC_INTERACT_DISTANCE);

        // Boat Interaction Check
        showBoatPrompt = (g_boat.placed && sqrt(pow(playerX - g_boat.x, 2) + pow(playerZ - g_boat.z, 2)) < BOAT_INTERACT_DISTANCE);

        // Win condition (Level 1)
        int coinsRemaining = 0;
        for (const auto& coin : g_coins) { if (coin.active) coinsRemaining++; }
    }
    else if (gameState == LEVEL_2) {
        // --- GEM PICKUPS (Level 2) ---
        for (auto& gem : g_gems) {
            if (!gem.active) continue;
            float d = sqrt(pow(playerX - gem.x, 2) + pow(playerZ - gem.z, 2));
            if (d < pickupDist) {
                gem.active = false;
                gemsCollected++;     // increment gem counter
                score += 10;          // optional: reward score for gems
                MciPlayOnce(ALIAS_COIN_PICKUP);
            }
        }

        // Pick up Key (Level 2)
        if (lvl2_key.active) {
            float d = sqrt(pow(playerX - lvl2_key.x, 2) + pow(playerZ - lvl2_key.z, 2));

            // --- FIX: Reduced radius to 2.0f so it doesn't vanish before you reach it ---
            if (d < 2.0f) {
                lvl2_key.active = false;
                hasLvl2Key = true;
                MciPlayOnce(ALIAS_COIN_PICKUP);
            }
        }

        // Open Chest (Level 2 WIN)
        float d = sqrt(pow(playerX - lvl2_chest.x, 2) + pow(playerZ - lvl2_chest.z, 2));
        if (d < 3.0f && hasLvl2Key) {
            gameState = WIN;
            // --- UPDATED: Stop music on win ---
            MciStop(ALIAS_MUSIC2);
            if (!winSoundPlayed) { MciPlayOnce(ALIAS_WIN); winSoundPlayed = true; }
        }
    }
}


// ---------------- RENDERING SCENES ----------------

// Level 1: Draw Platforms (Palets)
static void RenderPaletsOnPlatforms() {
    glEnable(GL_TEXTURE_2D); glEnable(GL_LIGHTING); glColor3f(0.6f, 0.5f, 0.4f);
    for (int i = 0; i < PLATFORM_COUNT; ++i) {
        const Platform& p = g_platforms[i];
        glPushMatrix();
        glTranslatef(p.x, p.y - 0.8f, p.z + 2.6f);
        glRotatef(90.0f, 1, 0, 0);
        float s = p.size * 0.2f; glScalef(s, s, s);
        model_palet.Draw();
        glPopMatrix();
    }
}

void RenderGround() {
    glEnable(GL_TEXTURE_2D); glDisable(GL_LIGHTING); glColor3f(1.0f, 1.0f, 1.0f);
    groundTexture.Use();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    const float tilingFactor = LAND_SIZE / 20.0f;
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0f, GROUND_Y, 0.0f);
    glTexCoord2f(tilingFactor, 0.0f); glVertex3f(LAND_SIZE, GROUND_Y, 0.0f);
    glTexCoord2f(tilingFactor, tilingFactor); glVertex3f(LAND_SIZE, GROUND_Y, LAND_SIZE);
    glTexCoord2f(0.0f, tilingFactor); glVertex3f(0.0f, GROUND_Y, LAND_SIZE);
    glEnd();

    // Water: keep solid blue, untextured
    glDisable(GL_TEXTURE_2D); glColor3f(0.2f, 0.4f, 1.0f);
    glBegin(GL_QUADS); glVertex3f(-100, WATER_Y, -100); glVertex3f(WORLD_SIZE + 100, WATER_Y, -100);
    glVertex3f(WORLD_SIZE + 100, WATER_Y, WORLD_SIZE + 100); glVertex3f(-100, WATER_Y, WORLD_SIZE + 100); glEnd();
    glEnable(GL_LIGHTING);
}

void RenderLevel2() {
    // Draw Platforms
    glEnable(GL_TEXTURE_2D); groundTexture.Use(); glColor3f(0.8f, 0.8f, 0.8f);
    for (const auto& p : lvl2_platforms) {
        float hw = p.width / 2.0f; float hl = p.length / 2.0f; float y = p.y;
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex3f(p.x - hw, y, p.z - hl); glTexCoord2f(1, 0); glVertex3f(p.x + hw, y, p.z - hl);
        glTexCoord2f(1, 1); glVertex3f(p.x + hw, y, p.z + hl); glTexCoord2f(0, 1); glVertex3f(p.x - hw, y, p.z + hl);
        glVertex3f(p.x - hw, y - 2.0f, p.z - hl); glVertex3f(p.x + hw, y - 2.0f, p.z - hl);
        glVertex3f(p.x + hw, y - 2.0f, p.z + hl); glVertex3f(p.x - hw, y - 2.0f, p.z + hl);
        glEnd();

        glDisable(GL_TEXTURE_2D); glColor3f(0.4f, 0.4f, 0.4f);
        glBegin(GL_QUADS);
        glVertex3f(p.x - hw, y, p.z + hl); glVertex3f(p.x + hw, y, p.z + hl); glVertex3f(p.x + hw, y - 2, p.z + hl); glVertex3f(p.x - hw, y - 2, p.z + hl);
        glVertex3f(p.x - hw, y, p.z - hl); glVertex3f(p.x + hw, y, p.z - hl); glVertex3f(p.x + hw, y - 2, p.z - hl); glVertex3f(p.x - hw, y - 2, p.z - hl);
        glVertex3f(p.x - hw, y, p.z - hl); glVertex3f(p.x - hw, y, p.z + hl); glVertex3f(p.x - hw, y - 2, p.z + hl); glVertex3f(p.x - hw, y - 2, p.z - hl);
        glVertex3f(p.x + hw, y, p.z - hl); glVertex3f(p.x + hw, y, p.z + hl); glVertex3f(p.x + hw, y - 2, p.z + hl); glVertex3f(p.x + hw, y - 2, p.z - hl);
        glEnd();
        glEnable(GL_TEXTURE_2D); glColor3f(0.8f, 0.8f, 0.8f);
    }

    for (const auto& gem : g_gems) {
        if (!gem.active) continue;
        glPushMatrix();
        glTranslatef(gem.x, gem.y + 0.3f * sinf(gameTimer * 3.5f), gem.z);
        glRotatef(gem.spinDeg, 0, 1, 0);
        glScalef(1.2f, 1.2f, 1.2f);
        DrawGem();
        glPopMatrix();
    }

    // --- RENDER SPIKE MODELS WITH METALLIC MATERIAL ---
    glDisable(GL_TEXTURE_2D); // Disable texture so material colors work

    // Disable COLOR MATERIAL so the underlying model color doesn't override the Silver
    glDisable(GL_COLOR_MATERIAL);

    for (const auto& p : lvl2_pendulums) {
        glPushMatrix();
        glTranslatef(p.pivotX, p.pivotY, p.pivotZ);
        if (p.axisZ) glRotatef(p.currentAngle, 0, 0, 1);
        else glRotatef(p.currentAngle, 1, 0, 0);

        // --- APPLY METALLIC SILVER MATERIAL ---
        GLfloat mat_ambient[] = { 0.25f, 0.25f, 0.25f, 1.0f };    // Dark Grey base
        GLfloat mat_diffuse[] = { 0.4f, 0.4f, 0.4f, 1.0f };       // Grey color
        GLfloat mat_specular[] = { 0.77f, 0.77f, 0.77f, 1.0f };   // Bright white shine
        GLfloat shine = 76.8f;                                    // High shininess

        glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
        glMaterialf(GL_FRONT, GL_SHININESS, shine);
        // -------------------------------------

        glScalef(0.2f, 0.2f, 0.2f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        model_spike.Draw();
        glPopMatrix();
    }
    // Re-enable generic white material for other objects
    glEnable(GL_COLOR_MATERIAL);
    GLfloat defaultMat[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, defaultMat);

    // --- DRAW LEVEL 2 KEY ---
    if (lvl2_key.active) {
        glPushMatrix();
        // --- ADD HOVER EFFECT (Sine Wave on Y) ---
        float hoverY = lvl2_key.y + 0.5f * sin(gameTimer * 3.0f);
        glTranslatef(lvl2_key.x, hoverY, lvl2_key.z);

        // --- DRAW KEY MODEL (UNLIT YELLOW) ---
        glRotatef(gameTimer * 90.0f, 0, 1, 0);


        glScalef(1.0f, 1.0f, 1.0f);

        glColor3f(1.0f, 0.84f, 0.0f); // Gold Color
        model_key.Draw();

        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glPopMatrix();
    }



    // --- DRAW CHEST 3D MODEL ---
    glEnable(GL_TEXTURE_2D);
    // Explicitly bind the chest texture
    glBindTexture(GL_TEXTURE_2D, tex_chest.texture[0]);

    glPushMatrix();
    glTranslatef(lvl2_chest.x, lvl2_chest.y, lvl2_chest.z);

    // Scale adjustment - BIGGER (Was 0.05f -> now 0.25f)
    glScalef(0.12f, 0.12f, 0.12f);

    // --- ROTATION FIX ---
    // Removed the X-Rotation that made it lie down.
    // Added Y-Rotation 180 to face the camera (player).
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);

    // Reset Material to white so texture colors show correctly
    GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT, white);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, white);

    model_chest_3d.Draw();

    glPopMatrix();

    // Unbind chest texture
    glBindTexture(GL_TEXTURE_2D, 0);

    // --- DRAW CHEST WALLS (Ground Texture) ---
    // 3D walls placed at the edges of the chest platform; opening on the approach side (-Z)
    {
        // Find the platform that contains the chest to place walls at its edges
        float cx = lvl2_chest.x;
        float cz = lvl2_chest.z;
        float px = cx, pz = cz;    // platform center fallback
        float py = GROUND_Y;       // top surface Y
        float width = 12.0f;        // fallback width
        float length = 10.0f;      // fallback length

        for (const auto& p : lvl2_platforms) {
            float hw = p.width * 0.5f;
            float hl = p.length * 0.5f;
            if (fabsf(cx - p.x) <= hw + 0.1f && fabsf(cz - p.z) <= hl + 0.1f) {
                px = p.x; pz = p.z; py = p.y; width = p.width; length = p.length; break;
            }
        }

        float hw = width * 0.5f;
        float hl = length * 0.5f;
        const float wallH = 8.0f;        // wall height
        const float thick = 0.6f;        // wall thickness
        const float repeatX = 2.5f;      // texture tiling along horizontal span
        const float repeatY = 4.0f;      // texture tiling along height

        glEnable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);          // match ground look (unlit textured)
        groundTexture.Use();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glColor3f(1.0f, 1.0f, 1.0f);

        // Helper lambdas to draw a textured box (axis-aligned)
        auto DrawBoxXAligned = [&](float xMin, float xMax, float zMin, float zMax, float yMin, float yMax) {
            // Front (-Z)
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);       glVertex3f(xMin, yMin, zMin);
            glTexCoord2f(repeatX, 0.0f);       glVertex3f(xMax, yMin, zMin);
            glTexCoord2f(repeatX, repeatY);    glVertex3f(xMax, yMax, zMin);
            glTexCoord2f(0.0f, repeatY);    glVertex3f(xMin, yMax, zMin);
            glEnd();
            // Back (+Z)
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);       glVertex3f(xMax, yMin, zMax);
            glTexCoord2f(repeatX, 0.0f);       glVertex3f(xMin, yMin, zMax);
            glTexCoord2f(repeatX, repeatY);    glVertex3f(xMin, yMax, zMax);
            glTexCoord2f(0.0f, repeatY);    glVertex3f(xMax, yMax, zMax);
            glEnd();
            // Left (-X)
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);       glVertex3f(xMin, yMin, zMax);
            glTexCoord2f(repeatX, 0.0f);       glVertex3f(xMin, yMin, zMin);
            glTexCoord2f(repeatX, repeatY);    glVertex3f(xMin, yMax, zMin);
            glTexCoord2f(0.0f, repeatY);    glVertex3f(xMin, yMax, zMax);
            glEnd();
            // Right (+X)
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);       glVertex3f(xMax, yMin, zMin);
            glTexCoord2f(repeatX, 0.0f);       glVertex3f(xMax, yMin, zMax);
            glTexCoord2f(repeatX, repeatY);    glVertex3f(xMax, yMax, zMax);
            glTexCoord2f(0.0f, repeatY);    glVertex3f(xMax, yMax, zMin);
            glEnd();
            // Top (+Y)
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);       glVertex3f(xMin, yMax, zMin);
            glTexCoord2f(repeatX, 0.0f);       glVertex3f(xMax, yMax, zMin);
            glTexCoord2f(repeatX, repeatY);    glVertex3f(xMax, yMax, zMax);
            glTexCoord2f(0.0f, repeatY);    glVertex3f(xMin, yMax, zMax);
            glEnd();
            // Bottom (-Y)
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);       glVertex3f(xMin, yMin, zMax);
            glTexCoord2f(repeatX, 0.0f);       glVertex3f(xMax, yMin, zMax);
            glTexCoord2f(repeatX, repeatY);    glVertex3f(xMax, yMin, zMin);
            glTexCoord2f(0.0f, repeatY);    glVertex3f(xMin, yMin, zMin);
            glEnd();
            };

        // Back wall (placed at +Z edge)
        DrawBoxXAligned(px - hw, px + hw, pz + hl - thick, pz + hl + thick, py, py + wallH);
        // Left wall (placed at -X edge)
        DrawBoxXAligned(px - hw - thick, px - hw + thick, pz - hl, pz + hl, py, py + wallH);
        // Right wall (placed at +X edge)
        DrawBoxXAligned(px + hw - thick, px + hw + thick, pz - hl, pz + hl, py, py + wallH);

        // Restore state for lit, textured models next
        glBindTexture(GL_TEXTURE_2D, 0);
        glEnable(GL_LIGHTING);

        // --- PLACE TWO TORCHES ON BACK WALL (behind chest) ---
        // Mount torches on the inner face of the back wall (+Z edge), facing toward the platform (-Z)
        {
            const float marginX = 3.0f;            // space from side edges (keep torches separated)
            const float mountZ = pz + hl - thick - 0.30f; // push torches further into the room (away from wall)
            const float ty = py + 3.6f;       // height above platform
            const float scale = 0.8f;            // slightly larger torch size

            // Left torch on back wall
            glEnable(GL_TEXTURE_2D);
            glColor3f(1.0f, 1.0f, 1.0f);
            glPushMatrix();
            glTranslatef(px - hw + marginX, ty, mountZ);
            glRotatef(180.0f, 0, 1, 0);   // face inward toward -Z
            glScalef(scale, scale, scale);
            model_torch.Draw();
            glPopMatrix();

            // Right torch on back wall
            glPushMatrix();
            glTranslatef(px + hw - marginX, ty, mountZ);
            glRotatef(180.0f, 0, 1, 0);   // face inward toward -Z
            glScalef(scale, scale, scale);
            model_torch.Draw();
            glPopMatrix();

            // --- DYNAMIC TORCH LIGHTS (very noticeable alternating intensity) ---
            // Use GL_LIGHT1 and GL_LIGHT2 as torch lights positioned at each torch.
            // Alternate intensity every ~0.7s between very low and very high to be obvious.
            glEnable(GL_LIGHT1);
            glEnable(GL_LIGHT2);

            float cycle = 0.6f; // faster, more noticeable pulsing
            int phase = (int)(gameTimer / cycle) % 2; // 0 or 1
            // Base colors
            GLfloat baseHigh[4] = { 1.0f, 0.95f, 0.75f, 1.0f }; // very warm bright
            GLfloat baseLow[4] = { 0.02f, 0.02f, 0.015f, 1.0f }; // near-off

            // Add noticeable flicker jitter using different waveforms
            float t = gameTimer;
            float jitterA = 0.35f * sinf(t * 7.0f) + 0.20f * sinf(t * 10.3f + 0.7f); // stronger flicker
            float jitterB = 0.35f * sinf(t * 7.6f + 1.4f) + 0.20f * sinf(t * 9.7f + 2.2f);
            // Clamp jitter so it doesn't go negative when combined with low values
            auto applyJitter = [](const GLfloat base[4], float jitter, GLfloat out[4]) {
                float f = 1.0f + jitter; if (f < 0.03f) f = 0.03f; if (f > 2.0f) f = 2.0f;
                out[0] = base[0] * f; out[1] = base[1] * f; out[2] = base[2] * f; out[3] = base[3];
                };

            GLfloat diffA[4], diffB[4];
            if (phase == 0) {
                applyJitter(baseHigh, jitterA, diffA); // A bright + flicker
                applyJitter(baseLow, jitterB, diffB); // B dim  + flicker
            }
            else {
                applyJitter(baseLow, jitterA, diffA); // A dim  + flicker
                applyJitter(baseHigh, jitterB, diffB); // B bright+ flicker
            }

            // Positions at each torch
            GLfloat torchA_Pos[4] = { px - hw + marginX, ty, mountZ, 1.0f };
            GLfloat torchB_Pos[4] = { px + hw - marginX, ty, mountZ, 1.0f };

            // Set light parameters (diffuse dominates, make ambient low, specular moderate)
            GLfloat ambLow[4] = { 0.02f, 0.02f, 0.02f, 1.0f };
            GLfloat spec[4] = { 0.6f, 0.55f, 0.5f, 1.0f };

            // Realistic falloff via attenuation
            glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.5f);
            glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.05f);
            glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.01f);
            glLightfv(GL_LIGHT1, GL_POSITION, torchA_Pos);
            glLightfv(GL_LIGHT1, GL_AMBIENT, ambLow);
            glLightfv(GL_LIGHT1, GL_DIFFUSE, diffA);
            glLightfv(GL_LIGHT1, GL_SPECULAR, spec);

            glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 0.5f);
            glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.05f);
            glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.01f);
            glLightfv(GL_LIGHT2, GL_POSITION, torchB_Pos);
            glLightfv(GL_LIGHT2, GL_AMBIENT, ambLow);
            glLightfv(GL_LIGHT2, GL_DIFFUSE, diffB);
            glLightfv(GL_LIGHT2, GL_SPECULAR, spec);

            glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_LIGHTING);
            glDisable(GL_TEXTURE_2D);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            glDepthMask(GL_FALSE);

            auto drawGlow = [](float x, float y, float z, float radius, float r, float g, float b) {
                glPushMatrix();
                glTranslatef(x, y, z);
                glColor4f(r, g, b, 0.9f);
                glBegin(GL_TRIANGLE_FAN);
                glVertex3f(0, 0, 0);
                glColor4f(r, g, b, 0.0f);
                glVertex3f(-radius, -radius, 0);
                glVertex3f(radius, -radius, 0);
                glVertex3f(radius, radius, 0);
                glVertex3f(-radius, radius, 0);
                glVertex3f(-radius, -radius, 0);
                glEnd();
                glPopMatrix();
                };

            float coreA = (phase == 0 ? 1.0f : 0.25f);
            float coreB = (phase == 0 ? 0.25f : 1.0f);
            drawGlow(torchA_Pos[0], torchA_Pos[1], torchA_Pos[2] + 0.05f, 1.2f, coreA, coreA * 0.85f, coreA * 0.6f);
            drawGlow(torchB_Pos[0], torchB_Pos[1], torchB_Pos[2] + 0.05f, 1.2f, coreB, coreB * 0.85f, coreB * 0.6f);

            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            glEnable(GL_LIGHTING);
            glPopAttrib();
        }
    }


    // Draw Water (Abyss)
    glDisable(GL_TEXTURE_2D); glColor3f(0.1f, 0.0f, 0.2f);
    glBegin(GL_QUADS); glVertex3f(-100, WATER_Y - 10, -100); glVertex3f(WORLD_SIZE, WATER_Y - 10, -100);
    glVertex3f(WORLD_SIZE, WATER_Y - 10, WORLD_SIZE); glVertex3f(-100, WATER_Y - 10, WORLD_SIZE); glEnd();
    glEnable(GL_LIGHTING);
}

// --- NEW FUNCTION: Draws a round sky sphere (Skydome) ---
static void RenderSkydome(float radius) {
    glDisable(GL_LIGHTING); glEnable(GL_TEXTURE_2D);
    // Disable depth writing so sky is always "behind" everything
    glDepthMask(GL_FALSE);

    skyboxTexture.Use();
    glColor3f(1.0f, 1.0f, 1.0f);

    GLUquadricObj* q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);
    gluQuadricTexture(q, GL_TRUE); // Enable texture coords for the sphere

    glPushMatrix();
    glRotatef(90, 1, 0, 0); // Rotate to align texture correctly
    // Invert normals to draw on the inside of the sphere
    gluQuadricOrientation(q, GLU_INSIDE);
    // Draw sphere with high slice/stack count for roundness
    gluSphere(q, radius, 32, 32);
    glPopMatrix();

    gluDeleteQuadric(q);

    // Re-enable depth writing
    glDepthMask(GL_TRUE);
    glDisable(GL_TEXTURE_2D); glEnable(GL_LIGHTING);
}

void RenderMenu() {
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    tex_menu_bg.Use(); glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, WIDTH, 0, HEIGHT);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0, 0); glTexCoord2f(1, 0); glVertex2f(WIDTH, 0);
    glTexCoord2f(1, 1); glVertex2f(WIDTH, HEIGHT); glTexCoord2f(0, 1); glVertex2f(0, HEIGHT);
    glEnd();

    tex_play_btn.Use();
    float btnW = 200, btnH = 100; float btnX = (WIDTH - btnW) / 2.0f; float btnY = 100.0f;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(btnX, btnY); glTexCoord2f(1, 0); glVertex2f(btnX + btnW, btnY);
    glTexCoord2f(1, 1); glVertex2f(btnX + btnW, btnY + btnH); glTexCoord2f(0, 1); glVertex2f(btnX, btnY + btnH);
    glEnd();
    glDisable(GL_BLEND);

    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}

void DrawLevelObjects() {
    if (gameState == LEVEL_1) {
        RenderGround();
        RenderPaletsOnPlatforms();
        glColor3f(1.0f, 1.0f, 1.0f); glEnable(GL_TEXTURE_2D);

        // Rocks
        for (const auto& r : g_rocks) { glPushMatrix(); glTranslatef(r.x, r.y, r.z); glRotatef(r.yawDeg, 0, 1, 0); glScalef(r.scale, r.scale, r.scale); model_rocks[r.modelIndex].Draw(); glPopMatrix(); }
        // Houses
        for (const auto& h : g_houses) { glPushMatrix(); glTranslatef(h.x, h.y, h.z); glRotatef(h.yawDeg, 0, 1, 0); glRotatef(90.0f, 1, 0, 0); glScalef(h.scale, h.scale, h.scale); model_houses.Draw(); glPopMatrix(); }
        // Trees
        for (const auto& t : g_trees) { glPushMatrix(); glTranslatef(t.x, t.y, t.z); glRotatef(0, -90, 1, 0); glScalef(t.scale + 1, t.scale + 1, t.scale + 1); model_tree.Draw(); glPopMatrix(); }
        // Coins
        for (const auto& coin : g_coins) {
            if (coin.active) {
                glPushMatrix(); glTranslatef(coin.x, coin.y, coin.z); glRotatef(coin.spinDeg, 0, 1, 0); glScalef(2.0f, 2.0f, 2.0f); DrawCustomCoin(); glPopMatrix();
            }
        }
        //for(const auto& t:g_treasures) {
            
            //glPushMatrix(); glTranslatef(t.x, t.y, t.z); glRotatef(t.yawDeg, 0, 1, 0); glScalef(0.5f, 0.5f, 0.5f); model_test.Draw(); glPopMatrix();
            
		//}
        // Map
        if (g_mapRoad.placed) { glPushMatrix(); glTranslatef(g_mapRoad.x, g_mapRoad.y, g_mapRoad.z); glRotatef(g_mapRoad.spinDeg, 0, 1, 0); glRotatef(45.0f, 1, 0, 0); glScalef(g_mapRoad.scale, g_mapRoad.scale, g_mapRoad.scale); model_map.Draw(); glPopMatrix(); }
        // Boat
        if (g_boat.placed) { glEnable(GL_TEXTURE_2D); glColor3f(0.6f, 0.5f, 0.4f); glPushMatrix(); glTranslatef(g_boat.x, g_boat.y + 10, g_boat.z + 120); glRotatef(g_boat.yawDeg, 0, 1, 0); glScalef(g_boat.scale, g_boat.scale, g_boat.scale); model_boat.Draw(); glPopMatrix(); }
        // NPC
        if (g_npc.placed) { glEnable(GL_TEXTURE_2D); glColor3f(1.0f, 1.0f, 1.0f); glPushMatrix(); glTranslatef(g_npc.x, g_npc.y, g_npc.z); glRotatef(g_npc.yawDeg, 0, 1, 0); glScalef(g_npc.scale, g_npc.scale, g_npc.scale); model_pirate.Draw(); glPopMatrix(); }
    }
    else if (gameState == LEVEL_2) {
        RenderLevel2();
    }
}

void DrawHUD() {
    // HUD background bar (NOW AT BOTTOM: 0 to 80 pixels high)
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, WIDTH, 0, HEIGHT);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glColor4f(0.0f, 0.0f, 0.0f, 0.35f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0); glVertex2f(WIDTH, 0); glVertex2f(WIDTH, 80); glVertex2f(0, 80);
    glEnd();
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);

    // Score and coins (Moved Y coordinates down to fit in bottom bar)
    char scoreText[64]; sprintf(scoreText, "Score: %d", score); RenderText(15, 60, scoreText);
    char coinText[64]; sprintf(coinText, "Coins: %d", coinsCollected); RenderText(15, 35, coinText);

    if (gameState == LEVEL_2) {
        char gemText[64]; sprintf(gemText, "Gems: %d", gemsCollected);
        RenderText(15, 10, gemText);
    }


    // Lives display (hearts) at BOTTOM RIGHT
    int heartCount = lives; if (heartCount < 0) heartCount = 0; if (heartCount > 5) heartCount = 5;
    float hx = WIDTH - 200.0f; float hy = 40.0f;

    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);               // FIX: ensure pure color for hearts
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, WIDTH, 0, HEIGHT);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    for (int i = 0; i < 5; ++i) {
        float x = hx + i * 28.0f; float y = hy;
        if (i < heartCount) glColor4f(1.0f, 0.0f, 0.0f, 0.9f);
        else glColor4f(0.5f, 0.5f, 0.5f, 0.4f);

        glBegin(GL_TRIANGLES);
        glVertex2f(x + 8, y); glVertex2f(x, y - 12); glVertex2f(x + 16, y - 12);
        glVertex2f(x + 8, y - 22); glVertex2f(x, y - 12); glVertex2f(x + 16, y - 12);
        glEnd();
    }

    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);               // restore for subsequent rendering
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);

    if (gameState == LEVEL_1) {
        char timerText[64]; sprintf(timerText, "Time: %.1f", gameTimer); RenderText(15, 10, timerText);
        if (coinsCollected >= BOAT_COST && hasMap) { glColor3f(0.0f, 1.0f, 0.0f); RenderText(10, 100, "You can now go to Level 2! (Go to Boat)"); glColor3f(1, 1, 1); }

        // Dialogue and Prompts
        if (showInteractPrompt && !showNPCDialogue) { glColor3f(1.0f, 1.0f, 0.0f); RenderText(WIDTH / 2 - 80, HEIGHT - 200, "Press E to talk"); glColor3f(1, 1, 1); }
        if (showBoatPrompt && !showBoatDialogue && !showBoatInsufficient && !showBoatDialogue2) { glColor3f(0.0f, 1.0f, 0.0f); RenderText(WIDTH / 2 - 120, HEIGHT - 200, "Press E to pay 10 gold"); glColor3f(1, 1, 1); }
        // NPC Dialogue
        if (showNPCDialogue) {
            glColor4f(0.0f, 0.0f, 0.0f, 0.7f); // Background
            RenderText(WIDTH / 2 - 380, HEIGHT / 2 + 50, "Ahoy there, matey!");
            RenderText(WIDTH / 2 - 380, HEIGHT / 2 + 10, "Find the treasure map in the village and collect");
            RenderText(WIDTH / 2 - 380, HEIGHT / 2 - 30, "at least 10 gold coins to pay for passage on me boat!");
            RenderText(WIDTH / 2 - 380, HEIGHT / 2 - 70, "The map will guide ye to the ancient treasure...");
        }
        // Boat Dialogue (Has enough coins and map - Ready to transition)
        if (showBoatDialogue) {
            glColor4f(0.0f, 0.0f, 0.0f, 0.7f); // Background
            RenderText(WIDTH / 2 - 380, HEIGHT / 2 + 50, "Ahoy there, matey! Ready to set sail!");
            RenderText(WIDTH / 2 - 380, HEIGHT / 2 + 10, "Welcome to Level 2! (Loading...)");
        }
        // Boat Dialogue (Has enough coins, no map)
        if (showBoatDialogue2) {
            glColor4f(0.0f, 0.0f, 0.0f, 0.7f); // Background
            RenderText(WIDTH / 2 - 380, HEIGHT / 2 + 50, "Yer got the gold, matey!");
            RenderText(WIDTH / 2 - 380, HEIGHT / 2 + 10, "but ye still need to find the map.");
            RenderText(WIDTH / 2 - 380, HEIGHT / 2 - 30, "can't set sail without it.");
        }
        // Boat Dialogue (Insufficient coins)
        if (showBoatInsufficient) {
            glColor4f(0.0f, 0.0f, 0.0f, 0.7f); // Background
            RenderText(WIDTH / 2 - 380, HEIGHT / 2 + 50, "Ye be short on coin, lad!");
            RenderText(WIDTH / 2 - 380, HEIGHT / 2 + 10, "Bring me 10 gold coins afore we set sail!");
            RenderText(WIDTH / 2 - 380, HEIGHT / 2 - 30, "Collect more coin aroun' the village.");
            RenderText(WIDTH / 2 - 380, HEIGHT / 2 - 70, "Then we ll chart a course!");
        }
    }
    else if (gameState == LEVEL_2) {
        RenderText(10, 100, "LEVEL 2: SPIKE DUNGEON");
        if (!hasLvl2Key) { glColor3f(1, 0, 0); RenderText(10, 85, "Objective: Find the Key on the side platform!"); glColor3f(1, 1, 1); }
        else { glColor3f(0, 1, 0); RenderText(10, 85, "Objective: Open the Chest!"); glColor3f(1, 1, 1); }
    }
}


void myDisplay(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (gameState == MENU) {
        RenderMenu();
    }
    else if (gameState == LEVEL_1 || gameState == LEVEL_2) {
        // Camera Setup
        glLoadIdentity();
        float eyeX, eyeY, eyeZ, centerX, centerY, centerZ;
        float rad = camYaw * PI / 180.0f;
        float pitchRad = camPitch * PI / 180.0f;
        if (isTopDown) {
            eyeX = playerX + camDistance * sin(rad);
            eyeY = playerY + 20.0f;
            eyeZ = playerZ + camDistance * cos(rad);

            centerX = playerX;
            centerY = playerY + 1.0f;
            centerZ = playerZ;
        }
        else if (isFirstPerson) {
            
            eyeX = playerX;
            eyeY = playerY + 8.2f; 
            eyeZ = playerZ;

            
            centerX = eyeX - 10.0f * sin(rad) * cos(pitchRad);
            centerY = eyeY - 10.0f * sin(pitchRad);
            centerZ = eyeZ - 10.0f * cos(rad) * cos(pitchRad);
        }
        else {
            // Third Person (Standard)
            eyeX = playerX + camDistance * sin(rad);
            eyeY = playerY + 5.0f;
            eyeZ = playerZ + camDistance * cos(rad);

            centerX = playerX;
            centerY = playerY + 1.0f;
            centerZ = playerZ;
        }
        gluLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, 0, 1, 0);

        // --- ROUND SKYDOME ---
        glPushMatrix();
        glTranslatef(LAND_SIZE / 2.0f, 0.0f, LAND_SIZE / 2.0f); // Center on map
        glDisable(GL_DEPTH_TEST);
        // Use new RenderSkydome function instead of Skybox
        RenderSkydome(600.0f);
        glEnable(GL_DEPTH_TEST);
        glPopMatrix();

        // ---------------- DYNAMIC SUN LOGIC ----------------
        // 1. Calculate Sun Position based on angle
        // It rotates around the Z-axis (East-West motion) centered on the map.
        // Center of map is (LAND_SIZE/2, 0, LAND_SIZE/2).
        float mapCenter = LAND_SIZE / 2.0f;
        float sunX = mapCenter + sunRadius * cos(sunAngle); // Moves East/West
        float sunY = sunRadius * sin(sunAngle);             // Moves Up/Down
        float sunZ = mapCenter;                             // Stays centered depth-wise

        // 2. Update the actual Light Source (GL_LIGHT0) position
        GLfloat lightPos[] = { sunX, sunY, sunZ, 1.0f }; // w=1.0f means Positional Light
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

        // 3. Simple Day/Night Cycle Logic (Dim light when under ground)
        if (sunY < -10.0f) {
            // Night time: Dim light to near black
            GLfloat dark[] = { 0.1f, 0.1f, 0.1f, 1.0f };
            glLightfv(GL_LIGHT0, GL_DIFFUSE, dark);
        }
        else {
            // Day time: Bright light
            GLfloat bright[] = { 0.8f, 0.8f, 0.8f, 1.0f };
            glLightfv(GL_LIGHT0, GL_DIFFUSE, bright);
        }

        // --- DRAW SUN AS A YELLOW SPHERE WITH TEXTURE ---
        glPushMatrix();
        glTranslatef(sunX, sunY, sunZ);
        glDisable(GL_LIGHTING); // Disable lighting so it glows
        glEnable(GL_TEXTURE_2D);
        tex_sun.Use(); // Bind the sun texture

        // Use pure white so the texture colors show clearly. 
        // If texture is grayscale, use Yellow (1,1,0) to tint it.
        glColor3f(1.0f, 1.0f, 1.0f);

        // Create Sphere with texture coords
        GLUquadricObj* qSun = gluNewQuadric();
        gluQuadricTexture(qSun, GL_TRUE);
        gluQuadricNormals(qSun, GLU_SMOOTH);

        // Draw Sphere (Radius 40.0f = Nice and Big)
        gluSphere(qSun, 40.0f, 32, 32);

        gluDeleteQuadric(qSun);

        glEnable(GL_LIGHTING);
        glPopMatrix();
        // ---------------------------------------------------

        // Scene Objects
        DrawLevelObjects();

        // --- DRAW PLAYER (PIRATE) ---
        // Only draw in Third Person
        if (!isFirstPerson) {
            glPushMatrix();
            glTranslatef(playerX, playerY, playerZ);
            glRotatef(playerYaw + 180.0f, 0, 1, 0); // Rotate to match camera (Face forward)
            glScalef(0.02f, 0.02f, 0.02f); // Scale down (matches NPC scale)

            // --- FIX FOR MISSING PIRATE TEXTURE IN LEVEL 2 ---
            // Ensure texturing is ON and color is WHITE before drawing the player
            // This fixes it if Level 2 disabled textures for spikes.
            glEnable(GL_TEXTURE_2D);
            glColor3f(1.0f, 1.0f, 1.0f);
            // -------------------------------------------------

            model_pirate.Draw();
            glPopMatrix();
        }

        // HUD
        DrawHUD();

        // FADE SCREEN
        if (fadeAlpha > 0.0f) {
            glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D); glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, WIDTH, 0, HEIGHT);
            glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
            glColor4f(0, 0, 0, fadeAlpha);
            glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f(WIDTH, 0); glVertex2f(WIDTH, HEIGHT); glVertex2f(0, HEIGHT); glEnd();
            glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
            glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
        }

    }
    else {
        // WIN/LOSE Screen
        if (gameState == WIN) {
            RenderFullScreenTexture(tex_win_bg);
        }
        else if (gameState == LOSE) {
            RenderFullScreenTexture(tex_lose_bg);
        }
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, WIDTH, 0, HEIGHT);
        glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
        glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glColor3f(1, 1, 1);
        if (gameState == WIN) {
            RenderText(WIDTH / 2 - 150, HEIGHT / 2, "You claimed the treasure!");
        }
        else {
            RenderText(WIDTH / 2 - 150, HEIGHT / 2, "Out of lives. Better luck next time!");
        }
        char finalScore[64]; sprintf(finalScore, "Final Score: %d", score); RenderText(WIDTH / 2 - 100, HEIGHT / 2 - 40, finalScore);
        RenderText(WIDTH / 2 - 120, HEIGHT / 2 - 80, "Press ESC to exit");
        glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
        glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    }
    glutSwapBuffers();
}


// ---------------- INPUT & ANIMATION ----------------

void myKeyboard(unsigned char button, int x, int y) {
    if (isFadingOut) return;
    switch (button) {
    case 'w': case 'W': keyW = true; break; case 's': case 'S': keyS = true; break;
    case 'a': case 'A': keyA = true; break; case 'd': case 'D': keyD = true; break;
    case ' ': spaceTrigger = true; break; case 'v': case 'V': isFirstPerson = !isFirstPerson; break;
    case 't':case 'T':
		isTopDown = !isTopDown;
		if (isTopDown) isFirstPerson = false;
        break;
    case 'e': case 'E':
        if (gameState == LEVEL_1) {
            if (showInteractPrompt) {
                MciPlayOnce(ALIAS_NPC_INTERACT); showNPCDialogue = true; dialogueTimer = 0.0f;
            }
            else if (showBoatPrompt) {
                MciPlayOnce(ALIAS_BOAT_INTERACT);
                if (coinsCollected >= BOAT_COST) {
                    if (hasMap) {
                        coinsCollected -= BOAT_COST; paidForBoat = true; isFadingOut = true;
                        showBoatDialogue = true; boatDialogueTimer = 0.0f; MciPlayOnce(ALIAS_COIN_PICKUP);
                    }
                    else {
                        showBoatDialogue2 = true; boatDialogueTimer = 0.0f;
                    }
                }
                else {
                    showBoatInsufficient = true; boatDialogueTimer = 0.0f;
                }
            }
        }
        break;
    case 27: Sound_Shutdown(); exit(0); break;
    }
}

void myKeyboardUp(unsigned char button, int x, int y) {
    switch (button) {
    case 'w': case 'W': keyW = false; break; case 's': case 'S': keyS = false; break;
    case 'a': case 'A': keyA = false; break; case 'd': case 'D': keyD = false; break;
    }
}
void myMouse(int button, int state, int x, int y) {
    if (gameState == MENU) {
        if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
            float btnW = 200, btnH = 100; float btnX = (WIDTH - btnW) / 2.0f; float btnY = 100.0f;
            int glY = HEIGHT - y;
            if (x >= btnX && x <= (btnX + btnW) && glY >= btnY && glY <= (btnY + btnH)) {
                gameState = LEVEL_1; // Start Level 1
                // Reset player to Level 1 start pos
                playerX = 2.0f; playerZ = 2.0f; playerY = 0.0f;
                // Reset game state for new run
                score = 1000; // Start at 1000 on new game
                coinsCollected = 0; // START WITH 10 COINS AS REQUESTED
                gameTimer = 0.0f;
                lives = 5; // START WITH 5 HEARTS
                showNPCDialogue = false; showBoatDialogue = false; showBoatDialogue2 = false; showBoatInsufficient = false;

                // --- UPDATED: Ensure Level 1 music starts ---
                MciStop(ALIAS_MUSIC2); // Stop level 2 if it was playing
                MciPlayLoop(ALIAS_MUSIC1);
            }
        }
    }
    else { // When in LEVEL_1 or LEVEL_2
        if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
            // Toggle camera perspective on left click
            isFirstPerson = !isFirstPerson;
        }

        // Original right-click camera rotation logic
        if (button == GLUT_RIGHT_BUTTON) {
            rotatingCamera = (state == GLUT_DOWN);
            lastMouseX = x;
            lastMouseY = y;
        }
    }
}

void myMotion(int x, int y) {
    if (!rotatingCamera) return;

    // Sensitivity factor (0.5f). Increase to make mouse faster.
    camYaw += (x - lastMouseX) * 0.5f;
    camPitch -= (y - lastMouseY) * 0.5f;

    lastMouseX = x;
    lastMouseY = y;

    // --- FIX: Allow looking up/down freely (Limits: -89 to 89) ---
    if (camPitch > 89.0f) camPitch = 89.0f;    // Look down limit
    if (camPitch < -89.0f) camPitch = -89.0f; // Look up limit (was -10)

    playerYaw = camYaw;
}

void myReshape(int w, int h) {
    if (h == 0) h = 1; glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    // --- INCREASED FAR PLANE TO 1000 ---
    gluPerspective(45.0, (GLdouble)w / (GLdouble)h, 0.1, 1000);
    glMatrixMode(GL_MODELVIEW);
}

void Anim() {
    static int lastT = -1; int t = glutGet(GLUT_ELAPSED_TIME);
    if (lastT == -1) lastT = t; float dt = (t - lastT) / 1000.0f; if (dt > 0.05f) dt = 0.05f; lastT = t;
    // decrease score over time: -1 point/sec, clamp at 0
    static float scoreAccum = 0.0f;
    if (gameState == LEVEL_1 || gameState == LEVEL_2) {
        scoreAccum += dt;
        while (scoreAccum >= 1.0f) {
            if (score > 0) score -= 1;
            scoreAccum -= 1.0f;
        }
    }

    // Transition Logic
    if (isFadingOut) {
        fadeAlpha += dt * 0.8f;
        if (fadeAlpha >= 1.0f) {
            fadeAlpha = 1.0f; isFadingOut = false; gameState = LEVEL_2; isFadingIn = true;
            ResetPlayerLvl2();

            // --- UPDATED: Switch Music when Level Changes ---
            MciStop(ALIAS_MUSIC1);      // Stop Level 1 Music
            MciPlayLoop(ALIAS_MUSIC2);  // Start Level 2 Music
            // ------------------------------------------------
        }
    }
    else if (isFadingIn) {
        fadeAlpha -= dt * 0.8f; if (fadeAlpha <= 0.0f) { fadeAlpha = 0.0f; isFadingIn = false; }
    }

    if (gameState == LEVEL_1 || gameState == LEVEL_2) {
        // --- FIX: Update Timer in BOTH levels so rotation works ---
        gameTimer += dt;

        // --- SUN MOVEMENT UPDATE ---
        sunAngle += sunSpeed * dt;
        if (sunAngle > 360.0f) sunAngle -= 360.0f;
        // ---------------------------

        if (gameState == LEVEL_1) {
            if (g_mapRoad.placed) { g_mapRoad.spinDeg += 60.0f * dt; if (g_mapRoad.spinDeg > 360.0f) g_mapRoad.spinDeg -= 360.0f; }
            for (auto& coin : g_coins) { coin.spinDeg += 120.0f * dt; if (coin.spinDeg > 360.0f) coin.spinDeg -= 360.0f; }
            // Dialogue Timers
            if (showNPCDialogue) { dialogueTimer += dt; if (dialogueTimer >= DIALOGUE_DURATION) showNPCDialogue = false; }
            if (showBoatDialogue || showBoatDialogue2 || showBoatInsufficient) { boatDialogueTimer += dt; if (boatDialogueTimer >= DIALOGUE_DURATION) { showBoatDialogue = false; showBoatDialogue2 = false; showBoatInsufficient = false; } }

        }
        else if (gameState == LEVEL_2) {
            for (auto& p : lvl2_pendulums) { p.currentAngle = p.maxAngle * sin(t / 1000.0f * p.speed); }
            // Spin gems in level 2
            for (auto& gem : g_gems) { gem.spinDeg += 90.0f * dt; if (gem.spinDeg > 360.0f) gem.spinDeg -= 360.0f; }
        }


        UpdateMovement(dt); CheckGameLogic();
    }
    glutPostRedisplay();
}

void main(int argc, char** argv) {
    glutInit(&argc, argv); glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT); glutInitWindowPosition(100, 150); glutCreateWindow(title);
    glutDisplayFunc(myDisplay); glutKeyboardFunc(myKeyboard); glutKeyboardUpFunc(myKeyboardUp);
    glutMouseFunc(myMouse); glutMotionFunc(myMotion); glutReshapeFunc(myReshape); glutIdleFunc(Anim);
    myInit(); LoadAssets(); Sound_Init();

    // --- UPDATED: Start with Level 1 Music ---
    MciPlayLoop(ALIAS_MUSIC1);

    glutMainLoop();
    Sound_Shutdown();
}