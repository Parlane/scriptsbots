#pragma once

#include "Agent.h"
#include "Settings.h"
#include "View.h"

#include <vector>

class World
{
public:
    World();
    ~World();

    void update();
    void reset();

    void draw(View *view, bool drawfood);

    bool isClosed() const;
    void setClosed(bool close);

    /**
     * Returns the number of herbivores and
     * carnivores in the world.
     * first : num herbs
     * second : num carns
     */
    std::pair<int, int> numHerbCarnivores() const;

    size_t numAgents() const;
    int epoch() const;

    //mouse interaction
    void processMouse(int button, int state, int x, int y);

    void addNewByCrossover();
    void addRandomBots(int num);
    void addCarnivore();
    void addHerbivore();

    void addRandomFood();

    void positionOfInterest(int type, Vector2f *pos);

    std::vector<int> numCarnivore;
    std::vector<int> numHerbivore;
    size_t ptr;

private:
    void setInputs();
    void processOutputs();
    void brainsTick();  //takes in[] to out[] for every agent

    void writeReport();

    void reproduce(Agent &ai, float MR, float MR2);

    int modcounter;
    int current_epoch;
    int idcounter;

    std::vector<Agent> agents;

    // food
    Vector2<int> food_size;
    float food[conf::WIDTH / conf::CZ][conf::HEIGHT / conf::CZ];

    bool closed; //if environment is closed, then no random bots are added per time interval
};
