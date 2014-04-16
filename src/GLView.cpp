#include "GLView.h"

#include <GL/glut.h>
#include <stdio.h>

void gl_processNormalKeys(unsigned char key, int x, int y)
{
    Vector2<int> mouse(x, y);
    GLVIEW->processNormalKeys(key, mouse);
}
void gl_changeSize(int w, int h)
{
    GLVIEW->changeSize(w, h);
}
void gl_handleIdle()
{
    GLVIEW->handleIdle();
}
void gl_processMouse(int button, int state, int x, int y)
{
    Vector2<int> mouse(x, y);
    GLVIEW->processMouse(button, state, mouse);
}
void gl_processMouseActiveMotion(int x, int y)
{
    Vector2<int> mouse(x, y);
    GLVIEW->processMouseActiveMotion(mouse);
}
void gl_renderScene()
{
    GLVIEW->renderScene();
}


void RenderString(float x, float y, void *font, const char *string, float r, float g, float b)
{
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    int len = (int) strlen(string);
    for (int i = 0; i < len; ++i)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, string[i]);
}

void drawCircle(float x, float y, float r)
{
    for (int k = 0; k < 17; k++)
    {
        float n = k * (M_PI / 8);
        glVertex3f(x + r * sin(n), y + r * cos(n), 0);
    }
}


GLView::GLView(World *s) :
    world(s),
    paused(false),
    draw(true),
    skipdraw(1),
    drawfood(true),
    modcounter(0),
    lastUpdate(0),
    frames(0)
{

    xtranslate = 0.0;
    ytranslate = 0.0;
    scalemult = 0.2; //1.0;
    downb[0] = 0;
    downb[1] = 0;
    downb[2] = 0;
    prev_mouse.x = 0;
    prev_mouse.y = 0;

    following = false;
}

GLView::~GLView()
{

}
void GLView::changeSize(int w, int h)
{
    // Reset the coordinate system before modifying
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, conf::WWIDTH, conf::WHEIGHT, 0, 0, 1);
}

void GLView::processMouse(int button, int state, const Vector2<int> &mouse)
{
    //printf("MOUSE EVENT: button=%i state=%i x=%i y=%i\n", button, state, x, y);

    //have world deal with it. First translate to world coordinates though
    if (button == 0)
    {
        int wx = (int)((mouse.x - conf::WWIDTH / 2) / scalemult) - xtranslate;
        int wy = (int)((mouse.y - conf::WHEIGHT / 2) / scalemult) - ytranslate;
        world->processMouse(button, state, wx, wy);
    }

    prev_mouse = mouse;
    downb[button] = 1 - state; //state is backwards, ah well
}

void GLView::processMouseActiveMotion(const Vector2<int> &mouse)
{
    //printf("MOUSE MOTION x=%i y=%i, %i %i %i\n", x, y, downb[0], downb[1], downb[2]);

    if (downb[1] == 1)
    {
        //mouse wheel. Change scale
        scalemult -= 0.002 * (mouse.y - prev_mouse.y);
        if (scalemult < 0.01)
            scalemult = 0.01;
    }

    if (downb[2] == 1)
    {
        //right mouse button. Pan around
        xtranslate += 2 * (mouse.x - prev_mouse.x);
        ytranslate += 2 * (mouse.y - prev_mouse.y);
    }

//    printf("%f %f %f \n", scalemult, xtranslate, ytranslate);

    prev_mouse = mouse;
}

void GLView::processNormalKeys(unsigned char key, const Vector2<int> &mouse)
{
    switch (key)
    {
        case 'a':
            for (int i = 0; i < 10; ++i)
            {
                world->addNewByCrossover();
            }
            break;
        case 'c':
            world->setClosed(!world->isClosed());
            printf("Environment closed now= %i\n", world->isClosed());
            break;
        case 'd':
            draw = !draw;
            break;
        case 'f':
            drawfood = !drawfood;
            break;
        case 'h':
            for (int i = 0; i < 10; ++i)
            {
                world->addHerbivore();
            }
            break;
        case 'o':
            if (following == 0)
                following = 1; //follow oldest agent: toggle
            else
                following = 0;
            break;
        case 'p':
            paused = !paused;
            break;
        case 'q':
            for (int i = 0; i < 10; ++i)
            {
                world->addCarnivore();
            }
            break;
        case 'r':
            world->reset();
            printf("World reset\n");
            break;
        case 's':
            if (following == 0)
                following = 2;
            else
                following = 0;
            break;
        case 27:
            exit(0);
            break;
        case 43:
            skipdraw++;
            break;
        case 45:
            skipdraw--;
            break;
        default:
            printf("Unknown key pressed: %i\n", key);
            break;
    }
}

