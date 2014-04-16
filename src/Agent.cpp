#include <stdio.h>
#include <iostream>
#include <string>

#include "Agent.h"
#include "AssemblyBrain.h"
#include "DWRAONBrain.h"
#include "Helpers.h"
#include "MLPBrain.h"
#include "Settings.h"

using namespace std;
Agent::Agent()
{
    pos = Vector2f(randf(0, conf::WIDTH), randf(0, conf::HEIGHT));
    angle = randf(-M_PI, M_PI);
    health = 1.0 + randf(0, 0.1);
    dfood = 0;
    age = 0;
    spikeLength = 0;
    wheel_left = 0;
    wheel_right = 0;
    soundmul = 1;
    give = 0;
    clockf1 = randf(5, 100);
    clockf2 = randf(5, 100);
    boost = false;
    indicator = 0;
    generation = 0;
    selectflag = 0;
    ir = 0;
    ig = 0;
    ib = 0;
    temperature_preference = randf(0, 1);
    hybrid = false;
    herbivore = randf(0, 1);
    repcounter = herbivore * randf(conf::REPRATEH - 0.1, conf::REPRATEH + 0.1) + (1 - herbivore) * randf(conf::REPRATEC - 0.1, conf::REPRATEC + 0.1);

    id = 0;

    smellmod = randf(0.1, 0.5);
    soundmod = randf(0.2, 0.6);
    hearmod = randf(0.7, 1.3);
    eyesensmod = randf(1.0, 3.0);
    bloodmod = randf(1.0, 3.0);

    MUTRATE1 = randf(0.001, 0.005);
    MUTRATE2 = randf(0.03, 0.07);

    spiked = false;

    eyes.resize(NUMEYES);

    for (int i = 0; i < NUMEYES; ++i)
    {
        eyes[i].fov = randf(0.5, 2);
        eyes[i].dir = randf(0, 2 * M_PI);
    }
}

void Agent::printSelf()
{
    printf("Agent age=%i\n", age);
    for (const string & mutation : mutations)
    {
        cout << mutation;
    }
}

void Agent::initEvent(float size, float r, float g, float b)
{
    indicator = size;
    ir = r;
    ig = g;
    ib = b;
}

