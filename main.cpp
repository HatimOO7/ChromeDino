#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <sstream>

const int WIN_W = 900;
const int WIN_H = 400;

const float GROUND_Y   = 80.0f;   // y-position of ground line

struct Dino {
    float x = 100.0f;
    float y = GROUND_Y;           // feet position
    float vy = 0.0f;
    bool  jumping = false;

    // body dimensions
    float w = 40.0f;
    float h = 55.0f;

    // leg animation
    int   legFrame   = 0;
    float legTimer   = 0.0f;
    float legSpeed   = 0.12f;
};

struct Cactus {
    float x;
    float w;
    float h;
    bool  scored = false;   // true after dino passes it

    Cactus(float startX) {
        x = startX;
        // random size variety
        w = 18.0f + (rand() % 14);
        h = 35.0f + (rand() % 30);
    }
};

struct Cloud {
    float x, y, scale;
};

enum GameState { PLAYING, GAME_OVER };

GameState  gState       = PLAYING;
int        gScore       = 0;
int        gBestScore   = 0;             // Added variable to track best score
float      gSpeed       = 5.5f;          // pixels per frame
float      gSpeedMax    = 18.0f;
float      gSpeedInc    = 0.002f;

Dino       dino;
std::vector<Cactus> cacti;
std::vector<Cloud>  clouds;

float      groundOffset = 0.0f;          // scrolling ground dashes
float      spawnTimer   = 0.0f;
float      spawnInterval= 90.0f;         // frames between cacti

const float GRAVITY     = -0.65f;
const float JUMP_VEL    =  14.0f;

void drawRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
      glVertex2f(x,     y);
      glVertex2f(x + w, y);
      glVertex2f(x + w, y + h);
      glVertex2f(x,     y + h);
    glEnd();
}

void drawText(float x, float y, const std::string& text, float r=0.15f, float g=0.15f, float b=0.15f) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : text)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}

void drawTextLarge(float x, float y, const std::string& text) {
    glColor3f(0.1f, 0.1f, 0.1f);
    glRasterPos2f(x, y);
    for (char c : text)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
}

std::string intToStr(int v) {
    std::ostringstream ss;
    ss << v;
    return ss.str();
}

// AABB collision
bool collides(const Dino& d, const Cactus& c) {
    float dx = d.x,       dy = d.y,       dw = d.w * 0.75f, dh = d.h;
    float cx = c.x + 2.f, cy = GROUND_Y,  cw = c.w - 4.f,   ch = c.h;

    // boxes overlap?
    return (dx < cx + cw) && (dx + dw > cx) &&
           (dy < cy + ch) && (dy + dh > cy);
}

void spawnCactus() {
    float x = WIN_W + 20.0f;
    // sometimes spawn a pair
    cacti.push_back(Cactus(x));
    if (rand() % 3 == 0)
        cacti.push_back(Cactus(x + 35.0f + rand() % 20));
}

void initGame() {
    gState   = PLAYING;
    gScore   = 0;
    gSpeed   = 5.5f;
    dino     = Dino();
    cacti.clear();
    clouds.clear();
    groundOffset = 0.0f;
    spawnTimer   = 0.0f;
    spawnInterval= 90.0f;

    for (int i = 0; i < 5; ++i) {
        Cloud cl;
        cl.x     = 100.0f + i * 180.0f;
        cl.y     = 200.0f + (rand() % 100);
        cl.scale = 0.6f + (rand() % 5) * 0.1f;
        clouds.push_back(cl);
    }
}

void drawDino() {
    float x = dino.x;
    float y = dino.y;       // bottom of feet

    glColor3f(0.25f, 0.25f, 0.25f);

    // body
    drawRect(x, y + 12, dino.w, dino.h - 20);

    // neck + head
    drawRect(x + dino.w - 14, y + dino.h - 24, 22, 24);

    // eye
    glColor3f(0.9f, 0.9f, 0.9f);
    drawRect(x + dino.w + 2, y + dino.h - 10, 7, 7);
    glColor3f(0.1f, 0.1f, 0.1f);
    drawRect(x + dino.w + 4, y + dino.h - 8, 4, 4);

    // tail
    glColor3f(0.25f, 0.25f, 0.25f);
    drawRect(x - 18, y + 20, 22, 12);
    drawRect(x - 26, y + 14, 12, 10);

    // legs with animation
    float leg1Y, leg2Y;
    if (dino.jumping) {
        leg1Y = -6; leg2Y = 0;
    } else {
        float t = sinf(dino.legTimer * 6.0f);
        leg1Y =  t * 6.0f;
        leg2Y = -t * 6.0f;
    }
    glColor3f(0.25f, 0.25f, 0.25f);
    drawRect(x + 6,  y + leg1Y,       12, 14);
    drawRect(x + 22, y + leg2Y,       12, 14);
}

void drawCactus(const Cactus& c) {
    glColor3f(0.18f, 0.55f, 0.18f);

    float x = c.x;
    float base_y = GROUND_Y;

    // main trunk
    drawRect(x + c.w/2 - 4, base_y, 10, c.h);

    // left arm
    float armH = c.h * 0.55f;
    drawRect(x,           base_y + armH - 14, c.w/2 - 2, 9);
    drawRect(x,           base_y + armH - 24, 9, 24);

    // right arm
    drawRect(x + c.w/2 + 4, base_y + armH - 20, c.w/2 - 4, 9);
    drawRect(x + c.w - 9,   base_y + armH - 32, 9, 28);
}