void GLView::handleIdle()
{
    modcounter++;
    if (!paused)
        world->update();

    //show FPS
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    frames++;
    if ((currentTime - lastUpdate) >= 1000)
    {
        std::pair<int, int> num_herbs_carns = world->numHerbCarnivores();
        char buf[256];
        sprintf(buf, "FPS: %d NumAgents: %lu Carnivores: %d Herbivores: %d Epoch: %d",
                frames,
                world->numAgents(),
                num_herbs_carns.second,
                num_herbs_carns.first,
                world->epoch());
        glutSetWindowTitle(buf);
        frames = 0;
        lastUpdate = currentTime;
    }
    if (skipdraw <= 0 && draw)
    {
        clock_t endwait;
        float mult = -0.005 * (skipdraw - 1); //ugly, ah well
        endwait = clock() + mult * CLOCKS_PER_SEC ;
        while (clock() < endwait) {}
    }

    if (draw)
    {
        if (skipdraw > 0)
        {
            if (modcounter % skipdraw == 0)
                renderScene(); //increase fps by skipping drawing
        }
        else
        {
            renderScene(); //we will decrease fps by waiting using clocks
        }
    }

}

void GLView::renderScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();

    glTranslatef(conf::WWIDTH / 2, conf::WHEIGHT / 2, 0.0f);
    glScalef(scalemult, scalemult, 1.0f);

    if (following == 0)
    {
        glTranslatef(xtranslate, ytranslate, 0.0f);
    }
    else
    {
        Vector2f pos(0, 0);
        world->positionOfInterest(following, &pos);
        //xi= (conf::WWIDTH/2-xi); //*scalemult;
        //yi= (conf::WHEIGHT/2-yi); //*scalemult;

        glTranslatef(-pos.x, -pos.y, 0.0f);

        //reset this if there is no interest. Probably agent that was followed died
        if (pos.x == 0 && pos.y == 0)
            following = 0;
    }

    world->draw(this, drawfood);

    glPopMatrix();
    glutSwapBuffers();
}

