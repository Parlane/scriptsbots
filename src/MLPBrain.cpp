#include "MLPBrain.h"
using namespace std;


MLPBox::MLPBox()
{

    synapse_weights.resize(CONNS, 0);
    id.resize(CONNS, 0);
    type.resize(CONNS, MLPSynapseType::REGULAR);

    //constructor
    for (int i = 0; i < CONNS; ++i)
    {
        synapse_weights[i] = randf(-3, 3);
        if (randf(0, 1) < 0.5)
            synapse_weights[i] = 0; // make brains sparse

        id[i] = randi(0, BRAINSIZE);
        if (randf(0, 1) < 0.2)
            id[i] = randi(0, INPUT_COUNT); //20% of the brain AT LEAST should connect to input.

        type[i] = MLPSynapseType::REGULAR;
        // make 5% be change sensitive synapses
        if (randf(0, 1) < 0.05)
            type[i] = MLPSynapseType::CHANGE_SENSITIVE;
    }

    kp = randf(0.9, 1.1);
    gw = randf(0, 5);
    bias = randf(-2, 2);

    out = 0;
    oldout = 0;
    target = 0;
}

MLPBrain::MLPBrain()
{

    //constructor
    for (int i = 0; i < BRAINSIZE; ++i)
    {
        MLPBox a; //make a random box and copy it over
        boxes.push_back(a);

        /*
        boxes[i].out= a.out;
        boxes[i].oldout = a.oldout;
        boxes[i].target= a.target;
        boxes[i].kp= a.kp;
        boxes[i].gw= a.gw;
        boxes[i].bias= a.bias;
        for (int j=0;j<CONNS;++j)
        {
            boxes[i].w[j]= a.w[j];
            boxes[i].id[j]= a.id[j];
            boxes[i].type[j] = a.type[j];
            if (i<BRAINSIZE/2)
            {
                boxes[i].id[j]= randi(0,INPUTSIZE);
            }
        }
        */
    }

    //do other initializations
    init();
}

MLPBrain::MLPBrain(const MLPBrain &other)
{
    boxes = other.boxes;
}

MLPBrain &MLPBrain::operator=(const MLPBrain &other)
{
    if (this != &other)
        boxes = other.boxes;
    return *this;
}


void MLPBrain::init()
{

}

void MLPBrain::tick(std::array<float, INPUT_COUNT>& in, std::array<float, OUTPUT_COUNT>& out)
{
    //do a single tick of the brain

    //take first few boxes and set their out to in[].
    for (int i = 0; i < INPUT_COUNT; ++i)
    {
        MLPBox& box = boxes[i];
        box.out = in[i];
    }

    //then do a dynamics tick and set all targets
    for (int i = INPUT_COUNT; i < BRAINSIZE; ++i)
    {
        MLPBox& box = boxes[i];

        float acc = 0;
        for (int j = 0; j < CONNS; ++j)
        {
            int idx = box.id[j];
            float val = boxes[idx].out;
            MLPSynapseType type = box.type[j];

            if (type == MLPSynapseType::CHANGE_SENSITIVE)
            {
                val -= boxes[idx].oldout;
                val *= 10.0f;
            }

            acc = acc + val * box.synapse_weights[j];
        }
        acc *= box.gw;
        acc += box.bias;

        //put through sigmoid
        acc = 1.0 / (1.0 + exp(-acc));

        box.target = acc;
    }

    //back up current out for each box
    for (int i = 0; i < BRAINSIZE; ++i)
    {
        MLPBox& box = boxes[i];
        box.oldout = box.out;
    }

    //make all boxes go a bit toward target
    for (int i = INPUT_COUNT; i < BRAINSIZE; ++i)
    {
        MLPBox& box = boxes[i];
        box.out = box.out + (box.target - box.out) * box.kp;
    }

    //finally set out[] to the last few boxes output
    for (int i = 0; i < OUTPUT_COUNT; ++i)
    {
        out[i] = boxes[BRAINSIZE - 1 - i].out;
    }
}

void MLPBrain::mutate(float MR, float MR2)
{
    for (int i = 0; i < BRAINSIZE; ++i)
    {
        MLPBox& box = boxes[i];
        if (randf(0, 1) < MR)
        {
            box.bias += randn(0, MR2);
//          a2.mutations.push_back("bias jiggled\n");
        }

        if (randf(0, 1) < MR)
        {
            box.kp += randn(0, MR2);
            Clamp(&box.kp, 0.01f, 1.0f);
//          a2.mutations.push_back("kp jiggled\n");
        }

        if (randf(0, 1) < MR)
        {
            box.gw += randn(0, MR2);
            FloorCap(&box.gw, 0.0f);
//          a2.mutations.push_back("kp jiggled\n");
        }

        if (randf(0, 1) < MR)
        {
            int rc = randi(0, CONNS);
            box.synapse_weights[rc] += randn(0, MR2);
//          a2.mutations.push_back("weight jiggled\n");
        }

        if (randf(0, 1) < MR)
        {
            //flip type of synapse
            int rc = randi(0, CONNS);
            box.type[rc] = (box.type[rc] == MLPSynapseType::REGULAR) ?
                           MLPSynapseType::CHANGE_SENSITIVE :
                           MLPSynapseType::REGULAR;
//          a2.mutations.push_back("weight jiggled\n");
        }

        //more unlikely changes here
        if (randf(0, 1) < MR)
        {
            int rc = randi(0, CONNS);
            int ri = randi(0, BRAINSIZE);
            box.id[rc] = ri;
//          a2.mutations.push_back("connectivity changed\n");
        }
    }
}

MLPBrain MLPBrain::crossover(const MLPBrain &other)
{
    //this could be made faster by returning a pointer
    //instead of returning by value
    MLPBrain newbrain(*this);
    for (size_t i = 0; i < newbrain.boxes.size(); ++i)
    {
        MLPBox& new_box = newbrain.boxes[i];
        const MLPBox& this_box = this->boxes[i];
        const MLPBox& other_box = other.boxes[i];

        if (randf(0, 1) < 0.5)
        {
            new_box.bias = this_box.bias;
            new_box.gw = this_box.gw;
            new_box.kp = this_box.kp;
            for (size_t j = 0; j < new_box.id.size(); ++j)
            {
                new_box.id[j] = this_box.id[j];
                new_box.synapse_weights[j] = this_box.synapse_weights[j];
                new_box.type[j] = this_box.type[j];
            }

        }
        else
        {
            new_box.bias = other_box.bias;
            new_box.gw = other_box.gw;
            new_box.kp = other_box.kp;
            for (size_t j = 0; j < new_box.id.size(); ++j)
            {
                new_box.id[j] = other_box.id[j];
                new_box.synapse_weights[j] = other_box.synapse_weights[j];
                new_box.type[j] = other_box.type[j];
            }
        }
    }
    return newbrain;
}

