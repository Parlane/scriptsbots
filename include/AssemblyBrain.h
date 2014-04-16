#pragma once

#include "Helpers.h"
#include "Settings.h"

#include <vector>

/**
 * Assembly Brain
 */
class AssemblyBrain
{
public:

    std::vector<float> w;

    AssemblyBrain();
    AssemblyBrain(const AssemblyBrain &other);
    virtual AssemblyBrain &operator=(const AssemblyBrain &other);

    void tick(std::vector<float> &in, std::vector<float> &out);
    void mutate(float MR, float MR2);
    AssemblyBrain crossover(const AssemblyBrain &other);

private:
    void init();
};
