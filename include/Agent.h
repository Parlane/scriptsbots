#pragma once

#include "AssemblyBrain.h"
#include "Common.h"
#include "DWRAONBrain.h"
#include "MLPBrain.h"
#include "vmath.h"

#include <array>
#include <string>
#include <vector>

class Color
{
public:
    float red;
    float green;
    float blue;
    Color() : red(0.0f), green(0.0f), blue(0.0f) {}
};

class Eye
{
public:
    float fov; // field of view for each eye
    float dir; //direction of each eye
    Eye() : fov(0.0f), dir(0.0f) {}
};

class Agent
{
public:
    Agent();


    void printSelf();
    //for drawing purposes
    void initEvent(float size, float r, float g, float b);

    void tick();
    Agent reproduce(float MR, float MR2);
    Agent crossover(const Agent &other);

    Vector2f pos;

    float health; //in [0,2]. I cant remember why.
    float angle; //of the bot

    Color color;

    // wheel speeds
    float wheel_left;
    float wheel_right;

    bool boost; //is this agent boosting

    float spikeLength;
    int age;

    bool spiked;

    std::array<float, INPUT_COUNT> in; //input: 2 eyes, sensors for R,G,B,proximity each, then Sound, Smell, Health
    std::array<float, OUTPUT_COUNT> out; //output: Left, Right, R, G, B, SPIKE

    float repcounter; //when repcounter gets to 0, this bot reproduces
    int generation; //generation counter
    bool hybrid; //is this agent result of crossover?
    float clockf1, clockf2; //the frequencies of the two clocks of this bot
    float soundmul; //sound multiplier of this bot. It can scream, or be very sneaky. This is actually always set to output 8

    // variables for drawing purposes
    float indicator;
    float ir;
    float ig;
    float ib; // indicator colors
    int selectflag; // is this agent selected?
    float dfood; // what is change in health of this agent due to giving/receiving?

    float give;    //is this agent attempting to give food to other agent?

    int id;

    //inhereted stuff
    float herbivore; //is this agent a herbivore? between 0 and 1
    float MUTRATE1; //how often do mutations occur?
    float MUTRATE2; //how significant are they?
    float temperature_preference; //what temperature does this agent like? [0 to 1]

    float smellmod;
    float soundmod;
    float hearmod;
    float eyesensmod;
    float bloodmod;

    std::vector<Eye> eyes;

//    DWRAONBrain brain; //THE BRAIN!!!!
//    AssemblyBrain brain;
    MLPBrain brain;

    //will store the mutations that this agent has from its parent
    //can be used to tune the mutation rate
    std::vector<std::string> mutations;
};
