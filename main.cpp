#include <windows.h>
#include <GL/glut.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#define PI 3.141516f

//S -> ship off (Epu),  L -> plane lands (Epu),  E -> stop ship (Epu)
//R -> storm + station (Rumi),  T -> traffic stop / go (Rumi)
//N -> day / night (Tonmoy),  W -> windmill speed (Mridul),  B -> city lights (Faysal)
//SPACE -> pause / resume,  LEFT -> rewind,  RIGHT -> next scene,  ESC -> exit
//SOUND : needs  -lwinmm  in  Project -> Build options -> Linker settings


typedef DWORD TimeStamp;

//time
static TimeStamp NowMs() { return GetTickCount(); }
static double secondsSince(TimeStamp t) {
    return (double)(GetTickCount() - t) / 1000.0;
}

//circle
void Circle(float r, float x, float y) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 100; i++) {
        float angle = 2.0f * PI * i / 100;
        glVertex2f(x + r * cos(angle), y + r * sin(angle));
    }
    glEnd();
}

//rectangle
void Rect(float x1, float y1, float x2, float y2) {
    glBegin(GL_POLYGON);
    glVertex2f(x1, y1); glVertex2f(x2, y1);
    glVertex2f(x2, y2); glVertex2f(x1, y2);
    glEnd();
}

//line
void Line(float x1, float y1, float x2, float y2) {
    glBegin(GL_LINES);
    glVertex2f(x1, y1); glVertex2f(x2, y2);
    glEnd();
}

//text
void DrawText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++)
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
}

//math
float Lerp(float a, float b, float t) { return a + (b - a) * t; }
float Clamp01(float t) { return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t); }
float SmoothStep(float t) { t = Clamp01(t); return t * t * (3.0f - 2.0f * t); }

//shadow
void drawShadow(float cx, float cy, float rx, float ry) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.22f);
    glPushMatrix();
    glTranslatef(cx, cy, 0.0f);
    glScalef(rx, ry, 1.0f);
    Circle(1.0f, 0.0f, 0.0f);
    glPopMatrix();
    glDisable(GL_BLEND);
}

enum Scene { SCENE1, SCENE2, SCENE3, SCENE4 };
Scene scene = SCENE1;

float sceneIntensity = 1.0f;
float waterPhase = 0.0f;
float bladeAngle = 0.0f;

//tonmoy
float sunAngle = 0.0f;

//mridul
float bladeSpeed = 3.0f;
int   windState = 1;

//faysal
bool  lightsOn = true;

//rumi
bool  trafficMoving = true;
const float WINDOW_ASPECT = 1000.0f / 500.0f;

float reverseHoldTimer = 0.0f;
bool  Reversing() { return reverseHoldTimer > 0.0f; }

//pause
bool      paused = false;
TimeStamp pauseStartTime;

//ship
float shipX = 0.0f;
float shipDY = 0.0f;
bool  shipMoving = false;
bool  shipStoppedInCity = false;
bool  scene1ShipDeparted = false;
const float SHIP_SPEED = 0.0026f;

//train
bool  trainMoving = false;
bool  raining = false;
bool  trainDecelerating = false;
bool  trainAtStation = false;
float trainX = 0.0f;
float trainSpeed = 0.0f;
float wheelAngle = 0.0f;
TimeStamp decelStartTime;
float decelStartX = 0.0f;
const float TRAIN_START_X      = -1.70f;
const float STATION_STOP_X     = 0.85f;
const float STATION_APPROACH_X = 0.30f;
const float BASE_TRAIN_SPEED   = 0.0018f;
double decelDurationSeconds = 4.0;

//airplane
bool  airplaneMoving = false;
bool  airplaneLanding = false;
bool  airplaneVisible = true;
float planeX = -1.4f, planeY = 0.55f, planeTilt = 0.0f, planeScale = 1.0f;
TimeStamp landingStartTime;
float planeLandStartX, planeLandStartY;
const float PLANE_CRUISE_Y = 0.55f;
const float PLANE_SPEED = 0.0032f;
const float LANDING_TARGET_X = -0.55f;
const float LANDING_TARGET_Y = -0.06f;
const double LANDING_DURATION_SECONDS = 3.0;
float birdPhase = 0.0f;

//weather
float weatherLevel = 0.0f;
struct RainDrop { float x, y, len; };
const int NUM_DROPS = 160;
RainDrop drops[NUM_DROPS];
float lightningTimer = 0.0f;
bool  lightningFlash = false;
float lightningFlashTimer = 0.0f;
float boltPoints[6][2];

//city
float dayNightT = 0.0f;
float windowBlinkTimer = 0.0f;
bool  windowBlinkOn = true;
struct Vehicle { float x, speed, y, w, h; };
Vehicle carA, carB, carC, brtcBus, aiubBus, cityBus;
const float CITY_ROAD_TOP = -0.34f, CITY_ROAD_BOTTOM = -0.58f;
const float CITY_GROUND_Y = -0.30f;
const float CITY_WATER_TOP = -0.58f, CITY_WATER_BOTTOM = -1.0f;

//scene 2
const float S2_SKY_BOTTOM       = -0.08f;
const float S2_HILL_BACK_BASE   = -0.14f, S2_HILL_BACK_APEX  = 0.04f;
const float S2_HILL_FRONT_BASE  = -0.18f, S2_HILL_FRONT_APEX = -0.02f;
const float S2_GRASS_TOP        = -0.22f, S2_GRASS_BOTTOM    = -0.42f;
const float S2_RAIL_BED_TOP     = -0.42f, S2_RAIL_BED_BOTTOM = -0.62f;
const float S2_RAIL_TOP_Y       = -0.50f, S2_RAIL_LOW_Y      = -0.52f;
const float S2_WATER_TOP        = -0.62f, S2_WATER_BOTTOM    = -1.0f;
const float S2_POLE_TOP         = 0.34f;
const float S2_MESSENGER_WIRE_Y = 0.30f;
const float S2_CONTACT_WIRE_Y   = 0.14f;
const float S2_STATION_X0 = 0.55f, S2_STATION_X1 = 1.35f;

//scene 3
const float S3_SKY_BOTTOM      = -0.08f;
const float S3_HILL_BACK_BASE  = -0.14f, S3_HILL_BACK_APEX  = 0.04f;
const float S3_HILL_FRONT_BASE = -0.18f, S3_HILL_FRONT_APEX = -0.02f;
const float S3_GRASS_TOP       = -0.22f, S3_GRASS_BOTTOM    = -0.42f;
const float S3_WATER_TOP       = -0.62f, S3_WATER_BOTTOM    = -1.0f;


#include <mmsystem.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

const char* KEY_SHIP  = "cruise";
const char* KEY_TRAIN = "train";
const char* KEY_STORM = "thunder";
const char* KEY_PLANE = "small-air";
const char* KEY_LAND  = "landing";
const char* KEY_CITY  = "horn";

char  soundDir[512] = "";
bool  soundReady = false;
const char* loopAmb  = 0;
const char* loopWx   = 0;
const char* loopShip = 0;


//sound

//folder
static bool isDir(const char* p) {
    DWORD a = GetFileAttributesA(p);
    return (a != INVALID_FILE_ATTRIBUTES) && (a & FILE_ATTRIBUTE_DIRECTORY);
}

//sound folder
static void findSoundDir() {
    char base[512], test[900];
    GetModuleFileNameA(NULL, base, sizeof(base));
    char* s = strrchr(base, '\\');
    if (s) *(s + 1) = '\0'; else base[0] = '\0';

    const char* c[4] = { "Sound", "..\\Sound", "..\\..\\Sound", "..\\..\\..\\Sound" };
    for (int i = 0; i < 4; i++) {
        sprintf(test, "%s%s", base, c[i]);
        if (isDir(test)) { sprintf(soundDir, "%s%s\\", base, c[i]); return; }
    }
    for (int i = 0; i < 4; i++) {
        if (isDir(c[i])) { sprintf(soundDir, "%s\\", c[i]); return; }
    }
    soundDir[0] = '\0';
}

//find clip
static bool findClip(const char* keyword, char* outName) {
    char pattern[600];
    WIN32_FIND_DATAA fd;
    sprintf(pattern, "%s*", soundDir);
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return false;
    bool found = false;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        char low[300];
        strncpy(low, fd.cFileName, sizeof(low) - 1);
        low[sizeof(low) - 1] = '\0';
        for (char* p = low; *p; p++) *p = (char)tolower((unsigned char)*p);
        const char* ext = strrchr(low, '.');
        if (!ext) continue;
        if (strcmp(ext, ".wav") != 0 && strcmp(ext, ".mp3") != 0) continue;
        if (strstr(low, keyword)) { strcpy(outName, fd.cFileName); found = true; break; }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return found;
}

//open clip
static void openClip(const char* keyword, const char* alias) {
    char file[300], cmd[900];
    if (!findClip(keyword, file)) return;
    const char* ext = strrchr(file, '.');
    bool wav = (ext && (strcmp(ext, ".wav") == 0 || strcmp(ext, ".WAV") == 0));
    sprintf(cmd, "open \"%s%s\" type %s alias %s", soundDir, file,
            wav ? "waveaudio" : "mpegvideo", alias);
    if (mciSendStringA(cmd, NULL, 0, NULL) != 0) {
        sprintf(cmd, "open \"%s%s\" alias %s", soundDir, file, alias);
        mciSendStringA(cmd, NULL, 0, NULL);
    }
}

//play sound
void playSound(const char* alias, bool loop) {
    if (!soundReady) return;
    char cmd[256];
    if (loop) {
        sprintf(cmd, "play %s from 0 repeat", alias);
        if (mciSendStringA(cmd, NULL, 0, NULL) == 0) return;
    }
    sprintf(cmd, "play %s from 0", alias);
    mciSendStringA(cmd, NULL, 0, NULL);
}

//stop sound
void stopSound(const char* a) { if (!soundReady) return; char c[256]; sprintf(c, "stop %s", a); mciSendStringA(c, NULL, 0, NULL); }

//pause one
static void pauseOne(const char* a) {
    char c[256];
    sprintf(c, "pause %s", a);
    if (mciSendStringA(c, NULL, 0, NULL) != 0) {
        sprintf(c, "stop %s", a);
        mciSendStringA(c, NULL, 0, NULL);
    }
}

//resume one
static void resumeOne(const char* a) { char c[256]; sprintf(c, "resume %s", a); mciSendStringA(c, NULL, 0, NULL); }

//is playing
static bool isPlaying(const char* alias) {
    char buf[64] = "", cmd[256];
    sprintf(cmd, "status %s mode", alias);
    mciSendStringA(cmd, buf, sizeof(buf), NULL);
    return strcmp(buf, "playing") == 0;
}

//sound start
void initSound() {
    findSoundDir();
    if (soundDir[0] == '\0') { soundReady = false; return; }
    openClip(KEY_SHIP,  "ship");
    openClip(KEY_TRAIN, "train");
    openClip(KEY_STORM, "storm");
    openClip(KEY_PLANE, "plane");
    openClip(KEY_LAND,  "land");
    openClip(KEY_CITY,  "city");
    soundReady = true;
}

//thunder off
void stopStormSound() {
    if (!soundReady) return;
    stopSound("storm");
    loopWx = 0;
}

//ship sound on
void shipSoundOn() {
    if (!soundReady || loopShip) return;
    loopShip = "ship";
    playSound("ship", true);
}

//ship sound off
void shipSoundOff() {
    if (!soundReady || !loopShip) return;
    loopShip = 0;
    stopSound("ship");
}

//scene sound
void sceneAudio(int sc) {
    if (!soundReady) return;
    stopSound("train"); stopSound("plane"); stopSound("city");
    loopAmb = 0;
    if (sc == 2)      { loopAmb = "train"; playSound("train", true); }
    else if (sc == 3) { loopAmb = "plane"; playSound("plane", true); }
    else if (sc == 4) { loopAmb = "city";  playSound("city",  true); stopStormSound(); }
    else              { stopStormSound(); }
}

//loop sound
void keepLooping() {
    static int tick = 0;
    if (!soundReady) return;
    if (++tick < 10) return;
    tick = 0;
    if (loopAmb  && !isPlaying(loopAmb))  playSound(loopAmb,  true);
    if (loopWx   && !isPlaying(loopWx))   playSound(loopWx,   true);
    if (loopShip && !isPlaying(loopShip)) playSound(loopShip, true);
}

//pause sound
void pauseAllSound() {
    if (!soundReady) return;
    pauseOne("ship"); pauseOne("train"); pauseOne("storm");
    pauseOne("plane"); pauseOne("land");  pauseOne("city");
}

//resume sound
void resumeAllSound() {
    if (!soundReady) return;
    resumeOne("ship"); resumeOne("train"); resumeOne("storm");
    resumeOne("plane"); resumeOne("land");  resumeOne("city");
}

//close sound
void closeAllSound() {
    if (!soundReady) return;
    mciSendStringA("close all", NULL, 0, NULL);
    soundReady = false;
}