void GLView::drawAgent(const Agent &agent)
{
    float n;
    float r = conf::BOTRADIUS;
    float rp = conf::BOTRADIUS + 2;
    //handle selected agent
    if (agent.selectflag > 0)
    {

        //draw selection
        glBegin(GL_POLYGON);
        glColor3f(1, 1, 0);
        drawCircle(agent.pos.x, agent.pos.y, conf::BOTRADIUS + 5);
        glEnd();

        glPushMatrix();
        glTranslatef(agent.pos.x - 80, agent.pos.y + 20, 0);
        //draw inputs, outputs
        float col;
        float yy = 15;
        float xx = 15;
        float ss = 16;
        glBegin(GL_QUADS);
        for (int j = 0; j < INPUT_COUNT; ++j)
        {
            col = agent.in[j];
            glColor3f(col, col, col);
            glVertex3f(0 + ss * j, 0, 0.0f);
            glVertex3f(xx + ss * j, 0, 0.0f);
            glVertex3f(xx + ss * j, yy, 0.0f);
            glVertex3f(0 + ss * j, yy, 0.0f);
        }
        yy += 5;
        for (int j = 0; j < OUTPUT_COUNT; ++j)
        {
            col = agent.out[j];
            glColor3f(col, col, col);
            glVertex3f(0 + ss * j, yy, 0.0f);
            glVertex3f(xx + ss * j, yy, 0.0f);
            glVertex3f(xx + ss * j, yy + ss, 0.0f);
            glVertex3f(0 + ss * j, yy + ss, 0.0f);
        }
        yy += ss * 2;

        //draw brain. Eventually move this to brain class?
        float offx = 0;
        ss = 8;
        xx = ss;
        for (int j = 0; j < BRAINSIZE; ++j)
        {
            col = agent.brain.boxes[j].out;
            glColor3f(col, col, col);

            glVertex3f(offx + 0 + ss * j, yy, 0.0f);
            glVertex3f(offx + xx + ss * j, yy, 0.0f);
            glVertex3f(offx + xx + ss * j, yy + ss, 0.0f);
            glVertex3f(offx + ss * j, yy + ss, 0.0f);

            if ((j + 1) % 30 == 0)
            {
                yy += ss;
                offx -= ss * 30;
            }
        }

        /*
        glEnd();
        glBegin(GL_LINES);
        float offx=0;
        ss=30;
        xx=ss;
        for (int j=0;j<BRAINSIZE;++j)
        {
            for(int k=0;k<CONNS;k++)
            {
                int j2= agent.brain.boxes[j].id[k];

                //project indices j and j2 into pixel space
                float x1= 0;
                float y1= 0;
                if (j<INPUTSIZE)
                {
                    x1= j*ss;
                    y1= yy;
                }
                else
                {
                    x1= ((j-INPUTSIZE)%30)*ss;
                    y1= yy+ss+2*ss*((int) (j-INPUTSIZE)/30);
                }

                float x2= 0;
                float y2= 0;
                if (j2<INPUTSIZE)
                {
                    x2= j2*ss;
                    y2= yy;
                }
                else
                {
                    x2= ((j2-INPUTSIZE)%30)*ss;
                    y2= yy+ss+2*ss*((int) (j2-INPUTSIZE)/30);
                }

                float ww= agent.brain.boxes[j].w[k];
                if (ww<0)
                    glColor3f(-ww, 0, 0);
                else glColor3f(0,0,ww);

                glVertex3f(x1,y1,0);
                glVertex3f(x2,y2,0);
            }
        }
        */

        glEnd();
        glPopMatrix();
    }

    //draw giving/receiving
    if (agent.dfood != 0)
    {
        glBegin(GL_POLYGON);
        float mag = Clamp(abs(agent.dfood) / conf::FOODTRANSFER / 3, 0.0f, 1.0f);
        if (agent.dfood > 0)
            glColor3f(0, mag, 0); //draw boost as green outline
        else
            glColor3f(mag, 0, 0);
        for (int k = 0; k < 17; k++)
        {
            n = k * (M_PI / 8);
            glVertex3f(agent.pos.x + rp * sin(n), agent.pos.y + rp * cos(n), 0);
            n = (k + 1) * (M_PI / 8);
            glVertex3f(agent.pos.x + rp * sin(n), agent.pos.y + rp * cos(n), 0);
        }
        glEnd();
    }

    //draw indicator of this agent... used for various events
    if (agent.indicator > 0)
    {
        glBegin(GL_POLYGON);
        glColor3f(agent.ir, agent.ig, agent.ib);
        drawCircle(agent.pos.x, agent.pos.y, conf::BOTRADIUS + ((int)agent.indicator));
        glEnd();
    }


    //draw eyes
    glBegin(GL_LINES);
    glColor3f(0.5, 0.5, 0.5);
    for (int q = 0; q < NUMEYES; q++)
    {
        glVertex3f(agent.pos.x, agent.pos.y, 0);
//        float aa= agent.angle+agent.eyedir[q]+agent.eyefov[q];
        float aa = agent.angle + agent.eyes[q].dir;
        glVertex3f(agent.pos.x + (conf::BOTRADIUS * 4)*cos(aa),
                   agent.pos.y + (conf::BOTRADIUS * 4)*sin(aa),
                   0);
        //aa = agent.angle+agent.eyedir[q]-agent.eyefov[q];
        //glVertex3f(agent.pos.x,agent.pos.y,0);
        //glVertex3f(agent.pos.x+(conf::BOTRADIUS*4)*cos(aa),
        //           agent.pos.y+(conf::BOTRADIUS*4)*sin(aa),
        //           0);
    }
    glEnd();

    glBegin(GL_POLYGON); //body
    glColor3f(agent.color.red, agent.color.green, agent.color.blue);
    drawCircle(agent.pos.x, agent.pos.y, conf::BOTRADIUS);
    glEnd();

    glBegin(GL_LINES);
    //outline
    if (agent.boost)
        glColor3f(0.8, 0, 0); //draw boost as green outline
    else
        glColor3f(0, 0, 0);

    for (int k = 0; k < 17; k++)
    {
        n = k * (M_PI / 8);
        glVertex3f(agent.pos.x + r * sin(n), agent.pos.y + r * cos(n), 0);
        n = (k + 1) * (M_PI / 8);
        glVertex3f(agent.pos.x + r * sin(n), agent.pos.y + r * cos(n), 0);
    }
    //and spike
    glColor3f(0.5, 0, 0);
    glVertex3f(agent.pos.x, agent.pos.y, 0);
    glVertex3f(agent.pos.x + (3 * r * agent.spikeLength)*cos(agent.angle), agent.pos.y + (3 * r * agent.spikeLength)*sin(agent.angle), 0);
    glEnd();

    //and health
    int xo = 18;
    int yo = -15;
    glBegin(GL_QUADS);
    //black background
    glColor3f(0, 0, 0);
    glVertex3f(agent.pos.x + xo, agent.pos.y + yo, 0);
    glVertex3f(agent.pos.x + xo + 5, agent.pos.y + yo, 0);
    glVertex3f(agent.pos.x + xo + 5, agent.pos.y + yo + 40, 0);
    glVertex3f(agent.pos.x + xo, agent.pos.y + yo + 40, 0);

    //health
    glColor3f(0, 0.8, 0);
    glVertex3f(agent.pos.x + xo, agent.pos.y + yo + 20 * (2 - agent.health), 0);
    glVertex3f(agent.pos.x + xo + 5, agent.pos.y + yo + 20 * (2 - agent.health), 0);
    glVertex3f(agent.pos.x + xo + 5, agent.pos.y + yo + 40, 0);
    glVertex3f(agent.pos.x + xo, agent.pos.y + yo + 40, 0);

    //if this is a hybrid, we want to put a marker down
    if (agent.hybrid)
    {
        glColor3f(0, 0, 0.8);
        glVertex3f(agent.pos.x + xo + 6, agent.pos.y + yo, 0);
        glVertex3f(agent.pos.x + xo + 12, agent.pos.y + yo, 0);
        glVertex3f(agent.pos.x + xo + 12, agent.pos.y + yo + 10, 0);
        glVertex3f(agent.pos.x + xo + 6, agent.pos.y + yo + 10, 0);
    }

    glColor3f(1 - agent.herbivore, agent.herbivore, 0);
    glVertex3f(agent.pos.x + xo + 6, agent.pos.y + yo + 12, 0);
    glVertex3f(agent.pos.x + xo + 12, agent.pos.y + yo + 12, 0);
    glVertex3f(agent.pos.x + xo + 12, agent.pos.y + yo + 22, 0);
    glVertex3f(agent.pos.x + xo + 6, agent.pos.y + yo + 22, 0);

    //how much sound is this bot making?
    glColor3f(agent.soundmul, agent.soundmul, agent.soundmul);
    glVertex3f(agent.pos.x + xo + 6, agent.pos.y + yo + 24, 0);
    glVertex3f(agent.pos.x + xo + 12, agent.pos.y + yo + 24, 0);
    glVertex3f(agent.pos.x + xo + 12, agent.pos.y + yo + 34, 0);
    glVertex3f(agent.pos.x + xo + 6, agent.pos.y + yo + 34, 0);

    //draw giving/receiving
    if (agent.dfood != 0)
    {

        float mag = Clamp(abs(agent.dfood) / conf::FOODTRANSFER / 3, 0.0f, 1.0f);

        if (agent.dfood > 0)
            glColor3f(0, mag, 0); //draw boost as green outline
        else
            glColor3f(mag, 0, 0);
        glVertex3f(agent.pos.x + xo + 6, agent.pos.y + yo + 36, 0);
        glVertex3f(agent.pos.x + xo + 12, agent.pos.y + yo + 36, 0);
        glVertex3f(agent.pos.x + xo + 12, agent.pos.y + yo + 46, 0);
        glVertex3f(agent.pos.x + xo + 6, agent.pos.y + yo + 46, 0);
    }


    glEnd();

    //print stats
    //generation count
    char buf[10];
    sprintf(buf, "%i", agent.generation);
    RenderString(agent.pos.x - conf::BOTRADIUS * 1.5, agent.pos.y + conf::BOTRADIUS * 1.8, GLUT_BITMAP_TIMES_ROMAN_24, buf, 0.0f, 0.0f, 0.0f);
    //age
    sprintf(buf, "%i", agent.age);
    float x = agent.age / 1000.0;
    if (x > 1)x = 1;
    RenderString(agent.pos.x - conf::BOTRADIUS * 1.5, agent.pos.y + conf::BOTRADIUS * 1.8 + 12, GLUT_BITMAP_TIMES_ROMAN_24, buf, x, 0.0f, 0.0f);

    //health
    sprintf(buf, "%.2f", agent.health);
    RenderString(agent.pos.x - conf::BOTRADIUS * 1.5, agent.pos.y + conf::BOTRADIUS * 1.8 + 24, GLUT_BITMAP_TIMES_ROMAN_24, buf, 0.0f, 0.0f, 0.0f);

    //repcounter
    sprintf(buf, "%.2f", agent.repcounter);
    RenderString(agent.pos.x - conf::BOTRADIUS * 1.5, agent.pos.y + conf::BOTRADIUS * 1.8 + 36, GLUT_BITMAP_TIMES_ROMAN_24, buf, 0.0f, 0.0f, 0.0f);
}

