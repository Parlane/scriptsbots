#pragma once

#include "Helpers.h"
#include "Settings.h"

#include <array>
#include <stdio.h>
#include <vector>

enum class MLPSynapseType
{
    REGULAR = 0,
    CHANGE_SENSITIVE = 1,
};

class MLPBox
{
public:

    MLPBox();

    std::vector<float> synapse_weights; //weight of each connecting box
    std::vector<int> id; //id in boxes[] of the connecting box
    std::vector<MLPSynapseType> type; //0: regular synapse. 1: change-sensitive synapse
    float kp; //damper
    float gw; //global w
    float bias;

    //state variables
    float target; //target value this node is going toward
    float out; //current output
    float oldout; //output a tick ago
};

/**
 * Damped Weighted Recurrent AND/OR Network
 */
class MLPBrain
{
public:

    std::vector<MLPBox> boxes;

    MLPBrain();
    MLPBrain(const MLPBrain &other);
    virtual MLPBrain &operator=(const MLPBrain &other);

    void tick(std::array<float, INPUT_COUNT>& in, std::array<float, OUTPUT_COUNT>& out);
    void mutate(float MR, float MR2);
    MLPBrain crossover(const MLPBrain &other);
private:
    void init();
};