//part 1 - tonmoy

//sun rays
void T_sunRays(float cx, float cy, float r) {
    if (r < 0.01f) return;
    glPushMatrix();
    glTranslatef(cx, cy, 0.0f);
    glScalef(1.0f, WINDOW_ASPECT, 1.0f);
    glRotatef(sunAngle, 0.0f, 0.0f, 1.0f);
    glColor3f(1.0f, 0.82f, 0.15f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (int k = 0; k < 12; k++) {
        float a = k * 30.0f * PI / 180.0f;
        glVertex2f(r * 1.30f * cos(a), r * 1.30f * sin(a));
        glVertex2f(r * 1.75f * cos(a), r * 1.75f * sin(a));
    }
    glEnd();
    glLineWidth(1.0f);
    glPopMatrix();
}

//day night key
void T_toggleDayNight() {
    if (scene != SCENE4) return;
    dayNightT = (dayNightT > 0.5f) ? 0.0f : 1.0f;
}

//sky
void T_scene1Background() {
    glColor3f(0.55f, 0.78f, 0.94f);
    glBegin(GL_POLYGON);
    glVertex2f(-1.0f, -0.3f); glVertex2f(1.0f, -0.3f);
    glVertex2f(1.0f, 1.0f);   glVertex2f(-1.0f, 1.0f);
    glEnd();

    glColor3f(0.06f, 0.28f, 0.55f);
    glBegin(GL_POLYGON);
    glVertex2f(-1.0f, -1.0f); glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, -0.4f);  glVertex2f(-1.0f, -0.4f);
    glEnd();

    glColor3f(0.45f, 0.42f, 0.38f);
    glBegin(GL_POLYGON);
    glVertex2f(-1.0f, -0.4f); glVertex2f(1.0f, -0.4f);
    glVertex2f(1.0f, -0.3f);  glVertex2f(-1.0f, -0.3f);
    glEnd();

    glColor3f(0.25f, 0.22f, 0.2f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (float x = -1.0f; x <= 1.0001f; x += 0.08f) {
        glVertex2f(x, -0.4f); glVertex2f(x, -0.3f);
    }
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(-1.0f, -0.35f);
    glVertex2f(1.0f, -0.35f);
    glEnd();

    glColor3f(1.0f, 0.87f, 0.2f);
    Circle(0.07f, 0.24f, 0.6f);
    T_sunRays(0.24f, 0.6f, 0.07f);
}

//clouds
void T_clouds1() {
    glColor3f(1.0f, 1.0f, 1.0f);
    Circle(0.028f, -0.796f, 0.76f);
    Circle(0.036f, -0.76f, 0.784f);
    Circle(0.028f, -0.724f, 0.76f);

    Circle(0.0224f, -0.3688f, 0.86f);
    Circle(0.0288f, -0.34f, 0.8792f);
    Circle(0.0224f, -0.3112f, 0.86f);

    Circle(0.0308f, 0.5204f, 0.72f);
    Circle(0.0396f, 0.56f, 0.7464f);
    Circle(0.0308f, 0.5996f, 0.72f);

    Circle(0.0196f, 0.7948f, 0.88f);
    Circle(0.0252f, 0.82f, 0.8968f);
    Circle(0.0196f, 0.8452f, 0.88f);

    Circle(0.02f, -0.914f, 0.92f);
    Circle(0.024f, -0.886f, 0.932f);

    Circle(0.026f, -0.07f, 0.84f);
    Circle(0.026f, -0.01f, 0.84f);
    Circle(0.022f, -0.04f, 0.8f);
    Circle(0.034f, -0.04f, 0.872f);

    Circle(0.02f, 0.23f, 0.9f);
    Circle(0.026f, 0.264f, 0.916f);
    Circle(0.03f, 0.30f, 0.924f);
    Circle(0.026f, 0.336f, 0.916f);
    Circle(0.02f, 0.37f, 0.9f);

    Circle(0.018f, 0.936f, 0.752f);
    Circle(0.016f, 0.984f, 0.744f);
    Circle(0.02f, 0.97f, 0.8f);
    Circle(0.028f, 0.96f, 0.776f);
}

//ship
void T_ship1() {
    glColor3f(0.15f, 0.35f, 0.45f);
    glBegin(GL_POLYGON);
    glVertex2f(-0.791f, -0.876f); glVertex2f(-0.609f, -0.876f);
    glVertex2f(-0.6272f, -0.928f); glVertex2f(-0.7728f, -0.928f);
    glEnd();
    glColor3f(0.05f, 0.05f, 0.08f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.791f, -0.876f); glVertex2f(-0.609f, -0.876f);
    glVertex2f(-0.6272f, -0.928f); glVertex2f(-0.7728f, -0.928f);
    glEnd();

    glColor3f(0.75f, 0.1f, 0.1f);
    Rect(-0.7728f, -0.928f, -0.6272f, -0.915f);

    glColor3f(0.95f, 0.95f, 0.95f);
    Rect(-0.76824f, -0.876f, -0.63176f, -0.8188f);
    glColor3f(0.05f, 0.05f, 0.08f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.76824f, -0.876f); glVertex2f(-0.63176f, -0.876f);
    glVertex2f(-0.63176f, -0.8188f); glVertex2f(-0.76824f, -0.8188f);
    glEnd();

    glColor3f(0.3f, 0.6f, 0.9f);
    for (int i = 0; i < 8; i++) {
        float wx = -0.75784f + i * 0.0156f;
        Rect(wx, -0.85312f, wx + 0.0065f, -0.84012f);
    }

    glColor3f(0.85f, 0.85f, 0.85f);
    Rect(-0.70342f, -0.8188f, -0.65564f, -0.772f);
    glColor3f(0.05f, 0.05f, 0.08f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.70342f, -0.8188f); glVertex2f(-0.65564f, -0.8188f);
    glVertex2f(-0.65564f, -0.772f);  glVertex2f(-0.70342f, -0.772f);
    glEnd();

    glColor3f(0.3f, 0.6f, 0.9f);
    Rect(-0.69952f, -0.79308f, -0.65954f, -0.78372f);

    glColor3f(0.75f, 0.3f, 0.15f);
    Rect(-0.6908f, -0.772f, -0.6778f, -0.7148f);
    glColor3f(0.1f, 0.1f, 0.1f);
    Rect(-0.6908f, -0.7278f, -0.6778f, -0.7148f);

    glColor3f(0.1f, 0.1f, 0.1f);
    glLineWidth(1.5f);
    Line(-0.75266f, -0.8188f, -0.75266f, -0.7668f);
    glLineWidth(1.0f);
}

//ship
void T_ship3() {
    glColor3f(0.5f, 0.15f, 0.2f);
    glBegin(GL_POLYGON);
    glVertex2f(0.555f, -0.852f); glVertex2f(0.765f, -0.852f);
    glVertex2f(0.744f, -0.912f); glVertex2f(0.576f, -0.912f);
    glEnd();
    glColor3f(0.05f, 0.05f, 0.08f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.555f, -0.852f); glVertex2f(0.765f, -0.852f);
    glVertex2f(0.744f, -0.912f); glVertex2f(0.576f, -0.912f);
    glEnd();

    glColor3f(0.75f, 0.1f, 0.1f);
    Rect(0.576f, -0.912f, 0.744f, -0.897f);

    glColor3f(0.95f, 0.95f, 0.95f);
    Rect(0.58124f, -0.852f, 0.73876f, -0.786f);
    glColor3f(0.05f, 0.05f, 0.08f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.58124f, -0.852f); glVertex2f(0.73876f, -0.852f);
    glVertex2f(0.73876f, -0.786f); glVertex2f(0.58124f, -0.786f);
    glEnd();

    glColor3f(0.3f, 0.6f, 0.9f);
    for (int i = 0; i < 8; i++) {
        float wx = 0.59324f + i * 0.018f;
        Rect(wx, -0.8256f, wx + 0.0075f, -0.8106f);
    }

    glColor3f(0.85f, 0.85f, 0.85f);
    Rect(0.65606f, -0.786f, 0.71118f, -0.732f);
    glColor3f(0.05f, 0.05f, 0.08f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.65606f, -0.786f); glVertex2f(0.71118f, -0.786f);
    glVertex2f(0.71118f, -0.732f); glVertex2f(0.65606f, -0.732f);
    glEnd();

    glColor3f(0.3f, 0.6f, 0.9f);
    Rect(0.66056f, -0.75632f, 0.70668f, -0.74552f);

    glColor3f(0.15f, 0.15f, 0.15f);
    Rect(0.67062f, -0.732f, 0.68562f, -0.666f);
    glColor3f(0.1f, 0.1f, 0.1f);
    Rect(0.67062f, -0.681f, 0.68562f, -0.666f);

    glColor3f(0.1f, 0.1f, 0.1f);
    glLineWidth(1.5f);
    Line(0.59924f, -0.786f, 0.59924f, -0.726f);
    glLineWidth(1.0f);
}

//sky
void T_sky2() {
    glColor3f(Lerp(0.55f, 0.24f, weatherLevel), Lerp(0.78f, 0.26f, weatherLevel), Lerp(0.94f, 0.32f, weatherLevel));
    Rect(-1.0f, S2_SKY_BOTTOM, 1.0f, 1.0f);
}

//sun
void T_sun2() {
    if (weatherLevel > 0.65f) return;
    float alpha = 1.0f - weatherLevel / 0.65f;
    glColor3f(1.0f, Lerp(0.95f, 0.87f, 1.0f - alpha), 0.2f);
    Circle(0.08f * alpha + 0.0001f, -0.75f, 0.72f);
}

//cloud
void T_cloudCluster(float cx, float cy, float s, float r, float g, float b) {
    glColor3f(r, g, b);
    Circle(0.035f * s, cx - 0.045f * s, cy);
    Circle(0.05f * s, cx, cy + 0.012f * s);
    Circle(0.035f * s, cx + 0.05f * s, cy);
    Circle(0.03f * s, cx + 0.01f * s, cy - 0.02f * s);
}

//clouds
void T_clouds2() {
    float r = Lerp(1.0f, 0.28f, weatherLevel), g = Lerp(1.0f, 0.30f, weatherLevel), b = Lerp(1.0f, 0.34f, weatherLevel);
    T_cloudCluster(-0.55f, 0.85f, 1.2f, r, g, b);
    T_cloudCluster(-0.15f, 0.90f, 0.9f, r, g, b);
    T_cloudCluster(0.30f, 0.80f, 1.4f, r, g, b);
    T_cloudCluster(0.65f, 0.88f, 1.1f, r, g, b);
    T_cloudCluster(0.90f, 0.78f, 1.0f, r, g, b);
}

//sky
void T_sky3() {
    glColor3f(Lerp(0.24f, 0.55f, 1.0f - weatherLevel), Lerp(0.26f, 0.78f, 1.0f - weatherLevel), Lerp(0.32f, 0.94f, 1.0f - weatherLevel));
    Rect(-1.0f, S3_SKY_BOTTOM, 1.0f, 1.0f);
}

//clouds
void T_clouds3() {
    float r = Lerp(0.28f, 1.0f, 1.0f - weatherLevel), g = Lerp(0.30f, 1.0f, 1.0f - weatherLevel), b = Lerp(0.34f, 1.0f, 1.0f - weatherLevel);
    T_cloudCluster(-0.85f, 0.85f, 1.0f, r, g, b);
    T_cloudCluster(-0.55f, 0.90f, 1.3f, r, g, b);
    T_cloudCluster(-0.20f, 0.80f, 0.9f, r, g, b);
    T_cloudCluster(0.35f, 0.88f, 1.0f, r, g, b);
    T_cloudCluster(0.75f, 0.78f, 0.8f, r, g, b);
}

//sun
void T_sun3() {
    float alpha = 1.0f - weatherLevel;
    if (alpha < 0.05f) return;
    glColor3f(1.0f, 0.87f, 0.2f);
    Circle(0.08f * alpha, 0.70f, 0.72f);
    T_sunRays(0.70f, 0.72f, 0.08f * alpha);
}

//rainbow arc
void T_rainbowUnitArcs(float appearAlpha) {
    float colors[7][3] = {
        {0.85f, 0.20f, 0.25f}, {0.90f, 0.55f, 0.15f}, {0.95f, 0.85f, 0.15f},
        {0.30f, 0.70f, 0.30f}, {0.25f, 0.45f, 0.85f}, {0.30f, 0.20f, 0.55f}, {0.55f, 0.25f, 0.65f}
    };
    float bandW = 0.06f;
    for (int i = 0; i < 7; i++) {
        glColor4f(colors[i][0], colors[i][1], colors[i][2], appearAlpha * 0.85f);
        glBegin(GL_TRIANGLE_STRIP);
        int segs = 60;
        float rInner = 1.0f + bandW * i, rOuter = 1.0f + bandW * (i + 1);
        for (int s = 0; s <= segs; s++) {
            float a = PI * s / segs;
            glVertex2f(rInner * cos(a), rInner * sin(a));
            glVertex2f(rOuter * cos(a), rOuter * sin(a));
        }
        glEnd();
    }
}

//rainbow
void T_rainbow() {
    float t = 1.0f - weatherLevel;
    float appear = (t - 0.55f) / 0.45f;
    if (appear <= 0.0f) return;
    appear = Clamp01(appear);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix();
    glTranslatef(0.15f, -0.15f, 0.0f);
    glScalef(0.55f, 0.55f, 1.0f);
    T_rainbowUnitArcs(appear);
    glPopMatrix();
    glDisable(GL_BLEND);
}

//sky
void T_sky4() {
    float r, g, b;
    if (dayNightT < 0.5f) {
        float t = dayNightT / 0.5f;
        r = Lerp(0.45f, 0.90f, t); g = Lerp(0.68f, 0.45f, t); b = Lerp(0.90f, 0.35f, t);
    } else {
        float t = (dayNightT - 0.5f) / 0.5f;
        r = Lerp(0.90f, 0.05f, t); g = Lerp(0.45f, 0.06f, t); b = Lerp(0.35f, 0.16f, t);
    }
    glColor3f(r, g, b);
    Rect(-1.0f, CITY_GROUND_Y, 1.0f, 1.0f);
}

//sun
void T_sun4() {
    if (dayNightT > 0.55f) return;
    float alpha = 1.0f - dayNightT / 0.55f;
    float sunY = Lerp(0.75f, -0.10f, dayNightT / 0.55f);
    glColor3f(1.0f, Lerp(0.85f, 0.35f, 1.0f - alpha), 0.2f);
    Circle(0.07f * (0.5f + 0.5f * alpha), -0.6f, sunY);
}

//moon and stars
void T_moonAndStars() {
    if (dayNightT < 0.45f) return;
    float alpha = Clamp01((dayNightT - 0.45f) / 0.55f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, alpha * 0.9f);
    for (int i = 0; i < 50; i++) {
        float fx = -1.0f + fmod(i * 0.618f, 2.0f);
        float fy = 0.15f + fmod(i * 0.382f + 0.1f, 0.83f);
        Rect(fx, fy, fx + 0.004f, fy + 0.004f);
    }
    glColor4f(0.93f, 0.93f, 0.85f, alpha);
    Circle(0.075f, 0.65f, 0.72f);
    glColor4f(0.10f, 0.10f, 0.24f, alpha);
    Circle(0.060f, 0.68f, 0.735f);
    glDisable(GL_BLEND);
}

//tonmoy part
void Tonmoy_SkyAndBackground() {
    switch (scene) {
        case SCENE1:
            T_scene1Background();
            T_clouds1();
            T_ship1();
            T_ship3();
            break;
        case SCENE2:
            T_sky2();
            T_sun2();
            T_clouds2();
            break;
        case SCENE3:
            T_sky3();
            T_clouds3();
            T_sun3();
            T_rainbow();
            break;
        case SCENE4:
            T_sky4();
            T_sun4();
            T_moonAndStars();
            break;
    }
}


//part 2 - mridul

//water
void M_waterShading(float topY, float botY) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.0f, 0.0f, 0.05f, 0.18f);
    Rect(-1.1f, botY, 1.1f, botY + (topY - botY) * 0.35f);

    glColor4f(1.0f, 1.0f, 1.0f, 0.10f);
    Rect(-1.1f, topY - 0.035f, 1.1f, topY);

    for (int row = 0; row < 4; row++) {
        float rowY = topY - 0.06f - row * ((topY - botY - 0.10f) / 4.0f);
        glColor4f(1.0f, 1.0f, 1.0f, 0.15f - row * 0.025f);
        glLineWidth(1.3f);
        glBegin(GL_LINES);
        for (float x = -1.15f; x <= 1.15f; x += 0.22f) {
            float wob = sin(waterPhase * 1.4f + x * 4.0f + row * 1.7f) * 0.014f;
            glVertex2f(x, rowY + wob);
            glVertex2f(x + 0.11f, rowY + wob);
        }
        glEnd();
    }
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
}