void Agent::tick()
{
    brain.tick(in, out);
}
Agent Agent::reproduce(float MR, float MR2)
{
    bool BDEBUG = false;
    if (BDEBUG)
        printf("New birth---------------\n");
    Agent a2;

    //spawn the baby somewhere closeby behind agent
    //we want to spawn behind so that agents dont accidentally eat their young right away
    Vector2f fb(conf::BOTRADIUS, 0);
    fb.rotate(-a2.angle);
    a2.pos = this->pos + fb + Vector2f(randf(-conf::BOTRADIUS * 2, conf::BOTRADIUS * 2), randf(-conf::BOTRADIUS * 2, conf::BOTRADIUS * 2));
    if (a2.pos.x < 0)
        a2.pos.x = conf::WIDTH + a2.pos.x;
    if (a2.pos.x >= conf::WIDTH)
        a2.pos.x = a2.pos.x - conf::WIDTH;
    if (a2.pos.y < 0)
        a2.pos.y = conf::HEIGHT + a2.pos.y;
    if (a2.pos.y >= conf::HEIGHT)
        a2.pos.y = a2.pos.y - conf::HEIGHT;

    a2.generation = this->generation + 1;
    a2.repcounter = a2.herbivore * randf(conf::REPRATEH - 0.1, conf::REPRATEH + 0.1) + (1 - a2.herbivore) * randf(conf::REPRATEC - 0.1, conf::REPRATEC + 0.1);

    //noisy attribute passing
    a2.MUTRATE1 = this->MUTRATE1;
    a2.MUTRATE2 = this->MUTRATE2;
    if (randf(0, 1) < 0.1)
        a2.MUTRATE1 = randn(this->MUTRATE1, conf::METAMUTRATE1);
    if (randf(0, 1) < 0.1)
        a2.MUTRATE2 = randn(this->MUTRATE2, conf::METAMUTRATE2);
    FloorCap(&a2.MUTRATE1, 0.001f);
    FloorCap(&a2.MUTRATE2, 0.02f);

    a2.herbivore = Clamp(randn(this->herbivore, 0.03), 0.0, 1.0);

    if (randf(0, 1) < MR * 5)
        a2.clockf1 = randn(a2.clockf1, MR2);
    FloorCap(&a2.clockf1, 2.0f);

    if (randf(0, 1) < MR * 5)
        a2.clockf2 = randn(a2.clockf2, MR2);
    FloorCap(&a2.clockf2, 2.0f);

    a2.smellmod = this->smellmod;
    a2.soundmod = this->soundmod;
    a2.hearmod = this->hearmod;
    a2.eyesensmod = this->eyesensmod;
    a2.bloodmod = this->bloodmod;
    if (randf(0, 1) < MR * 5)
    {
        float oo = a2.smellmod;
        a2.smellmod = randn(a2.smellmod, MR2);
        if (BDEBUG) printf("smell mutated from %f to %f\n", oo, a2.smellmod);
    }
    if (randf(0, 1) < MR * 5)
    {
        float oo = a2.soundmod;
        a2.soundmod = randn(a2.soundmod, MR2);
        if (BDEBUG) printf("sound mutated from %f to %f\n", oo, a2.soundmod);
    }
    if (randf(0, 1) < MR * 5)
    {
        float oo = a2.hearmod;
        a2.hearmod = randn(a2.hearmod, MR2);
        if (BDEBUG) printf("hear mutated from %f to %f\n", oo, a2.hearmod);
    }
    if (randf(0, 1) < MR * 5)
    {
        float oo = a2.eyesensmod;
        a2.eyesensmod = randn(a2.eyesensmod, MR2);
        if (BDEBUG) printf("eyesens mutated from %f to %f\n", oo, a2.eyesensmod);
    }
    if (randf(0, 1) < MR * 5)
    {
        float oo = a2.bloodmod;
        a2.bloodmod = randn(a2.bloodmod, MR2);
        if (BDEBUG) printf("blood mutated from %f to %f\n", oo, a2.bloodmod);
    }

    a2.eyes = this->eyes;
    for (int i = 0; i < NUMEYES; ++i)
    {
        if (randf(0, 1) < MR * 5)
            a2.eyes[i].fov = randn(a2.eyes[i].fov, MR2);
        if (a2.eyes[i].fov < 0)
            a2.eyes[i].fov = 0;

        if (randf(0, 1) < MR * 5)
            a2.eyes[i].dir = randn(a2.eyes[i].dir, MR2);

        Clamp(&a2.eyes[i].dir, 0.0f, TAU);
    }

    a2.temperature_preference = Clamp(randn(this->temperature_preference, 0.005), 0.0, 1.0);
//    a2.temperature_preference= this->temperature_preference;

    //mutate brain here
    a2.brain = this->brain;
    a2.brain.mutate(MR, MR2);

    return a2;

}

Agent Agent::crossover(const Agent &other)
{
    //this could be made faster by returning a pointer
    //instead of returning by value
    Agent anew;
    anew.hybrid = true; //set this non-default flag
    anew.generation = this->generation;
    if (other.generation < anew.generation)
        anew.generation = other.generation;

    //agent heredity attributes
    anew.clockf1 = randf(0, 1) < 0.5 ? this->clockf1 : other.clockf1;
    anew.clockf2 = randf(0, 1) < 0.5 ? this->clockf2 : other.clockf2;
    anew.herbivore = randf(0, 1) < 0.5 ? this->herbivore : other.herbivore;
    anew.MUTRATE1 = randf(0, 1) < 0.5 ? this->MUTRATE1 : other.MUTRATE1;
    anew.MUTRATE2 = randf(0, 1) < 0.5 ? this->MUTRATE2 : other.MUTRATE2;
    anew.temperature_preference = randf(0, 1) < 0.5 ? this->temperature_preference : other.temperature_preference;

    anew.smellmod = randf(0, 1) < 0.5 ? this->smellmod : other.smellmod;
    anew.soundmod = randf(0, 1) < 0.5 ? this->soundmod : other.soundmod;
    anew.hearmod = randf(0, 1) < 0.5 ? this->hearmod : other.hearmod;
    anew.eyesensmod = randf(0, 1) < 0.5 ? this->eyesensmod : other.eyesensmod;
    anew.bloodmod = randf(0, 1) < 0.5 ? this->bloodmod : other.bloodmod;

    for (int i = 0; i < NUMEYES; ++i)
    {
        anew.eyes[i].fov = randf(0, 1) < 0.5 ? this->eyes[i].fov : other.eyes[i].fov;
        anew.eyes[i].dir = randf(0, 1) < 0.5 ? this->eyes[i].dir : other.eyes[i].dir;
    }

    anew.brain = this->brain.crossover(other.brain);

    return anew;
}
