#pragma once

const int NUMEYES = 4;
const int BRAINSIZE = 200;
const int CONNS = 4;


enum InputType
{
    //P1 R1 G1 B1 FOOD P2 R2 G2 B2 SOUND SMELL HEALTH P3 R3 G3 B3 CLOCK1 CLOCK 2 HEARING     BLOOD_SENSOR   TEMPERATURE_SENSOR
    //0   1  2  3  4   5   6  7 8   9     10     11   12 13 14 15 16       17      18           19                 20

    P0 = 0,
    R0,
    G0,
    B0,
    FOOD,
    P1,
    R1,
    G1,
    B1,
    SOUND,
    SMELL,
    HEALTH,
    P2,
    R2,
    G2,
    B2,
    CLOCK1,
    CLOCK2,
    HEARING,
    BLOOD_SENSOR,
    TEMP_SENSOR,
    P3,
    R3,
    G3,
    B3,

    // Do not add after this.
    INPUT_COUNT
};


enum OutputType
{
    //LEFT RIGHT R G B SPIKE BOOST SOUND_MULTIPLIER GIVING
    // 0    1    2 3 4   5     6         7             8

    WHEEL_LEFT = 0,
    WHEEL_RIGHT,
    RED,
    GREEN,
    BLUE,
    SPIKE,
    BOOST,
    SOUND_MULTIPLIER,
    GIVING,

    // Do not add after this.
    OUTPUT_COUNT
};

namespace conf
{

const int   WIDTH = 6000;  //width and height of simulation
const int   HEIGHT = 3000;
const int   WWIDTH = 1600;  //window width and height
const int   WHEIGHT = 900;

const int   CZ = 50; //cell size in pixels, for food squares. Should divide well into Width Height

const int   NUMBOTS = 10; //initially, and minimally
const float BOTRADIUS = 10.0f; //for drawing
const float BOTSPEED = 0.3f;
const float SPIKESPEED = 0.005f; //how quickly can attack spike go up?
const float SPIKEMULT = 1.0f; //essentially the strength of every spike impact
const int   BABIES = 2; //number of babies per agent when they reproduce
const float BOOSTSIZEMULT = 2.0f; //how much boost do agents get? when boost neuron is on
const float REPRATEH = 7.0f; //reproduction rate for herbivores
const float REPRATEC = 7.0f; //reproduction rate for carnivores

const float DIST = 150.0f; //how far can the eyes see on each bot?
const float METAMUTRATE1 = 0.002f; //what is the change in MUTRATE1 and 2 on reproduction? lol
const float METAMUTRATE2 = 0.05f;

const float FOODINTAKE = 0.002f; //how much does every agent consume?
const float FOODWASTE = 0.001f; //how much food disapears if agent eats?
const float FOODMAX = 0.5f; //how much food per cell can there be at max?
const int   FOODADDFREQ = 1500; //how often does random square get to full food?
const int   FOODSTART = 10;    //how much food is added at world creation

const float FOODTRANSFER = 0.001f; //how much is transfered between two agents trading food? per iteration
const float FOOD_SHARING_DISTANCE = 500.0f; //how far away is food shared between bots?

const float TEMPERATURE_DISCOMFORT = 0.0f; //how quickly does health drain in nonpreferred temperatures (0= disabled. 0.005 is decent value)

const float FOOD_DISTRIBUTION_RADIUS = 100.0f; //when bot is killed, how far is its body distributed?

const float REPMULT = 5.0f; //when a body of dead animal is distributed, how much of it goes toward increasing birth counter for surrounding bots?
}