//river bank
void M_riverBank(float topY, float botY) {
    float h = topY - botY;
    if (h <= 0.0f) return;

    glColor3f(0.24f, 0.5f, 0.22f);
    Rect(-1.1f, botY + h * 0.55f, 1.1f, topY);
    glColor3f(0.48f, 0.36f, 0.22f);
    Rect(-1.1f, botY + h * 0.20f, 1.1f, botY + h * 0.55f);
    glColor3f(0.30f, 0.24f, 0.16f);
    Rect(-1.1f, botY, 1.1f, botY + h * 0.20f);

    glColor3f(0.16f, 0.14f, 0.10f);
    glLineWidth(1.3f);
    Line(-1.1f, botY, 1.1f, botY);
    glLineWidth(1.0f);

    glColor3f(0.20f, 0.46f, 0.20f);
    for (float x = -1.05f; x <= 1.05f; x += 0.13f) {
        glBegin(GL_TRIANGLES);
        glVertex2f(x, topY);
        glVertex2f(x + 0.014f, topY + h * 0.30f);
        glVertex2f(x + 0.028f, topY);
        glEnd();
    }
}

//windmill
void M_windmill(float hubX, float hubY, float baseY, float phaseDeg) {
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(3.0f);
    Line(hubX, baseY, hubX, hubY);

    glPushMatrix();
    glTranslatef(hubX, hubY, 0.0f);
    glScalef(1.0f, WINDOW_ASPECT, 1.0f);
    glRotatef(bladeAngle + phaseDeg, 0.0f, 0.0f, 1.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    for (int k = 0; k < 3; k++) {
        float a = (90.0f + k * 120.0f) * PI / 180.0f;
        glVertex2f(0.0f, 0.0f);
        glVertex2f(0.075f * cos(a), 0.075f * sin(a));
    }
    glEnd();
    glPopMatrix();

    glColor3f(0.82f, 0.82f, 0.82f);
    Circle(0.007f, hubX, hubY);
}

//trees
void M_treesScene1() {
    glColor3f(0.40f, 0.26f, 0.13f);
    Rect(-0.934f, -0.3f, -0.926f, -0.236f);
    glColor3f(0.20f, 0.55f, 0.25f);
    Circle(0.024f, -0.93f, -0.204f);
    glColor3f(0.24f, 0.60f, 0.28f);
    Circle(0.016f, -0.944f, -0.22f);
    Circle(0.016f, -0.916f, -0.22f);

    glColor3f(0.40f, 0.26f, 0.13f);
    Rect(-0.8204f, -0.3f, -0.8116f, -0.2296f);
    glColor3f(0.20f, 0.55f, 0.25f);
    Circle(0.0264f, -0.816f, -0.1944f);
    glColor3f(0.24f, 0.60f, 0.28f);
    Circle(0.0176f, -0.8314f, -0.212f);
    Circle(0.0176f, -0.8006f, -0.212f);

    glColor3f(0.40f, 0.26f, 0.13f);
    Rect(-0.7096f, -0.3f, -0.7024f, -0.2424f);
    glColor3f(0.20f, 0.55f, 0.25f);
    Circle(0.0216f, -0.706f, -0.2136f);
    glColor3f(0.24f, 0.60f, 0.28f);
    Circle(0.0144f, -0.7186f, -0.228f);
    Circle(0.0144f, -0.6934f, -0.228f);

    glColor3f(0.40f, 0.26f, 0.13f);
    Rect(-0.59f, -0.3f, -0.582f, -0.236f);
    glColor3f(0.20f, 0.55f, 0.25f);
    Circle(0.024f, -0.586f, -0.204f);
    glColor3f(0.24f, 0.60f, 0.28f);
    Circle(0.016f, -0.6f, -0.22f);
    Circle(0.016f, -0.572f, -0.22f);
}

//windmill speed key
void M_cycleWindmillSpeed() {
    windState = (windState + 1) % 3;
    bladeSpeed = (windState == 0) ? 0.0f : ((windState == 1) ? 3.0f : 8.0f);
}

//windmills and trees
void M_windmillsAndTrees1() {
    M_windmill(-0.97f, -0.02f, -0.3f, 0.0f);
    M_windmill(-0.86f, 0.14f, -0.3f, 40.0f);
    M_windmill(-0.75f, 0.04f, -0.3f, 80.0f);
    M_windmill(-0.63f, 0.18f, -0.3f, 120.0f);
    M_treesScene1();
}

//hill
void M_hillShape(float x0, float x1, float baseY, float apexY, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_POLYGON);
    glVertex2f(x0, -1.0f);
    int N = 24;
    for (int i = 0; i <= N; i++) {
        float t = (float)i / N;
        float x = x0 + (x1 - x0) * t;
        float y = baseY + (apexY - baseY) * sin(t * PI);
        glVertex2f(x, y);
    }
    glVertex2f(x1, -1.0f);
    glEnd();
}

//hills
void M_hills2() {
    float t = weatherLevel;
    M_hillShape(-1.1f, 1.1f, S2_HILL_BACK_BASE, S2_HILL_BACK_APEX,
                Lerp(0.30f, 0.14f, t), Lerp(0.55f, 0.22f, t), Lerp(0.35f, 0.20f, t));
    M_hillShape(-1.1f, 1.1f, S2_HILL_FRONT_BASE, S2_HILL_FRONT_APEX,
                Lerp(0.20f, 0.09f, t), Lerp(0.45f, 0.16f, t), Lerp(0.25f, 0.14f, t));
}

//tree
void M_treeAt(float x, float baseY, float s) {
    glColor3f(0.36f, 0.22f, 0.12f);
    Rect(x - 0.006f * s, baseY, x + 0.006f * s, baseY + 0.06f * s);
    glColor3f(0.18f, 0.50f, 0.22f);
    Circle(0.03f * s, x, baseY + 0.085f * s);
    glColor3f(0.22f, 0.58f, 0.26f);
    Circle(0.02f * s, x - 0.016f * s, baseY + 0.07f * s);
    Circle(0.02f * s, x + 0.018f * s, baseY + 0.10f * s);
}

//trees
void M_trees2() {
    float xs[] = { -1.25f, -0.98f, -0.60f, -0.20f, 0.30f, 1.20f };
    for (int i = 0; i < 6; i++) M_treeAt(xs[i], S2_GRASS_TOP - 0.06f, 1.0f);
}

//grass
void M_grass2() {
    glColor3f(0.30f, 0.55f, 0.25f);
    Rect(-1.1f, S2_GRASS_BOTTOM, 1.1f, S2_GRASS_TOP);
}

//rail track
void M_railwayBed() {
    glColor3f(0.40f, 0.38f, 0.34f);
    Rect(-1.1f, S2_RAIL_BED_BOTTOM, 1.1f, S2_RAIL_BED_TOP);
    glColor3f(0.30f, 0.22f, 0.14f);
    glLineWidth(3.0f);
    for (float x = -1.1f; x <= 1.1f; x += 0.045f)
        Line(x, S2_RAIL_LOW_Y - 0.03f, x, S2_RAIL_TOP_Y + 0.015f);
    glColor3f(0.62f, 0.62f, 0.65f);
    glLineWidth(2.5f);
    Line(-1.1f, S2_RAIL_TOP_Y, 1.1f, S2_RAIL_TOP_Y);
    Line(-1.1f, S2_RAIL_LOW_Y, 1.1f, S2_RAIL_LOW_Y);
    glLineWidth(1.0f);
}

//electric poles
void M_electricPoles() {
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(2.5f);
    for (float x = -1.30f; x <= 1.30f; x += 0.45f) {
        Line(x, S2_RAIL_BED_BOTTOM, x, S2_POLE_TOP);
        Line(x - 0.06f, S2_POLE_TOP, x + 0.06f, S2_POLE_TOP);
        Line(x, S2_POLE_TOP, x, S2_MESSENGER_WIRE_Y);
        Line(x, S2_MESSENGER_WIRE_Y, x, S2_CONTACT_WIRE_Y);
    }
    glLineWidth(1.4f);
    Line(-1.35f, S2_MESSENGER_WIRE_Y, 1.35f, S2_MESSENGER_WIRE_Y);
    glLineWidth(1.0f);
    Line(-1.35f, S2_CONTACT_WIRE_Y, 1.35f, S2_CONTACT_WIRE_Y);
}

//water
void M_water2() {
    glColor3f(0.06f, 0.24f, 0.48f);
    Rect(-1.1f, S2_WATER_BOTTOM, 1.1f, S2_WATER_TOP);
    M_waterShading(S2_WATER_TOP, S2_WATER_BOTTOM);
}

//hills
void M_hills3Back()  { M_hillShape(-1.1f, 1.1f, S3_HILL_BACK_BASE,  S3_HILL_BACK_APEX,  0.30f, 0.55f, 0.35f); }

//hills
void M_hills3Front() { M_hillShape(-1.1f, 1.1f, S3_HILL_FRONT_BASE, S3_HILL_FRONT_APEX, 0.20f, 0.45f, 0.25f); }