void GLView::drawMisc()
{
    float mm = 3;
    //draw misc info
    glBegin(GL_LINES);
    glColor3f(0, 1, 0);
    for (size_t q = 0; q < world->numHerbivore.size() - 1; q++)
    {
        if (q == world->ptr - 1)
            continue;
        glVertex3f(q * 10, -20 - mm * world->numHerbivore[q], 0);
        glVertex3f((q + 1) * 10, -20 - mm * world->numHerbivore[q + 1], 0);
    }
    glColor3f(1, 0, 0);
    for (size_t q = 0; q < world->numHerbivore.size() - 1; q++)
    {
        if (q == world->ptr - 1)
            continue;
        glVertex3f(q * 10, -20 - mm * world->numCarnivore[q], 0);
        glVertex3f((q + 1) * 10, -20 - mm * world->numCarnivore[q + 1], 0);
    }
    glColor3f(0, 0, 0);
    glVertex3f(world->ptr * 10, -20, 0);
    glVertex3f(world->ptr * 10, -mm * 100, 0);
    glEnd();

    RenderString(2500, -80, GLUT_BITMAP_TIMES_ROMAN_24, "Press d for extra speed", 0.0f, 0.0f, 0.0f);
    RenderString(2500, -20, GLUT_BITMAP_TIMES_ROMAN_24, "Press s to follow selected agent, o to follow oldest", 0.0f, 0.0f, 0.0f);
}

void GLView::drawFood(int x, int y, float quantity)
{
    //draw food
    if (drawfood)
    {
        glBegin(GL_QUADS);
        glColor3f(0.9 - quantity, 0.9 - quantity, 1.0 - quantity);
        glVertex3f(x * conf::CZ, y * conf::CZ, 0);
        glVertex3f(x * conf::CZ + conf::CZ, y * conf::CZ, 0);
        glVertex3f(x * conf::CZ + conf::CZ, y * conf::CZ + conf::CZ, 0);
        glVertex3f(x * conf::CZ, y * conf::CZ + conf::CZ, 0);
        glEnd();
    }
}

void GLView::setWorld(World *w)
{
    world = w;
}