void drawCloud(const Cloud& cl) {
    glColor3f(0.88f, 0.88f, 0.88f);
    float x = cl.x, y = cl.y, s = cl.scale * 40.0f;

    // three overlapping ellipses
    auto ellipse = [&](float cx, float cy, float rx, float ry) {
        int segs = 20;
        glBegin(GL_POLYGON);
        for (int i = 0; i < segs; i++) {
            float a = 2.0f * M_PI * i / segs;
            glVertex2f(cx + rx * cosf(a), cy + ry * sinf(a));
        }
        glEnd();
    };
    ellipse(x,        y,      s * 0.9f, s * 0.45f);
    ellipse(x - s,   y - 4,  s * 0.65f, s * 0.35f);
    ellipse(x + s,   y - 4,  s * 0.65f, s * 0.35f);
}

void drawGround() {
    glColor3f(0.3f, 0.3f, 0.3f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
      glVertex2f(0, GROUND_Y);
      glVertex2f(WIN_W, GROUND_Y);
    glEnd();

    // dashed pebbles
    glColor3f(0.6f, 0.6f, 0.6f);
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    for (float px = fmod(-groundOffset, 60.0f); px < WIN_W; px += 60.0f) {
        glVertex2f(px + 10, GROUND_Y - 5);
        glVertex2f(px + 35, GROUND_Y - 3);
    }
    glEnd();
}

void update() {
    if (gState == GAME_OVER) return;

    // speed ramp
    gSpeed = std::min(gSpeed + gSpeedInc, gSpeedMax);
    spawnInterval = std::max(55.0f, spawnInterval - 0.005f);

    // ground scroll
    groundOffset += gSpeed;

    for (auto& cl : clouds) {
        cl.x -= gSpeed * 0.25f;
        if (cl.x < -200) {
            cl.x = WIN_W + 50;
            cl.y = 160.0f + rand() % 130;
        }
    }

    if (dino.jumping) {
        dino.vy += GRAVITY;
        dino.y  += dino.vy;
        if (dino.y <= GROUND_Y) {
            dino.y  = GROUND_Y;
            dino.vy = 0;
            dino.jumping = false;
        }
    } else {
        dino.legTimer += dino.legSpeed;
    }

    // spawn cacti
    spawnTimer++;
    if (spawnTimer >= spawnInterval) {
        spawnCactus();
        spawnTimer = 0;
    }

    // move cacti
    for (auto& c : cacti) {
        c.x -= gSpeed;

        // score
        if (!c.scored && (c.x + c.w) < dino.x) {
            c.scored = true;
            gScore  += 10;
        }
    }

    // remove off-screen cacti
    while (!cacti.empty() && cacti.front().x + cacti.front().w < -20)
        cacti.erase(cacti.begin());

    // collision
    for (const auto& c : cacti) {
        if (collides(dino, c)) {
            gState = GAME_OVER;
            // Record best score
            if (gScore > gBestScore) {
                gBestScore = gScore;
            }
            return;
        }
    }
}

void display() {
    glClearColor(0.95f, 0.95f, 0.92f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // clouds
    for (const auto& cl : clouds) drawCloud(cl);

    // ground
    drawGround();

    // cacti
    for (const auto& c : cacti) drawCactus(c);

    // dino
    drawDino();

    // score
    std::string scoreStr = "SCORE: " + intToStr(gScore);
    drawText(WIN_W - 200, WIN_H - 35, scoreStr);

    // speed indicator
    std::string speedStr = "SPD: " + intToStr((int)gSpeed);
    drawText(20, WIN_H - 35, speedStr, 0.5f, 0.5f, 0.5f);

    if (gState == GAME_OVER) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f, 0.0f, 0.0f, 0.35f);
        // Box height increased slightly to fit all text beautifully
        drawRect(WIN_W/2 - 200, WIN_H/2 - 80, 400, 160);
        glDisable(GL_BLEND);

        // Best score moved below the Final score
        drawTextLarge(WIN_W/2 - 80, WIN_H/2 + 45, "GAME OVER!");
        drawTextLarge(WIN_W/2 - 80, WIN_H/2 + 15, "Final Score: " + intToStr(gScore));
        drawTextLarge(WIN_W/2 - 80, WIN_H/2 - 15, "Best Score: " + intToStr(gBestScore));

        drawText(WIN_W/2 - 80, WIN_H/2 - 50, "Press  R  to Restart", 0.2f, 0.5f, 0.2f);
    }

    // instructions
    if (gState == PLAYING && gScore == 0 && !dino.jumping) {
        drawText(WIN_W/2 - 140, GROUND_Y + 100, "SPACE / UP  =  Jump", 0.45f, 0.45f, 0.45f);
    }

    glutSwapBuffers();
}

void timer(int) {
    update();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void keyboard(unsigned char key, int, int) {
    if (key == ' ') {
        if (gState == PLAYING && !dino.jumping) {
            dino.jumping = true;
            dino.vy = JUMP_VEL;
        }
    }
    if ((key == 'r' || key == 'R') && gState == GAME_OVER) {
        initGame();
    }
    if (key == 27) exit(0);   // ESC quit
}

void specialKeys(int key, int, int) {
    if (key == GLUT_KEY_UP) {
        if (gState == PLAYING && !dino.jumping) {
            dino.jumping = true;
            dino.vy = JUMP_VEL;
        }
    }
}

int main(int argc, char** argv) {
    srand((unsigned)time(nullptr));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Chrome Dino Runner  |  OpenGL");

    initGame();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}