//trees
void M_trees3() {
    float xs[] = { -1.20f, -0.85f, -0.10f, 0.55f, 1.10f };
    for (int i = 0; i < 5; i++) M_treeAt(xs[i], S3_GRASS_TOP - 0.06f, 1.0f);
    glColor3f(0.30f, 0.55f, 0.25f);
    Rect(-1.1f, S3_GRASS_BOTTOM, 1.1f, S3_GRASS_TOP);
}

//water
void M_water3() {
    glColor3f(0.06f, 0.24f, 0.48f);
    Rect(-1.1f, S3_WATER_BOTTOM, 1.1f, S3_WATER_TOP);
    M_waterShading(S3_WATER_TOP, S3_WATER_BOTTOM);
}

//road
void M_cityRoad() {
    glColor3f(0.55f, 0.55f, 0.55f);
    Rect(-1.1f, CITY_ROAD_TOP - 0.04f, 1.1f, CITY_GROUND_Y);
    glColor3f(Lerp(0.24f, 0.08f, dayNightT), Lerp(0.24f, 0.08f, dayNightT), Lerp(0.26f, 0.10f, dayNightT));
    Rect(-1.1f, CITY_ROAD_BOTTOM, 1.1f, CITY_ROAD_TOP);

    float midY = (CITY_ROAD_TOP + CITY_ROAD_BOTTOM) * 0.5f;
    glColor3f(0.90f, 0.85f, 0.30f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (float x = -1.2f; x <= 1.2f; x += 0.10f) {
        glVertex2f(x, midY);
        glVertex2f(x + 0.05f, midY);
    }
    glEnd();
    glLineWidth(1.0f);
}

//water
void M_water4() {
    glColor3f(Lerp(0.07f, 0.03f, dayNightT), Lerp(0.20f, 0.06f, dayNightT), Lerp(0.38f, 0.14f, dayNightT));
    Rect(-1.1f, CITY_WATER_BOTTOM, 1.1f, CITY_WATER_TOP);
    M_waterShading(CITY_WATER_TOP, CITY_WATER_BOTTOM);
}

//mridul part
void Mridul_LandWaterAndTrack() {
    switch (scene) {
        case SCENE1:
            M_windmillsAndTrees1();
            break;
        case SCENE2:
            M_hills2();
            M_trees2();
            M_grass2();
            M_railwayBed();
            M_electricPoles();
            M_water2();
            M_riverBank(S2_RAIL_BED_BOTTOM + 0.02f, S2_WATER_TOP - 0.045f);
            break;
        case SCENE3:
            M_hills3Back();
            M_hills3Front();
            M_trees3();
            M_riverBank(S3_GRASS_BOTTOM, S3_WATER_TOP);
            M_water3();
            break;
        case SCENE4:
            M_cityRoad();
            M_water4();
            M_riverBank(CITY_ROAD_BOTTOM + 0.025f, CITY_WATER_TOP - 0.025f);
            break;
    }
}


//part 3 - faysal

//city lights key
void F_toggleLights() {
    lightsOn = !lightsOn;
}

//buildings
void F_buildingsShopsTea() {
    float wr = lightsOn ? 1.0f : 0.35f;
    float wg = lightsOn ? 0.85f : 0.38f;
    float wb = lightsOn ? 0.30f : 0.45f;
    glColor3f(0.55f, 0.35f, 0.65f);
    Rect(-0.56f, -0.3f, -0.47f, 0.26f);
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.56f, -0.3f); glVertex2f(-0.47f, -0.3f);
    glVertex2f(-0.47f, 0.26f); glVertex2f(-0.56f, 0.26f);
    glEnd();
    glColor3f(wr, wg, wb);
    for (int r = 0; r < 5; r++) {
        float wy = -0.26f + r * 0.096f;
        Rect(-0.544f, wy, -0.53f, wy + 0.032f);
        Rect(-0.5f, wy, -0.486f, wy + 0.032f);
    }

    glColor3f(0.85f, 0.55f, 0.35f);
    Rect(-0.47f, -0.3f, -0.4f, -0.08f);
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.47f, -0.3f); glVertex2f(-0.4f, -0.3f);
    glVertex2f(-0.4f, -0.08f); glVertex2f(-0.47f, -0.08f);
    glEnd();
    glColor3f(0.4f, 0.15f, 0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.476f, -0.08f); glVertex2f(-0.394f, -0.08f); glVertex2f(-0.435f, -0.031f);
    glEnd();
    glColor3f(0.35f, 0.2f, 0.1f);
    Rect(-0.4455f, -0.3f, -0.4245f, -0.201f);
    glColor3f(0.85f, 0.9f, 1.0f);
    Circle(0.0098f, -0.435f, -0.1405f);

    glColor3f(0.75f, 0.35f, 0.40f);
    Rect(-0.4f, -0.3f, -0.29f, -0.06f);
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.4f, -0.3f); glVertex2f(-0.29f, -0.3f);
    glVertex2f(-0.29f, -0.06f); glVertex2f(-0.4f, -0.06f);
    glEnd();
    glColor3f(0.4f, 0.15f, 0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.296f, -0.06f); glVertex2f(-0.204f, -0.06f); glVertex2f(-0.25f, -0.004f);
    glEnd();
    glColor3f(0.35f, 0.2f, 0.1f);
    Rect(-0.262f, -0.3f, -0.238f, -0.192f);
    glColor3f(0.85f, 0.9f, 1.0f);
    Circle(0.0112f, -0.25f, -0.126f);

    glColor3f(0.35f, 0.55f, 0.55f);
    Rect(-0.4f, -0.3f, -0.29f, 0.42f);
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.4f, -0.3f); glVertex2f(-0.29f, -0.3f);
    glVertex2f(-0.29f, 0.42f); glVertex2f(-0.4f, 0.42f);
    glEnd();
    glColor3f(wr, wg, wb);
    for (int r = 0; r < 7; r++) {
        float wy = -0.26f + r * 0.096f;
        Rect(-0.384f, wy, -0.37f, wy + 0.032f);
        Rect(-0.34f, wy, -0.326f, wy + 0.032f);
    }

    glColor3f(0.55f, 0.50f, 0.30f);
    Rect(-0.29f, -0.3f, -0.21f, 0.14f);
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.29f, -0.3f); glVertex2f(-0.21f, -0.3f);
    glVertex2f(-0.21f, 0.14f); glVertex2f(-0.29f, 0.14f);
    glEnd();
    glColor3f(wr, wg, wb);
    for (int r = 0; r < 4; r++) {
        float wy = -0.26f + r * 0.096f;
        Rect(-0.194f, wy, -0.18f, wy + 0.032f);
        Rect(-0.15f, wy, -0.136f, wy + 0.032f);
    }

    glColor3f(0.35f, 0.40f, 0.65f);
    Rect(-0.21f, -0.3f, -0.02f, 0.5f);
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.21f, -0.3f); glVertex2f(-0.02f, -0.3f);
    glVertex2f(-0.02f, 0.5f);  glVertex2f(-0.21f, 0.5f);
    glEnd();
    glColor3f(wr, wg, wb);
    for (int r = 0; r < 8; r++) {
        float wy = -0.26f + r * 0.096f;
        Rect(-0.094f, wy, -0.08f, wy + 0.032f);
        Rect(-0.05f, wy, -0.036f, wy + 0.032f);
    }

    glColor3f(0.65f, 0.50f, 0.40f);
    Rect(-0.02f, -0.3f, 0.05f, -0.1f);
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.02f, -0.3f); glVertex2f(0.05f, -0.3f);
    glVertex2f(0.05f, -0.1f);  glVertex2f(-0.02f, -0.1f);
    glEnd();
    glColor3f(0.4f, 0.15f, 0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.026f, -0.1f); glVertex2f(0.056f, -0.1f); glVertex2f(0.015f, -0.051f);
    glEnd();
    glColor3f(0.35f, 0.2f, 0.1f);
    Rect(0.0045f, -0.3f, 0.0255f, -0.21f);
    glColor3f(0.85f, 0.9f, 1.0f);
    Circle(0.0098f, 0.015f, -0.155f);

    glColor3f(0.40f, 0.60f, 0.45f);
    Rect(0.05f, -0.3f, 0.17f, 0.34f);
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.05f, -0.3f); glVertex2f(0.17f, -0.3f);
    glVertex2f(0.17f, 0.34f); glVertex2f(0.05f, 0.34f);
    glEnd();
    glColor3f(wr, wg, wb);
    for (int r = 0; r < 6; r++) {
        float wy = -0.26f + r * 0.096f;
        Rect(0.066f, wy, 0.08f, wy + 0.032f);
        Rect(0.11f, wy, 0.124f, wy + 0.032f);
    }

    glColor3f(0.55f, 0.35f, 0.65f);
    Rect(0.17f, -0.3f, 0.25f, -0.04f);
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.17f, -0.3f); glVertex2f(0.25f, -0.3f);
    glVertex2f(0.25f, -0.04f); glVertex2f(0.17f, -0.04f);
    glEnd();
    glColor3f(0.4f, 0.15f, 0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(0.164f, -0.04f); glVertex2f(0.256f, -0.04f); glVertex2f(0.21f, 0.016f);
    glEnd();
    glColor3f(0.35f, 0.2f, 0.1f);
    Rect(0.198f, -0.3f, 0.222f, -0.183f);
    glColor3f(0.85f, 0.9f, 1.0f);
    Circle(0.0112f, 0.21f, -0.1115f);

    glColor3f(0.85f, 0.55f, 0.35f);
    Rect(0.25f, -0.3f, 0.36f, 0.22f);
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.25f, -0.3f); glVertex2f(0.36f, -0.3f);
    glVertex2f(0.36f, 0.22f); glVertex2f(0.25f, 0.22f);
    glEnd();
    glColor3f(wr, wg, wb);
    for (int r = 0; r < 5; r++) {
        float wy = -0.26f + r * 0.096f;
        Rect(0.266f, wy, 0.28f, wy + 0.032f);
        Rect(0.31f, wy, 0.324f, wy + 0.032f);
    }

    glColor3f(0.35f, 0.55f, 0.55f);
    Rect(0.36f, -0.3f, 0.45f, 0.46f);
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.36f, -0.3f); glVertex2f(0.45f, -0.3f);
    glVertex2f(0.45f, 0.46f); glVertex2f(0.36f, 0.46f);
    glEnd();
    glColor3f(wr, wg, wb);
    for (int r = 0; r < 7; r++) {
        float wy = -0.26f + r * 0.096f;
        Rect(0.376f, wy, 0.39f, wy + 0.032f);
        Rect(0.42f, wy, 0.434f, wy + 0.032f);
    }

    glColor3f(0.75f, 0.35f, 0.40f);
    Rect(0.45f, -0.3f, 0.52f, -0.08f);
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.45f, -0.3f); glVertex2f(0.52f, -0.3f);
    glVertex2f(0.52f, -0.08f); glVertex2f(0.45f, -0.08f);
    glEnd();
    glColor3f(0.4f, 0.15f, 0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(0.444f, -0.08f); glVertex2f(0.526f, -0.08f); glVertex2f(0.485f, -0.031f);
    glEnd();
    glColor3f(0.35f, 0.2f, 0.1f);
    Rect(0.4745f, -0.3f, 0.4955f, -0.201f);
    glColor3f(0.85f, 0.9f, 1.0f);
    Circle(0.0098f, 0.485f, -0.1405f);

    glColor3f(0.55f, 0.50f, 0.30f);
    Rect(0.52f, -0.3f, 0.62f, 0.3f);
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.52f, -0.3f); glVertex2f(0.62f, -0.3f);
    glVertex2f(0.62f, 0.3f);  glVertex2f(0.52f, 0.3f);
    glEnd();
    glColor3f(wr, wg, wb);
    for (int r = 0; r < 6; r++) {
        float wy = -0.26f + r * 0.096f;
        Rect(0.536f, wy, 0.55f, wy + 0.032f);
        Rect(0.58f, wy, 0.594f, wy + 0.032f);
    }

    glColor3f(0.35f, 0.40f, 0.65f);
    Rect(0.62f, -0.3f, 0.71f, -0.06f);
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.62f, -0.3f); glVertex2f(0.71f, -0.3f);
    glVertex2f(0.71f, -0.06f); glVertex2f(0.62f, -0.06f);
    glEnd();
    glColor3f(0.4f, 0.15f, 0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(0.614f, -0.06f); glVertex2f(0.716f, -0.06f); glVertex2f(0.665f, 0.003f);
    glEnd();
    glColor3f(0.35f, 0.2f, 0.1f);
    Rect(0.6515f, -0.3f, 0.6785f, -0.192f);
    glColor3f(0.85f, 0.9f, 1.0f);
    Circle(0.0126f, 0.665f, -0.126f);

    glColor3f(0.65f, 0.50f, 0.40f);
    Rect(0.71f, -0.3f, 0.83f, 0.38f);
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.71f, -0.3f); glVertex2f(0.83f, -0.3f);
    glVertex2f(0.83f, 0.38f); glVertex2f(0.71f, 0.38f);
    glEnd();
    glColor3f(wr, wg, wb);
    for (int r = 0; r < 6; r++) {
        float wy = -0.26f + r * 0.096f;
        Rect(0.726f, wy, 0.74f, wy + 0.032f);
        Rect(0.77f, wy, 0.784f, wy + 0.032f);
    }

    glColor3f(0.40f, 0.60f, 0.45f);
    Rect(0.83f, -0.3f, 0.91f, 0.18f);
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.83f, -0.3f); glVertex2f(0.91f, -0.3f);
    glVertex2f(0.91f, 0.18f); glVertex2f(0.83f, 0.18f);
    glEnd();
    glColor3f(wr, wg, wb);
    for (int r = 0; r < 4; r++) {
        float wy = -0.26f + r * 0.096f;
        Rect(0.846f, wy, 0.86f, wy + 0.032f);
    }

    glColor3f(0.85f, 0.55f, 0.35f);
    Rect(0.91f, -0.3f, 0.98f, -0.08f);
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.91f, -0.3f); glVertex2f(0.98f, -0.3f);
    glVertex2f(0.98f, -0.08f); glVertex2f(0.91f, -0.08f);
    glEnd();
    glColor3f(0.4f, 0.15f, 0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(0.904f, -0.08f); glVertex2f(0.986f, -0.08f); glVertex2f(0.945f, -0.031f);
    glEnd();
    glColor3f(0.35f, 0.2f, 0.1f);
    Rect(0.9345f, -0.3f, 0.9555f, -0.201f);
    glColor3f(0.85f, 0.9f, 1.0f);
    Circle(0.0098f, 0.945f, -0.1405f);
}

//station
void F_station() {
    glColor3f(0.55f, 0.50f, 0.42f);
    Rect(S2_STATION_X0, S2_RAIL_BED_TOP, S2_STATION_X1, S2_GRASS_TOP);

    glColor3f(0.65f, 0.42f, 0.30f);
    Rect(S2_STATION_X0 + 0.05f, S2_GRASS_TOP, S2_STATION_X0 + 0.32f, 0.10f);
    glColor3f(0.1f, 0.1f, 0.1f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(S2_STATION_X0 + 0.05f, S2_GRASS_TOP);
    glVertex2f(S2_STATION_X0 + 0.32f, S2_GRASS_TOP);
    glVertex2f(S2_STATION_X0 + 0.32f, 0.10f);
    glVertex2f(S2_STATION_X0 + 0.05f, 0.10f);
    glEnd();
    glLineWidth(1.0f);

    glColor3f(0.55f, 0.78f, 0.88f);
    Rect(S2_STATION_X0 + 0.09f, -0.10f, S2_STATION_X0 + 0.15f, 0.0f);
    Rect(S2_STATION_X0 + 0.21f, -0.10f, S2_STATION_X0 + 0.27f, 0.0f);
    glColor3f(0.35f, 0.22f, 0.12f);
    Rect(S2_STATION_X0 + 0.16f, S2_GRASS_TOP, S2_STATION_X0 + 0.20f, -0.02f);

    glColor3f(1.0f, 1.0f, 1.0f);
    Rect(S2_STATION_X0 + 0.09f, 0.06f, S2_STATION_X0 + 0.28f, 0.095f);
    glColor3f(0.1f, 0.1f, 0.15f);
    glLineWidth(1.2f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(S2_STATION_X0 + 0.09f, 0.06f);
    glVertex2f(S2_STATION_X0 + 0.28f, 0.06f);
    glVertex2f(S2_STATION_X0 + 0.28f, 0.095f);
    glVertex2f(S2_STATION_X0 + 0.09f, 0.095f);
    glEnd();
    glLineWidth(1.0f);
    glColor3f(0.1f, 0.1f, 0.15f);
    DrawText(S2_STATION_X0 + 0.10f, 0.070f, "CITY STATION");

    glColor3f(0.10f, 0.40f, 0.32f);
    Rect(S2_STATION_X0 - 0.08f, 0.03f, S2_STATION_X1 - 0.05f, 0.10f);
    glColor3f(0.08f, 0.30f, 0.24f);
    Rect(S2_STATION_X0 - 0.08f, 0.10f, S2_STATION_X1 - 0.05f, 0.13f);

    glColor3f(0.30f, 0.30f, 0.30f);
    glLineWidth(3.0f);
    Line(S2_STATION_X0 - 0.02f, S2_RAIL_BED_TOP, S2_STATION_X0 - 0.02f, 0.03f);
    Line(S2_STATION_X1 - 0.10f, S2_RAIL_BED_TOP, S2_STATION_X1 - 0.10f, 0.03f);
    glLineWidth(1.0f);

    glColor3f(0.75f, 0.75f, 0.75f);
    Rect(S2_STATION_X0 - 0.05f, S2_RAIL_BED_TOP, S2_STATION_X1, S2_RAIL_BED_TOP + 0.03f);
}

//building
void F_building(float x, float w, float h, float r, float g, float b, int seed, bool blinker) {
    glColor3f(r * sceneIntensity, g * sceneIntensity, b * sceneIntensity);
    Rect(x, CITY_GROUND_Y, x + w, CITY_GROUND_Y + h);
    glColor3f(0.05f, 0.05f, 0.07f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, CITY_GROUND_Y); glVertex2f(x + w, CITY_GROUND_Y);
    glVertex2f(x + w, CITY_GROUND_Y + h); glVertex2f(x, CITY_GROUND_Y + h);
    glEnd();

    int cols = (int)(w / 0.028f); if (cols < 2) cols = 2;
    int rows = (int)(h / 0.05f);  if (rows < 2) rows = 2;
    float cellW = w / cols, cellH = h / rows;
    bool lit = lightsOn && dayNightT > 0.5f;
    for (int ry = 0; ry < rows; ry++) {
        for (int cx = 0; cx < cols; cx++) {
            bool on = ((ry * 7 + cx * 3 + seed) % 5) != 0;
            bool isBlinkCell = blinker && ry == rows / 2 && cx == cols / 2;
            if (isBlinkCell) on = windowBlinkOn;
            if (lit && on) glColor3f(1.0f, 0.85f, 0.35f);
            else if (lit)  glColor3f(0.08f, 0.08f, 0.12f);
            else           glColor3f(0.55f, 0.65f, 0.72f);
            float wx = x + cx * cellW + cellW * 0.22f;
            float wy = CITY_GROUND_Y + ry * cellH + cellH * 0.22f;
            Rect(wx, wy, wx + cellW * 0.56f, wy + cellH * 0.56f);
        }
    }
}

//skyline
void F_skyline() {
    float bw[9] = { 0.13f, 0.10f, 0.15f, 0.09f, 0.14f, 0.11f, 0.16f, 0.10f, 0.12f };
    float bh[9] = { 0.55f, 0.75f, 0.45f, 0.85f, 0.60f, 0.95f, 0.50f, 0.72f, 0.58f };
    float bc[9][3] = {
        {0.30f,0.32f,0.42f}, {0.35f,0.28f,0.30f}, {0.28f,0.34f,0.38f},
        {0.32f,0.30f,0.45f}, {0.36f,0.30f,0.32f}, {0.26f,0.30f,0.40f},
        {0.34f,0.34f,0.30f}, {0.30f,0.36f,0.40f}, {0.32f,0.28f,0.36f}
    };
    float x = -1.15f;
    for (int i = 0; i < 9; i++) {
        bool blinker = (i == 3 || i == 6);
        F_building(x, bw[i], bh[i], bc[i][0], bc[i][1], bc[i][2], i * 11 + 3, blinker);
        x += bw[i] + 0.015f;
    }
}

//street light
void F_streetlight(float x) {
    float base = CITY_ROAD_TOP - 0.04f;
    float top  = 0.10f;
    const float ARM = 0.045f, ANG = 40.0f;

    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(2.5f);
    Line(x, base, x, top);

    glPushMatrix();
    glTranslatef(x, top, 0.0f);
    glScalef(1.0f, WINDOW_ASPECT, 1.0f);
    glRotatef(ANG, 0.0f, 0.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(ARM, 0.0f);
    glEnd();
    glPopMatrix();
    glLineWidth(1.0f);

    float lx = x + ARM * cos(ANG * PI / 180.0f);
    float ly = top + ARM * WINDOW_ASPECT * sin(ANG * PI / 180.0f);

    if (dayNightT > 0.5f) {
        float alpha = (dayNightT - 0.5f) / 0.5f;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1.0f, 0.9f, 0.5f, 0.35f * alpha * (lightsOn ? 1.0f : 0.0f));
        Circle(0.045f, lx, ly);
        glColor4f(1.0f, 0.95f, 0.6f, 0.9f * alpha * (lightsOn ? 1.0f : 0.0f));
        Circle(0.018f, lx, ly);
        glDisable(GL_BLEND);
        if (!lightsOn) {
            glColor3f(0.35f, 0.35f, 0.33f);
            Circle(0.014f, lx, ly);
        }
    } else {
        glColor3f(0.75f, 0.75f, 0.7f);
        Circle(0.014f, lx, ly);
    }
}

//street lights
void F_streetlights() {
    for (float x = -1.0f; x <= 1.0f; x += 0.35f) F_streetlight(x);
}

//faysal part
void Faysal_BuildingsAndStructures() {
    switch (scene) {
        case SCENE1:
            F_buildingsShopsTea();
            break;
        case SCENE2:
            F_station();
            break;
        case SCENE3:
            break;
        case SCENE4:
            F_skyline();
            F_streetlights();
            break;
    }
}


//part 4 - rumi

//train wheel
void R_wheelPair(float cx, float y, float bogieHalfW) {
    glColor3f(0.22f, 0.22f, 0.24f);
    Rect(cx - bogieHalfW, y - 0.006f, cx + bogieHalfW, y + 0.012f);
    glColor3f(0.08f, 0.08f, 0.08f);
    Circle(0.032f, cx, y);
    glColor3f(0.60f, 0.60f, 0.62f);
    Circle(0.010f, cx, y);
    glPushMatrix();
    glTranslatef(cx, y, 0.0f);
    glRotatef(wheelAngle, 0.0f, 0.0f, 1.0f);
    glColor3f(0.3f, 0.3f, 0.3f);
    glLineWidth(1.5f);
    Line(-0.028f, 0.0f, 0.028f, 0.0f);
    Line(0.0f, -0.028f, 0.0f, 0.028f);
    glLineWidth(1.0f);
    glPopMatrix();
}

//coach
void R_coachLivery(float ox, float bodyRight, float bottom, float top) {
    float h = top - bottom;
    float bodyW = bodyRight - ox;

    glColor3f(0.75f, 0.14f, 0.13f);
    Rect(ox, bottom, bodyRight, top);

    glColor3f(0.95f, 0.95f, 0.95f);
    Rect(ox, bottom + h * 0.14f, bodyRight, bottom + h * 0.205f);
    glColor3f(0.03f, 0.42f, 0.20f);
    Rect(ox, bottom + h * 0.24f, bodyRight, bottom + h * 0.78f);
    glColor3f(0.95f, 0.95f, 0.95f);
    Rect(ox, bottom + h * 0.783f, bodyRight, bottom + h * 0.843f);

    float winStart = ox + bodyW * 0.22f, winEnd = ox + bodyW * 0.78f;
    float winTotal = winEnd - winStart, winGap = winTotal * 0.06f;
    float winW = (winTotal - 4.0f * winGap) / 5.0f;
    for (int i = 0; i < 5; i++) {
        float wx = winStart + i * (winW + winGap);
        glColor3f(0.08f, 0.10f, 0.14f);
        Rect(wx, bottom + h * 0.30f, wx + winW, bottom + h * 0.63f);
        glColor3f(0.85f, 0.87f, 0.90f);
        Rect(wx, bottom + h * 0.585f, wx + winW, bottom + h * 0.63f);
    }

    glColor3f(0.10f, 0.10f, 0.11f);
    Rect(ox + bodyW * 0.05f, bottom + h * 0.13f, ox + bodyW * 0.15f, bottom + h * 0.78f);
    Rect(ox + bodyW * 0.85f, bottom + h * 0.13f, ox + bodyW * 0.95f, bottom + h * 0.78f);

    glColor3f(0.08f, 0.08f, 0.10f);
    glLineWidth(1.2f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(ox, bottom); glVertex2f(bodyRight, bottom);
    glVertex2f(bodyRight, top); glVertex2f(ox, top);
    glEnd();
    glLineWidth(1.0f);
}

//engine
void R_engine(float ox, float w, float bottom, float top) {
    float h = top - bottom;
    float TIPX = ox + w;
    float stepX = ox + w * 0.918f;
    float stepTop = bottom + h * 0.375f;
    float noseH = stepTop - bottom;
    float noseW = TIPX - stepX;

    glColor3f(0.72f, 0.09f, 0.09f);
    glBegin(GL_POLYGON);
    glVertex2f(ox, bottom);
    glVertex2f(TIPX, bottom);
    glVertex2f(TIPX, stepTop);
    glVertex2f(stepX, stepTop);
    glVertex2f(stepX, top);
    glVertex2f(ox, top);
    glEnd();

    glColor3f(0.88f, 0.80f, 0.58f);
    Rect(ox, bottom + h * 0.30f, stepX, bottom + h * 0.52f);
    Rect(stepX, bottom + noseH * 0.30f, TIPX, bottom + noseH * 0.52f);

    glColor3f(0.10f, 0.30f, 0.58f);
    Rect(ox, bottom + h * 0.52f, stepX, top);
    Rect(stepX, bottom + noseH * 0.52f, TIPX, stepTop);

    glColor3f(0.10f, 0.10f, 0.12f);
    Rect(ox, bottom, TIPX, bottom + h * 0.08f);

    glColor3f(0.08f, 0.09f, 0.11f);
    Rect(stepX + noseW * 0.18f, bottom + noseH * 0.58f, TIPX - noseW * 0.18f, stepTop - noseH * 0.06f);

    glColor3f(0.85f, 0.15f, 0.15f);
    Circle(0.014f, ox + (stepX - ox) * 0.14f, bottom + h * 0.41f);
    glColor3f(0.95f, 0.95f, 0.95f);
    Circle(0.007f, ox + (stepX - ox) * 0.14f, bottom + h * 0.41f);

    glColor3f(1.0f, 0.95f, 0.6f);
    Circle(0.013f, TIPX - noseW * 0.5f, bottom + noseH * 0.22f);

    glColor3f(0.05f, 0.05f, 0.07f);
    glLineWidth(1.3f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(ox, bottom);
    glVertex2f(TIPX, bottom);
    glVertex2f(TIPX, stepTop);
    glVertex2f(stepX, stepTop);
    glVertex2f(stepX, top);
    glVertex2f(ox, top);
    glEnd();
    glLineWidth(1.0f);
}

//coach
void R_coach(float ox, float w, float bottom, float top, bool isFront, bool hasPantograph) {
    float h = top - bottom;

    if (isFront) {
        R_engine(ox, w, bottom, top);
    } else {
        R_coachLivery(ox, ox + w, bottom, top);
    }

    float bogieHalfW = w * 0.05f;
    R_wheelPair(ox + w * 0.22f, bottom - 0.015f, bogieHalfW);
    R_wheelPair(ox + w * 0.78f, bottom - 0.015f, bogieHalfW);

    if (hasPantograph) {
        float px = ox + w * 0.5f;
        glColor3f(0.15f, 0.15f, 0.15f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_STRIP);
        glVertex2f(px, top + h * 0.10f);
        glVertex2f(px - 0.03f, top + (S2_CONTACT_WIRE_Y - top) * 0.5f);
        glVertex2f(px, S2_CONTACT_WIRE_Y);
        glVertex2f(px + 0.03f, top + (S2_CONTACT_WIRE_Y - top) * 0.5f);
        glVertex2f(px, top + h * 0.10f);
        glEnd();
        glLineWidth(1.0f);
    }
}

//train
void R_train(float noseX) {
    const int COACHES = 3;
    const float COACH_W = 0.46f, GAP = 0.025f;
    const float BOTTOM = S2_RAIL_TOP_Y + 0.02f, TOP = -0.24f;
    float trainLen = COACHES * COACH_W + (COACHES - 1) * GAP;
    float leftEdge = noseX - trainLen;

    drawShadow((leftEdge + noseX) * 0.5f, BOTTOM - 0.03f, trainLen * 0.52f, 0.02f);

    for (int i = 0; i < COACHES; i++) {
        float ox = leftEdge + i * (COACH_W + GAP);
        bool isFront = (i == COACHES - 1);
        bool hasPanto = (i == 1);
        R_coach(ox, COACH_W, BOTTOM, TOP, isFront, hasPanto);
        if (i > 0) {
            glColor3f(0.05f, 0.05f, 0.06f);
            Rect(ox - GAP, BOTTOM + (TOP - BOTTOM) * 0.3f, ox, BOTTOM + (TOP - BOTTOM) * 0.6f);
        }
    }
}

//wheel
void R_wheel4(float cx, float cy, float r) {
    glColor3f(0.05f, 0.05f, 0.05f);
    Circle(r, cx, cy);
    glColor3f(0.7f, 0.7f, 0.72f);
    Circle(r * 0.4f, cx, cy);
}

//headlight
void R_headlightGlow(float cx, float cy) {
    if (dayNightT < 0.5f) return;
    float alpha = (dayNightT - 0.5f) / 0.5f;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 0.95f, 0.6f, 0.8f * alpha);
    Circle(0.010f, cx, cy);
    glColor4f(1.0f, 0.95f, 0.6f, 0.25f * alpha);
    Circle(0.022f, cx, cy);
    glDisable(GL_BLEND);
}

//car
void R_car(const Vehicle &v, float r, float g, float b) {
    float x = v.x, y = v.y, w = v.w, h = v.h;
    drawShadow(x + w * 0.5f, y - 0.01f, w * 0.55f, 0.018f);
    glColor3f(r, g, b);
    glBegin(GL_POLYGON);
    glVertex2f(x, y);                    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h * 0.6f);     glVertex2f(x + w * 0.8f, y + h * 0.6f);
    glVertex2f(x + w * 0.62f, y + h);    glVertex2f(x + w * 0.30f, y + h);
    glVertex2f(x + w * 0.15f, y + h * 0.6f); glVertex2f(x, y + h * 0.6f);
    glEnd();
    glColor3f(Lerp(r,1.0f,0.5f), Lerp(g,1.0f,0.5f), Lerp(b,1.0f,0.5f));
    Rect(x + w * 0.05f, y + h * 0.02f, x + w * 0.95f, y + h * 0.10f);
    glColor3f(0.55f, 0.78f, 0.90f);
    Rect(x + w * 0.32f, y + h * 0.62f, x + w * 0.60f, y + h * 0.93f);
    glColor3f(0.08f, 0.08f, 0.08f);
    glLineWidth(1.2f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y); glVertex2f(x + w, y);
    glVertex2f(x + w, y + h * 0.6f); glVertex2f(x, y + h * 0.6f);
    glEnd();
    glLineWidth(1.0f);
    R_wheel4(x + w * 0.22f, y, h * 0.20f);
    R_wheel4(x + w * 0.78f, y, h * 0.20f);
    R_headlightGlow(x + w * 0.98f, y + h * 0.30f);
}

//brtc bus
void R_brtcBusUnit() {
    glColor3f(0.90f, 0.15f, 0.45f);
    Rect(0.0f, 0.0f, 1.0f, 1.0f);
    glColor3f(1.0f, 0.55f, 0.75f);
    Rect(0.0f, 0.88f, 1.0f, 1.0f);
    glColor3f(0.95f, 0.95f, 0.95f);
    Rect(0.0f, 0.48f, 1.0f, 0.55f);
    Rect(0.0f, 0.0f, 1.0f, 0.06f);

    glColor3f(0.55f, 0.78f, 0.90f);
    for (int i = 0; i < 5; i++) {
        float wx = 0.06f + i * 0.175f;
        Rect(wx, 0.10f, wx + 0.12f, 0.36f);
        Rect(wx, 0.60f, wx + 0.12f, 0.86f);
    }

    glColor3f(0.08f, 0.08f, 0.08f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.0f, 0.0f); glVertex2f(1.0f, 0.0f);
    glVertex2f(1.0f, 1.0f); glVertex2f(0.0f, 1.0f);
    glEnd();
    glLineWidth(1.0f);

    glColor3f(0.05f, 0.05f, 0.05f);
    Circle(0.11f, 0.16f, 0.0f);
    Circle(0.11f, 0.84f, 0.0f);
    glColor3f(0.7f, 0.7f, 0.72f);
    Circle(0.045f, 0.16f, 0.0f);
    Circle(0.045f, 0.84f, 0.0f);

    glColor3f(1.0f, 1.0f, 1.0f);
    DrawText(0.30f, 0.40f, "BRTC");
}

//brtc bus
void R_brtcBus(const Vehicle &v) {
    drawShadow(v.x + v.w * 0.5f, v.y - 0.02f, v.w * 0.55f, 0.02f);
    glPushMatrix();
    glTranslatef(v.x, v.y, 0.0f);
    glScalef(v.w, v.h, 1.0f);
    R_brtcBusUnit();
    glPopMatrix();
    R_headlightGlow(v.x + v.w * 0.99f, v.y + v.h * 0.20f);
}

//aiub bus
void R_aiubBusUnit() {
    glColor3f(0.96f, 0.97f, 0.98f);
    Rect(0.0f, 0.0f, 1.0f, 1.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    Rect(0.0f, 0.85f, 1.0f, 1.0f);
    glColor3f(0.12f, 0.30f, 0.62f);
    Rect(0.0f, 0.15f, 1.0f, 0.32f);

    glColor3f(0.55f, 0.78f, 0.90f);
    for (int i = 0; i < 4; i++) {
        float wx = 0.08f + i * 0.22f;
        Rect(wx, 0.45f, wx + 0.15f, 0.78f);
    }

    glColor3f(0.08f, 0.08f, 0.08f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.0f, 0.0f); glVertex2f(1.0f, 0.0f);
    glVertex2f(1.0f, 1.0f); glVertex2f(0.0f, 1.0f);
    glEnd();
    glLineWidth(1.0f);

    glColor3f(0.05f, 0.05f, 0.05f);
    Circle(0.15f, 0.22f, 0.0f);
    Circle(0.15f, 0.78f, 0.0f);
    glColor3f(0.7f, 0.7f, 0.72f);
    Circle(0.06f, 0.22f, 0.0f);
    Circle(0.06f, 0.78f, 0.0f);

    glColor3f(0.08f, 0.20f, 0.55f);
    DrawText(0.22f, 0.20f, "AIUB");
}

//aiub bus
void R_aiubBus(const Vehicle &v) {
    drawShadow(v.x + v.w * 0.5f, v.y - 0.02f, v.w * 0.55f, 0.02f);
    glPushMatrix();
    glTranslatef(v.x, v.y, 0.0f);
    glScalef(v.w, v.h, 1.0f);
    R_aiubBusUnit();
    glPopMatrix();
    R_headlightGlow(v.x + v.w * 0.99f, v.y + v.h * 0.25f);
}

//city bus
void R_cityBus(const Vehicle &v) {
    float x = v.x, y = v.y, w = v.w, h = v.h;
    drawShadow(x + w * 0.5f, y - 0.02f, w * 0.55f, 0.02f);
    glColor3f(0.15f, 0.55f, 0.35f);
    Rect(x, y, x + w, y + h);
    glColor3f(0.55f, 0.85f, 0.65f);
    Rect(x, y + h * 0.9f, x + w, y + h);
    glColor3f(0.90f, 0.90f, 0.90f);
    Rect(x, y + h * 0.06f, x + w, y + h * 0.14f);

    glColor3f(dayNightT > 0.5f ? 1.0f : 0.55f, dayNightT > 0.5f ? 0.92f : 0.78f, dayNightT > 0.5f ? 0.55f : 0.90f);
    for (int i = 0; i < 5; i++) {
        float wx = x + w * 0.05f + i * w * 0.18f;
        Rect(wx, y + h * 0.45f, wx + w * 0.13f, y + h * 0.80f);
    }

    glColor3f(0.08f, 0.08f, 0.08f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y); glVertex2f(x + w, y);
    glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
    glLineWidth(1.0f);

    R_wheel4(x + w * 0.18f, y, h * 0.13f);
    R_wheel4(x + w * 0.82f, y, h * 0.13f);
    R_headlightGlow(x + w * 0.99f, y + h * 0.25f);

    glColor3f(1.0f, 1.0f, 1.0f);
    DrawText(x + w * 0.30f, y + h * 0.55f, "CITY");
}

//traffic key
void R_toggleTraffic() {
    if (scene != SCENE4) return;
    trafficMoving = !trafficMoving;
}

//traffic
void R_traffic() {
    R_car(carA, 0.95f, 0.80f, 0.15f);
    R_car(carB, 0.85f, 0.20f, 0.18f);
    R_car(carC, 0.20f, 0.55f, 0.65f);
    R_cityBus(cityBus);
    R_brtcBus(brtcBus);
    R_aiubBus(aiubBus);
}

//rumi part
void Rumi_TrainAndTraffic() {
    switch (scene) {
        case SCENE1:
            break;
        case SCENE2:
            R_train(trainX);
            break;
        case SCENE3:
            break;
        case SCENE4:
            R_traffic();
            break;
    }
}


//part 5 - epu

//ship wake
void E_shipWake(float dx, float dy) {
    if (!shipMoving) return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int i = 0; i < 6; i++) {
        float t = fmod(waterPhase * 1.6f + i * 0.17f, 1.0f);
        float wx = dx - 0.15f - t * 0.85f;
        float wy = dy - 0.895f + sin(waterPhase * 3.0f + i * 1.3f) * 0.006f;
        float alpha = 0.30f * (1.0f - t);
        float rx = 0.018f + t * 0.03f, ry = 0.007f + t * 0.006f;
        glColor4f(1.0f, 1.0f, 1.0f, alpha);
        glPushMatrix();
        glTranslatef(wx, wy, 0.0f);
        glScalef(rx, ry, 1.0f);
        Circle(1.0f, 0.0f, 0.0f);
        glPopMatrix();
    }
    glDisable(GL_BLEND);
}

//ship moving
void E_travelingShip(float dx, float dy) {
    E_shipWake(dx, dy);

    glPushMatrix();
    glTranslatef(dx, dy, 0.0f);

    glColor3f(0.2f, 0.2f, 0.25f);
    glBegin(GL_POLYGON);
    glVertex2f(-0.14f, -0.8f);
    glVertex2f(0.14f, -0.8f);
    glVertex2f(0.112f, -0.88f);
    glVertex2f(-0.112f, -0.88f);
    glEnd();
    glColor3f(0.05f, 0.05f, 0.08f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.14f, -0.8f);
    glVertex2f(0.14f, -0.8f);
    glVertex2f(0.112f, -0.88f);
    glVertex2f(-0.112f, -0.88f);
    glEnd();

    glColor3f(0.75f, 0.1f, 0.1f);
    Rect(-0.112f, -0.88f, 0.112f, -0.86f);

    glColor3f(0.95f, 0.95f, 0.95f);
    Rect(-0.105f, -0.8f, 0.105f, -0.712f);
    glColor3f(0.05f, 0.05f, 0.08f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.105f, -0.8f); glVertex2f(0.105f, -0.8f);
    glVertex2f(0.105f, -0.712f); glVertex2f(-0.105f, -0.712f);
    glEnd();

    glColor3f(0.3f, 0.6f, 0.9f);
    for (int i = 0; i < 8; i++) {
        float wx = -0.089f + i * 0.024f;
        Rect(wx, -0.7648f, wx + 0.010f, -0.7448f);
    }

    glColor3f(0.85f, 0.85f, 0.85f);
    Rect(-0.00524f, -0.712f, 0.06824f, -0.64f);
    glColor3f(0.05f, 0.05f, 0.08f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.00524f, -0.712f); glVertex2f(0.06824f, -0.712f);
    glVertex2f(0.06824f, -0.64f);   glVertex2f(-0.00524f, -0.64f);
    glEnd();

    glColor3f(0.3f, 0.6f, 0.9f);
    Rect(0.00076f, -0.6724f, 0.06224f, -0.658f);

    glColor3f(0.8f, 0.2f, 0.15f);
    Rect(0.01416f, -0.64f, 0.03416f, -0.552f);
    glColor3f(0.1f, 0.1f, 0.1f);
    Rect(0.01416f, -0.572f, 0.03416f, -0.552f);

    glColor3f(0.1f, 0.1f, 0.1f);
    glLineWidth(1.5f);
    Line(-0.081f, -0.712f, -0.081f, -0.632f);
    glLineWidth(1.0f);

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(1.0f);
    Line(-0.14f, -0.8f, 0.14f, -0.8f);
    glPopMatrix();

    drawShadow(dx, dy - 0.895f, 0.16f, 0.02f);
}

//airplane
void E_airplane(float cx, float cy, float tiltDeg, float scale) {
    glPushMatrix();
    glTranslatef(cx, cy, 0.0f);
    glRotatef(tiltDeg, 0.0f, 0.0f, 1.0f);
    glScalef(scale, scale, 1.0f);

    glColor3f(0.80f, 0.81f, 0.84f);
    glBegin(GL_POLYGON);
    glVertex2f(-0.085f, -0.016f);
    glVertex2f(-0.150f, -0.052f);
    glVertex2f(-0.175f, -0.046f);
    glVertex2f(-0.105f, -0.010f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(0.020f, -0.018f);
    glVertex2f(-0.075f, -0.098f);
    glVertex2f(-0.135f, -0.088f);
    glVertex2f(-0.020f, -0.012f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(-0.135f, 0.016f);
    glVertex2f(-0.185f, 0.058f);
    glVertex2f(-0.160f, 0.014f);
    glEnd();

    float body[22] = {
        -0.150f, 0.020f,  -0.100f, 0.038f,  0.000f, 0.042f,  0.090f, 0.034f,
        0.150f, 0.018f,   0.190f, 0.000f,   0.150f, -0.016f, 0.060f, -0.028f,
        -0.060f, -0.028f, -0.130f, -0.014f, -0.160f, 0.000f
    };
    glColor3f(0.62f, 0.80f, 0.90f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 11; i++) glVertex2f(body[i * 2], body[i * 2 + 1]);
    glEnd();

    glColor3f(0.15f, 0.35f, 0.65f);
    Circle(0.017f, 0.085f, 0.014f);

    glColor3f(0.85f, 0.15f, 0.15f);
    glBegin(GL_TRIANGLES);
    glVertex2f(0.145f, 0.020f); glVertex2f(0.175f, 0.040f); glVertex2f(0.175f, 0.018f);
    glEnd();

    glColor3f(0.08f, 0.08f, 0.10f);
    glLineWidth(1.4f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 11; i++) glVertex2f(body[i * 2], body[i * 2 + 1]);
    glEnd();
    glLineWidth(1.0f);
    glPopMatrix();
}

//bird
void E_bird(float cx, float cy, float flap, float rockDeg) {
    glColor3f(0.15f, 0.15f, 0.18f);
    glLineWidth(1.6f);
    glPushMatrix();
    glTranslatef(cx, cy, 0.0f);
    glRotatef(rockDeg, 0.0f, 0.0f, 1.0f);
    glScalef(1.0f, 0.6f + 0.4f * fabs(sin(flap)), 1.0f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(-0.03f, 0.0f);
    glVertex2f(-0.01f, 0.015f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(0.01f, 0.015f);
    glVertex2f(0.03f, 0.0f);
    glEnd();
    glPopMatrix();
    glLineWidth(1.0f);
}

//rain start
void E_initRain() {
    for (int i = 0; i < NUM_DROPS; i++) {
        drops[i].x = -1.2f + (rand() % 2400) / 1000.0f;
        drops[i].y = -1.0f + (rand() % 2000) / 1000.0f;
        drops[i].len = 0.03f + (rand() % 30) / 1000.0f;
    }
}

//rain moving
void E_updateRain() {
    for (int i = 0; i < NUM_DROPS; i++) {
        drops[i].y -= 0.028f;
        drops[i].x -= 0.006f;
        if (drops[i].y < -1.0f) {
            drops[i].y = 1.0f;
            drops[i].x = -1.2f + (rand() % 2400) / 1000.0f;
        }
    }
}

//rain
void E_rain() {
    if (weatherLevel < 0.05f) return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.75f, 0.85f, 0.95f, 0.6f * weatherLevel);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
    int active = (int)(NUM_DROPS * weatherLevel);
    for (int i = 0; i < active; i++) {
        glVertex2f(drops[i].x, drops[i].y);
        glVertex2f(drops[i].x - drops[i].len * 0.35f, drops[i].y - drops[i].len);
    }
    glEnd();
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
}

//lightning point
void E_triggerLightning() {
    float x = -0.7f + (rand() % 1400) / 1000.0f;
    float y = 0.95f;
    boltPoints[0][0] = x;           boltPoints[0][1] = y;
    boltPoints[1][0] = x - 0.03f;   boltPoints[1][1] = y - 0.18f;
    boltPoints[2][0] = x + 0.02f;   boltPoints[2][1] = y - 0.20f;
    boltPoints[3][0] = x - 0.015f;  boltPoints[3][1] = y - 0.40f;
    boltPoints[4][0] = x + 0.03f;   boltPoints[4][1] = y - 0.44f;
    boltPoints[5][0] = x;           boltPoints[5][1] = y - 0.62f;
    sceneIntensity = 1.6f;
}

//lightning
void E_lightning() {
    if (!lightningFlash) return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float flashAlpha = 0.28f * (sceneIntensity - 1.0f) / 0.6f;
    if (flashAlpha < 0.0f) flashAlpha = 0.0f;
    glColor4f(1.0f, 1.0f, 1.0f, flashAlpha);
    Rect(-1.0f, -1.0f, 1.0f, 1.0f);
    glColor3f(1.0f, 0.98f, 0.65f);
    glLineWidth(2.5f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < 6; i++) glVertex2f(boltPoints[i][0], boltPoints[i][1]);
    glEnd();
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
}

//weather
void E_updateWeather() {
    if (weatherLevel > 0.02f) E_updateRain();
    sceneIntensity = Lerp(sceneIntensity, 1.0f, 0.15f);
    if (weatherLevel > 0.05f) {
        lightningTimer += 0.016f;
        if (!lightningFlash && lightningTimer > 1.6f && (rand() % 100) < 3) {
            lightningFlash = true;
            lightningFlashTimer = 0.0f;
            lightningTimer = 0.0f;
            E_triggerLightning();
        }
        if (lightningFlash) {
            lightningFlashTimer += 0.016f;
            if (lightningFlashTimer > 0.15f) lightningFlash = false;
        }
    } else {
        lightningFlash = false;
    }
}

//text
void E_hud1() {
    glColor3f(0.05f, 0.05f, 0.05f);
    if (!scene1ShipDeparted)
        DrawText(-0.98f, 0.92f, "Press 'S' to send the ship off - the train starts on its own next");
}

//text
void E_hud2() {
    glColor3f(0.05f, 0.05f, 0.05f);
    if (!raining)
        DrawText(-0.98f, 0.92f, "Train is moving - press 'R' to bring on the storm");
    else if (!trainAtStation)
        DrawText(-0.98f, 0.92f, "Storm running - the train is easing into the station");
}

//text
void E_hud3() {
    glColor3f(0.05f, 0.05f, 0.05f);
    if (!airplaneLanding)
        DrawText(-0.98f, 0.92f, "Airplane is flying - press 'L' to begin the landing");
}

//text
void E_hud4() {
    glColor3f(dayNightT > 0.5f ? 1.0f : 0.05f, dayNightT > 0.5f ? 1.0f : 0.05f, dayNightT > 0.5f ? 1.0f : 0.05f);
    if (!shipStoppedInCity)
        DrawText(-0.98f, 0.92f, "Press 'E' to stop the ship");
}

//key list
void E_hudKeys() {
    glColor3f(dayNightT > 0.5f && scene == SCENE4 ? 0.90f : 0.15f,
              dayNightT > 0.5f && scene == SCENE4 ? 0.90f : 0.15f,
              dayNightT > 0.5f && scene == SCENE4 ? 0.90f : 0.15f);
    DrawText(-0.98f, -0.97f, "N day/night   W windmill   B lights   T traffic   SPACE pause");
}

//pause banner
void E_pausedBanner() {
    if (!paused) return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.40f);
    Rect(-0.33f, 0.785f, 0.30f, 0.865f);
    glDisable(GL_BLEND);
    glColor3f(1.0f, 0.95f, 0.35f);
    DrawText(-0.30f, 0.810f, "PAUSED - press SPACE to continue");
}

//epu part
void Epu_ShipPlaneAndWeather() {
    switch (scene) {
        case SCENE1:
            E_travelingShip(shipX, shipDY);
            E_hud1();
            break;
        case SCENE2:
            E_travelingShip(shipX, shipDY);
            E_rain();
            E_lightning();
            E_hud2();
            break;
        case SCENE3:
            E_travelingShip(shipX, shipDY);
            if (airplaneVisible) E_airplane(planeX, planeY, planeTilt, planeScale);
            E_bird(fmod(0.4f + birdPhase * 0.6f, 2.6f) - 1.3f, 0.68f, birdPhase * 6.0f, 6.0f * sin(birdPhase * 2.0f));
            E_bird(fmod(1.0f + birdPhase * 0.5f, 2.6f) - 1.3f, 0.60f, birdPhase * 6.0f + 2.0f, -6.0f * sin(birdPhase * 2.0f));
            E_rain();
            E_lightning();
            E_hud3();
            break;
        case SCENE4:
            E_travelingShip(shipX, shipDY);
            E_hud4();
            break;
    }
    E_hudKeys();
    E_pausedBanner();
}

//display
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    Tonmoy_SkyAndBackground();
    Mridul_LandWaterAndTrack();
    Faysal_BuildingsAndStructures();
    Rumi_TrainAndTraffic();
    Epu_ShipPlaneAndWeather();

    glutSwapBuffers();
}

//scene 1
void updateScene1() {
    if (shipMoving && !Reversing()) {
        shipX += SHIP_SPEED;
        if (shipX > 1.5f) {
            scene = SCENE2;
            shipX = TRAIN_START_X - 0.15f;
            shipDY = 0.18f;

            trainMoving = true;
            trainSpeed = BASE_TRAIN_SPEED;
            trainX = TRAIN_START_X;
        }
    }
}

//scene 2
void updateScene2() {
    if (trainMoving && !trainDecelerating && !Reversing()) {
        trainX += trainSpeed;

        if (trainX > 1.8f) trainX = TRAIN_START_X;
    }

    if (raining && !Reversing()) {
        weatherLevel = Clamp01(weatherLevel + 0.006f);
    }

    if (trainDecelerating && !Reversing()) {
        double t = secondsSince(decelStartTime) / decelDurationSeconds;
        if (t >= 1.0) {
            trainX = STATION_STOP_X;
            trainMoving = false;
            trainDecelerating = false;
            trainAtStation = true;

            scene = SCENE3;
            airplaneMoving = true;
            airplaneLanding = false;
            airplaneVisible = true;
            planeX = -1.4f; planeY = PLANE_CRUISE_Y; planeTilt = 0.0f; planeScale = 1.0f;
            shipX = -1.35f;
            shipDY = 0.18f;
        } else {
            trainX = Lerp(decelStartX, STATION_STOP_X, SmoothStep((float)t));
        }
    }

    if (shipMoving && !Reversing()) {
        shipX += SHIP_SPEED;
        if (shipX > 1.4f) shipX = -1.4f;
    }

    wheelAngle += trainMoving ? 6.0f : 0.0f;
    if (wheelAngle > 360.0f) wheelAngle -= 360.0f;

    E_updateWeather();
}

//scene 3
void updateScene3() {
    if (!Reversing()) weatherLevel = Clamp01(weatherLevel - 0.0012f);
    birdPhase += 0.016f;

    if (loopWx && weatherLevel < 0.05f) stopStormSound();

    if (shipMoving && !Reversing()) {
        shipX += SHIP_SPEED;
        if (shipX > 1.4f) shipX = -1.4f;
    }

    if (airplaneMoving && !airplaneLanding && !Reversing()) {
        planeX += PLANE_SPEED;
        planeY = PLANE_CRUISE_Y + 0.02f * sin(planeX * 3.0f);
        planeTilt = 0.0f;
        planeScale = 1.0f;
        if (planeX > 1.4f) planeX = -1.4f;
    }

    if (airplaneLanding && !Reversing()) {
        double t = secondsSince(landingStartTime) / LANDING_DURATION_SECONDS;
        if (t >= 1.0) {
            airplaneVisible = false;
            airplaneLanding = false;

            scene = SCENE4;
            shipX = -1.2f;
            shipDY = 0.0f;
            shipMoving = true;
            shipStoppedInCity = false;
            dayNightT = 0.0f;
        } else {
            float e = SmoothStep((float)t);
            planeX = Lerp(planeLandStartX, LANDING_TARGET_X, e);
            planeY = Lerp(planeLandStartY, LANDING_TARGET_Y, e);
            planeTilt = Lerp(0.0f, -18.0f, e);
            planeScale = Lerp(1.0f, 0.55f, e);
        }
    }

    E_updateWeather();
}

//vehicle move
void wrapVehicle(Vehicle &v) {
    v.x += v.speed;
    if (v.x > 1.4f) v.x = -1.5f - v.w;
}

//scene 4
void updateScene4() {
    if (!Reversing()) dayNightT = Clamp01(dayNightT + 0.00045f);
    sceneIntensity = Lerp(1.0f, 0.62f, dayNightT);

    windowBlinkTimer += 0.016f;
    if (windowBlinkTimer > 1.4f) { windowBlinkTimer = 0.0f; windowBlinkOn = !windowBlinkOn; }

    if (trafficMoving) {
        wrapVehicle(carA); wrapVehicle(carB); wrapVehicle(carC);
        wrapVehicle(brtcBus); wrapVehicle(aiubBus); wrapVehicle(cityBus);
    }

    if (shipMoving && !shipStoppedInCity && !Reversing()) {
        shipX += SHIP_SPEED;
        if (shipX > 1.4f) shipX = -1.4f;
    }
}

//animation
void update(int value) {
    if (paused) {
        glutPostRedisplay();
        glutTimerFunc(16, update, 0);
        return;
    }

    static int lastAudioScene = -1;
    if ((int)scene != lastAudioScene) {
        lastAudioScene = (int)scene;
        sceneAudio(lastAudioScene + 1);
    }
    if (shipMoving) shipSoundOn(); else shipSoundOff();
    keepLooping();

    bladeAngle += bladeSpeed;
    if (bladeAngle > 360.0f) bladeAngle -= 360.0f;

    sunAngle += 0.6f;
    if (sunAngle > 360.0f) sunAngle -= 360.0f;

    waterPhase += 0.016f;

    if (reverseHoldTimer > 0.0f) reverseHoldTimer -= 0.016f;

    switch (scene) {
        case SCENE1: updateScene1(); break;
        case SCENE2: updateScene2(); break;
        case SCENE3: updateScene3(); break;
        case SCENE4: updateScene4(); break;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

const float REV_SHIP_STEP     = 0.030f;
const float REV_TRAIN_STEP    = 0.020f;
const float REV_PLANE_STEP    = 0.030f;
const float REV_WEATHER_STEP  = 0.030f;
const float REV_DAYNIGHT_STEP = 0.020f;
const float REV_HOLD_SECONDS  = 0.25f;

//rewind
void reverseStep() {
    reverseHoldTimer = REV_HOLD_SECONDS;

    if (scene == SCENE4) {
        dayNightT = Clamp01(dayNightT - REV_DAYNIGHT_STEP);
        shipStoppedInCity = false;
        shipMoving = true;
        shipX -= REV_SHIP_STEP;
        if (shipX <= -1.3f) {

            scene = SCENE3;
            weatherLevel = 0.0f;
            airplaneMoving = true;
            airplaneLanding = false;
            airplaneVisible = true;
            planeX = -0.2f;
            planeY = PLANE_CRUISE_Y;
            planeTilt = 0.0f;
            planeScale = 1.0f;
            shipX = -0.2f;
            shipDY = 0.18f;
            shipMoving = true;
        }
        return;
    }

    if (scene == SCENE3) {
        airplaneLanding = false;
        airplaneMoving = true;
        airplaneVisible = true;
        weatherLevel = Clamp01(weatherLevel + REV_WEATHER_STEP);
        planeX -= REV_PLANE_STEP;
        shipMoving = true;
        shipX -= REV_SHIP_STEP;
        if (planeX <= -1.4f && weatherLevel >= 0.999f) {

            scene = SCENE2;
            trainAtStation = false;
            trainDecelerating = false;
            trainMoving = true;
            trainX = STATION_STOP_X - 0.35f;
            raining = false;
            weatherLevel = 0.0f;
            shipX = 1.0f;
            shipDY = 0.18f;
            shipMoving = true;
        }
        return;
    }

    if (scene == SCENE2) {
        trainDecelerating = false;
        trainAtStation = false;
        trainMoving = true;
        trainX -= REV_TRAIN_STEP;
        shipMoving = true;
        shipX -= REV_SHIP_STEP;
        if (raining) {
            weatherLevel = Clamp01(weatherLevel - REV_WEATHER_STEP);
            if (weatherLevel <= 0.0f) { raining = false; stopStormSound(); }
        }
        if (trainX <= TRAIN_START_X) {

            scene = SCENE1;
            scene1ShipDeparted = false;
            shipMoving = false;
            shipX = 0.0f;
            shipDY = 0.0f;
            trainMoving = false;
            trainDecelerating = false;
            trainAtStation = false;
            trainX = TRAIN_START_X;
            raining = false;
            weatherLevel = 0.0f;
        }
        return;
    }

    if (scene == SCENE1) {
        shipX -= REV_SHIP_STEP;
        if (shipX <= 0.0f) {
            shipX = 0.0f;
            shipMoving = false;
            scene1ShipDeparted = false;
        }
        return;
    }
}

//ship start
void triggerShipDeparture() {
    if (scene != SCENE1 || scene1ShipDeparted) return;
    scene1ShipDeparted = true;
    shipMoving = true;
    shipSoundOn();
}

//thunder
void triggerRain() {
    if (scene != SCENE2 || !trainMoving || raining) return;
    raining = true;
    E_initRain();
    loopWx = "storm";
    playSound("storm", true);

    if (trainX >= STATION_STOP_X - 0.05f) {
        trainX = STATION_APPROACH_X;
    }

    trainDecelerating = true;
    decelStartTime = NowMs();
    decelStartX = trainX;

    float remaining = STATION_STOP_X - decelStartX;
    decelDurationSeconds = remaining / 0.30;
    if (decelDurationSeconds < 1.5) decelDurationSeconds = 1.5;
    if (decelDurationSeconds > 7.0) decelDurationSeconds = 7.0;
}

//plane landing
void triggerLanding() {
    if (scene != SCENE3 || !airplaneMoving || airplaneLanding) return;
    airplaneLanding = true;
    landingStartTime = NowMs();
    planeLandStartX = planeX;
    planeLandStartY = planeY;
    playSound("land", false);
}

//ship stop
void triggerShipStop() {
    if (scene != SCENE4 || !shipMoving) return;
    shipMoving = false;
    shipStoppedInCity = true;
    shipSoundOff();
}

//next scene
void forwardStep() {
    switch (scene) {
        case SCENE1:
            scene = SCENE2;
            scene1ShipDeparted = true;
            trainMoving = true;
            trainSpeed = BASE_TRAIN_SPEED;
            trainX = TRAIN_START_X;
            raining = false;
            trainDecelerating = false;
            trainAtStation = false;
            weatherLevel = 0.0f;
            shipMoving = true;
            shipX = TRAIN_START_X - 0.15f;
            shipDY = 0.18f;
            break;

        case SCENE2:
            scene = SCENE3;
            trainMoving = false;
            trainDecelerating = false;
            trainAtStation = true;
            trainX = STATION_STOP_X;
            raining = false;
            weatherLevel = 1.0f;
            airplaneMoving = true;
            airplaneLanding = false;
            airplaneVisible = true;
            planeX = -1.4f; planeY = PLANE_CRUISE_Y; planeTilt = 0.0f; planeScale = 1.0f;
            shipMoving = true;
            shipX = -1.35f;
            shipDY = 0.18f;
            break;

        case SCENE3:
            scene = SCENE4;
            airplaneVisible = false;
            airplaneLanding = false;
            airplaneMoving = false;
            shipMoving = true;
            shipX = -1.2f;
            shipDY = 0.0f;
            shipStoppedInCity = false;
            dayNightT = 0.0f;
            break;

        case SCENE4:
            triggerShipStop();
            break;
    }
}

//pause
void togglePause() {
    if (!paused) {
        paused = true;
        pauseStartTime = NowMs();
        pauseAllSound();
    } else {
        DWORD held = GetTickCount() - pauseStartTime;
        decelStartTime += held;
        landingStartTime += held;
        paused = false;
        resumeAllSound();
    }
}

//keyboard
void keyboard(unsigned char key, int x, int y) {
    if (key == 27) { closeAllSound(); exit(0); }

    if (key == ' ') { togglePause(); return; }
    if (paused) return;

    if (key == 's' || key == 'S') triggerShipDeparture();
    else if (key == 'r' || key == 'R') triggerRain();
    else if (key == 'l' || key == 'L') triggerLanding();
    else if (key == 'e' || key == 'E') triggerShipStop();
    else if (key == 'n' || key == 'N') T_toggleDayNight();
    else if (key == 'w' || key == 'W') M_cycleWindmillSpeed();
    else if (key == 'b' || key == 'B') F_toggleLights();
    else if (key == 't' || key == 'T') R_toggleTraffic();
}

//arrow keys
void specialKeys(int key, int x, int y) {
    if (paused) return;
    if (key == GLUT_KEY_LEFT) reverseStep();
    else if (key == GLUT_KEY_RIGHT) forwardStep();
}

//vehicle
void setVehicle(Vehicle &v, float x, float speed, float y, float w, float h) {
    v.x = x; v.speed = speed; v.y = y; v.w = w; v.h = h;
}

//vehicles
void initCity() {
    setVehicle(carA,   -1.3f, 0.0060f, CITY_ROAD_BOTTOM + 0.055f, 0.14f, 0.10f);
    setVehicle(carB,    0.2f, 0.0048f, CITY_ROAD_BOTTOM + 0.055f, 0.14f, 0.10f);
    setVehicle(carC,    0.9f, 0.0072f, CITY_ROAD_BOTTOM + 0.055f, 0.14f, 0.10f);
    setVehicle(brtcBus, -0.6f, 0.0032f, CITY_ROAD_BOTTOM + 0.03f, 0.34f, 0.24f);
    setVehicle(aiubBus,  0.5f, 0.0040f, CITY_ROAD_BOTTOM + 0.04f, 0.24f, 0.17f);
    setVehicle(cityBus, -1.0f, 0.0026f, CITY_ROAD_BOTTOM + 0.045f, 0.28f, 0.15f);
}

//main
int main(int argc, char** argv) {
    srand((unsigned)time(0));
    initCity();
    E_initRain();
    initSound();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1000, 500);
    glutCreateWindow("From Nature to Urban Life");
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(16, update, 0);
    glutMainLoop();
    closeAllSound();
    return 0;
}
