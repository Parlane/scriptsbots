#include "Helpers.h"
#include "Settings.h"
#include "vmath.h"
#include "World.h"

#include <ctime>
#include <stdio.h>

using namespace std;

World::World() :
    modcounter(0),
    current_epoch(0),
    idcounter(0),
    food_size(conf::WIDTH / conf::CZ, conf::HEIGHT / conf::CZ),
    closed(false)
{
    reset();
}

void World::reset()
{
    agents.clear();
    addRandomBots(conf::NUMBOTS);

    for (int x = 0; x < food_size.x; x++)
    {
        for (int y = 0; y < food_size.y; y++)
        {
            food[x][y] = 0;
        }
    }

    for (int i = 0; i < conf::FOODSTART; ++i)
    {
        addRandomFood();
    }

    numCarnivore.clear();
    numHerbivore.clear();
    numCarnivore.resize(200, 0);
    numHerbivore.resize(200, 0);
    ptr = 0;
}

void World::update()
{
    modcounter++;

    //Process periodic events
    //Age goes up!
    if (modcounter % 100 == 0)
    {
        for (Agent & agent : agents)
        {
            agent.age += 1;
        }
    }

    if (modcounter % 1000 == 0)
    {
        std::pair<int, int> num_herbs_carns = numHerbCarnivores();
        numHerbivore[ptr] = num_herbs_carns.first;
        numCarnivore[ptr] = num_herbs_carns.second;
        ptr++;
        if (ptr == numHerbivore.size())
            ptr = 0;
    }

    if (modcounter % 1000 == 0)
        writeReport();

    if (modcounter >= 10000)
    {
        modcounter = 0;
        current_epoch++;
    }

    if (modcounter % conf::FOODADDFREQ == 0)
    {
        addRandomFood();
    }

    //reset any counter variables per agent
    for (Agent & agent : agents)
    {
        agent.spiked = false;
    }

    //give input to every agent. Sets in[InputType::] array
    setInputs();

    //brains tick. computes in[InputType::] -> out[]
    brainsTick();

    //read output and process consequences of bots on environment. requires out[]
    processOutputs();

    //process bots: health and deaths
    for (Agent & agent : agents)
    {
        float baseloss = 0.0002; // + 0.0001*(abs(agent.w1) + abs(agent.w2))/2;
        //if (agent.w1<0.1 && agent.w2<0.1) baseloss=0.0001; //hibernation :p
        //baseloss += 0.00005*agent.soundmul; //shouting costs energy. just a tiny bit

        if (agent.boost)
        {
            //boost carries its price, and it's pretty heavy!
            agent.health -= baseloss * conf::BOOSTSIZEMULT * 1.3;
        }
        else
        {
            agent.health -= baseloss;
        }
    }

    //process temperature preferences
    for (Agent & agent : agents)
    {
        //calculate temperature at the agents spot. (based on distance from equator)
        float dd = 2.0 * abs(agent.pos.x / conf::WIDTH - 0.5);
        float discomfort = abs(dd - agent.temperature_preference);
        discomfort = discomfort * discomfort;
        if (discomfort < 0.08)
            discomfort = 0;
        agent.health -= conf::TEMPERATURE_DISCOMFORT * discomfort;
    }

    //process indicator (used in drawing)
    for (Agent & agent : agents)
    {
        if (agent.indicator > 0)
            agent.indicator -= 1.0f;
    }

    //remove dead agents.
    //first distribute foods
    for (Agent & agent : agents)
    {
        //if this agent was spiked this round as well (i.e. killed). This will make it so that
        //natural deaths can't be capitalized on. I feel I must do this or otherwise agents
        //will sit on spot and wait for things to die around them. They must do work!
        if (agent.health <= 0 && agent.spiked)
        {

            //distribute its food. It will be erased soon
            //first figure out how many are around, to distribute this evenly
            int numaround = 0;
            for (const Agent & nearby_agent : agents)
            {
                if (nearby_agent.health > 0)
                {
                    float d = (agent.pos - nearby_agent.pos).length();
                    if (d < conf::FOOD_DISTRIBUTION_RADIUS)
                    {
                        ++numaround;
                    }
                }
            }

            //young killed agents should give very little resources
            //at age 5, they mature and give full. This can also help prevent
            //agents eating their young right away
            float agemult = 1.0f;
            if (agent.age < 5.0f)
                agemult = agent.age * 0.2f;

            if (numaround > 0)
            {
                //distribute its food evenly
                for (Agent & nearby_agent : agents)
                {
                    if (nearby_agent.health > 0)
                    {
                        float d = (agent.pos - nearby_agent.pos).length();
                        if (d < conf::FOOD_DISTRIBUTION_RADIUS)
                        {
                            nearby_agent.health += 5.0f * (1 - nearby_agent.herbivore) * (1 - nearby_agent.herbivore) / pow(numaround, 1.25f) * agemult;
                            CeilingCap(&nearby_agent.health, 2.0f); // cap it

                            // Good job, can use spare parts to make copies
                            nearby_agent.repcounter -= conf::REPMULT * (1 - nearby_agent.herbivore) * (1 - nearby_agent.herbivore) / pow(numaround, 1.25f) * agemult;
                            nearby_agent.initEvent(30, 1.0f, 1.0f, 1.0f); // White means they ate! nice
                        }
                    }
                }
            }

        }
    }
    auto iter = agents.begin();
    while (iter != agents.end())
    {
        if (iter->health <= 0)
        {
            iter = agents.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    //handle reproduction
    for (Agent & agent : agents)
    {
        if (agent.repcounter < 0 && agent.health > 0.65 && modcounter % 15 == 0 && randf(0, 1) < 0.1)
        {
            //agent is healthy and is ready to reproduce. Also inject a bit non-determinism
            //agent.health= 0.8; //the agent is left vulnerable and weak, a bit
            reproduce(agent, agent.MUTRATE1, agent.MUTRATE2); //this adds conf::BABIES new agents to agents[]
            agent.repcounter = agent.herbivore * randf(conf::REPRATEH - 0.1, conf::REPRATEH + 0.1) + (1 - agent.herbivore) * randf(conf::REPRATEC - 0.1, conf::REPRATEC + 0.1);
        }
    }

    //add new agents, if environment isn't closed
    if (!closed)
    {
        //make sure environment is always populated with at least NUMBOTS bots
        if (agents.size() < conf::NUMBOTS)
        {
            //add new agent
            addRandomBots(1);
        }
        if (modcounter % 100 == 0)
        {
            if (randf(0, 1) < 0.5)
            {
                addRandomBots(1); //every now and then add random bots in
            }
            else
            {
                addNewByCrossover(); //or by crossover
            }
        }
    }


}

void World::setInputs()
{
    for (Agent& a : agents)
    {

        //HEALTH
        a.in[InputType::HEALTH] = Clamp(a.health / 2, 0.0f, 1.0f); //divide by 2 since health is in [0,2]

        //FOOD
        int cx = (int) a.pos.x / conf::CZ;
        int cy = (int) a.pos.y / conf::CZ;
        a.in[InputType::FOOD] = food[cx][cy] / conf::FOODMAX;

        //SOUND SMELL EYES
        vector<float> p(NUMEYES, 0);
        vector<float> r(NUMEYES, 0);
        vector<float> g(NUMEYES, 0);
        vector<float> b(NUMEYES, 0);

        float soaccum = 0;
        float smaccum = 0;
        float hearaccum = 0;

        //BLOOD ESTIMATOR
        float blood = 0;

        for (const Agent& a2 : agents)
        {

            if (&a == &a2)
                continue;

            if (a.pos.x < a2.pos.x - conf::DIST ||
                a.pos.x > a2.pos.x + conf::DIST ||
                a.pos.y > a2.pos.y + conf::DIST ||
                a.pos.y < a2.pos.y - conf::DIST)
                continue;

            float d = (a.pos - a2.pos).length();

            if (d < conf::DIST)
            {

                //smell
                smaccum += (conf::DIST - d) / conf::DIST;

                //sound
                soaccum += (conf::DIST - d) / conf::DIST * (max(fabs(a2.wheel_left), fabs(a2.wheel_right)));

                //hearing. Listening to other agents
                hearaccum += a2.soundmul * (conf::DIST - d) / conf::DIST;

                float ang = (a2.pos - a.pos).get_angle(); //current angle between bots

                for (int q = 0; q < NUMEYES; q++)
                {
                    Eye& eye = a.eyes[q];

                    float aa = a.angle + eye.dir;
                    if (aa < -M_PI)
                        aa += 2 * M_PI;
                    if (aa > M_PI)
                        aa -= 2 * M_PI;

                    float diff1 = aa - ang;
                    if (fabs(diff1) > M_PI)
                        diff1 = 2 * M_PI - fabs(diff1);

                    diff1 = fabs(diff1);

                    float fov = eye.fov;
                    if (diff1 < fov)
                    {
                        //we see a2 with this eye. Accumulate stats
                        float mul1 = a.eyesensmod * (fabs(fov - diff1) / fov) * ((conf::DIST - d) / conf::DIST);
                        p[q] += mul1 * (d / conf::DIST);
                        r[q] += mul1 * a2.color.red;
                        g[q] += mul1 * a2.color.green;
                        b[q] += mul1 * a2.color.blue;
                    }
                }

                //blood sensor
                float forwangle = a.angle;
                float diff4 = forwangle - ang;

                if (fabs(forwangle) > M_PI)
                    diff4 = 2 * M_PI - fabs(forwangle);

                diff4 = fabs(diff4);
                if (diff4 < PI38)
                {
                    float mul4 = ((PI38 - diff4) / PI38) * ((conf::DIST - d) / conf::DIST);
                    //if we can see an agent close with both eyes in front of us
                    blood += mul4 * (1 - a2.health / 2); //remember: health is in [0 2]
                    //agents with high life dont bleed. low life makes them bleed more
                }
            }
        }

        smaccum *= a.smellmod;
        soaccum *= a.soundmod;
        hearaccum *= a.hearmod;
        blood *= a.bloodmod;

        a.in[InputType::P0] = Clamp(p[0], 0.0f, 1.0f);
        a.in[InputType::R0] = Clamp(r[0], 0.0f, 1.0f);
        a.in[InputType::G0] = Clamp(g[0], 0.0f, 1.0f);
        a.in[InputType::B0] = Clamp(b[0], 0.0f, 1.0f);

        a.in[InputType::P1] = Clamp(p[1], 0.0f, 1.0f);
        a.in[InputType::R1] = Clamp(r[1], 0.0f, 1.0f);
        a.in[InputType::G1] = Clamp(g[1], 0.0f, 1.0f);
        a.in[InputType::B1] = Clamp(b[1], 0.0f, 1.0f);
        a.in[InputType::SOUND] = Clamp(soaccum, 0.0f, 1.0f);
        a.in[InputType::SMELL] = Clamp(smaccum, 0.0f, 1.0f);

        a.in[InputType::P2] = Clamp(p[2], 0.0f, 1.0f);
        a.in[InputType::R2] = Clamp(r[2], 0.0f, 1.0f);
        a.in[InputType::G2] = Clamp(g[2], 0.0f, 1.0f);
        a.in[InputType::B2] = Clamp(b[2], 0.0f, 1.0f);
        a.in[InputType::CLOCK1] = abs(sin(modcounter / a.clockf1));
        a.in[InputType::CLOCK2] = abs(sin(modcounter / a.clockf2));
        a.in[InputType::HEARING] = Clamp(hearaccum, 0.0f, 1.0f);
        a.in[InputType::BLOOD_SENSOR] = Clamp(blood, 0.0f, 1.0f);

        //temperature varies from 0 to 1 across screen.
        //it is 0 at equator (in middle), and 1 on edges. Agents can sense discomfort
        float dd = 2.0 * abs(a.pos.x / conf::WIDTH - 0.5);
        float discomfort = abs(dd - a.temperature_preference);
        a.in[InputType::TEMP_SENSOR] = discomfort;

        a.in[InputType::P3] = Clamp(p[3], 0.0f, 1.0f);
        a.in[InputType::R3] = Clamp(r[3], 0.0f, 1.0f);
        a.in[InputType::G3] = Clamp(g[3], 0.0f, 1.0f);
        a.in[InputType::B3] = Clamp(b[3], 0.0f, 1.0f);

    }
}

void World::processOutputs()
{
    //assign meaning
    for (Agent & agent : agents)
    {
        agent.color.red = agent.out[OutputType::RED];
        agent.color.green = agent.out[OutputType::GREEN];
        agent.color.blue = agent.out[OutputType::BLUE];
        agent.wheel_left = agent.out[OutputType::WHEEL_LEFT]; //-(2*agent.out[0]-1);
        agent.wheel_right = agent.out[OutputType::WHEEL_RIGHT]; //-(2*agent.out[1]-1);
        agent.boost = agent.out[OutputType::BOOST] > 0.5;
        agent.soundmul = agent.out[OutputType::SOUND_MULTIPLIER];
        agent.give = agent.out[OutputType::GIVING];

        //spike length should slowly tend towards out[OutputType::SPIKE]
        float g = agent.out[OutputType::SPIKE];
        if (agent.spikeLength < g)
            agent.spikeLength += conf::SPIKESPEED;
        else if (agent.spikeLength > g)
            agent.spikeLength = g; //its easy to retract spike, just hard to put it up
    }

    //move bots
    //#pragma omp parallel for
    for (size_t i = 0; i < agents.size(); ++i)
    {
        Agent &agent = agents[i];

        Vector2f v(conf::BOTRADIUS / 2, 0);
        v.rotate(agent.angle + M_PI / 2);

        Vector2f w1p = agent.pos + v; //wheel positions
        Vector2f w2p = agent.pos - v;

        float BW1 = conf::BOTSPEED * agent.wheel_left;
        float BW2 = conf::BOTSPEED * agent.wheel_right;

        if (agent.boost)
        {
            BW1 = BW1 * conf::BOOSTSIZEMULT;
            BW2 = BW2 * conf::BOOSTSIZEMULT;
        }

        //move bots
        Vector2f vv = w2p - agent.pos;
        vv.rotate(-BW1);
        agent.pos = w2p - vv;
        agent.angle -= BW1;
        if (agent.angle < -M_PI)
            agent.angle = M_PI - (-M_PI - agent.angle);

        vv = agent.pos - w1p;
        vv.rotate(BW2);
        agent.pos = w1p + vv;
        agent.angle += BW2;
        if (agent.angle > M_PI)
            agent.angle = -M_PI + (agent.angle - M_PI);

        //wrap around the map
        if (agent.pos.x < 0)
            agent.pos.x = conf::WIDTH + agent.pos.x;
        if (agent.pos.x >= conf::WIDTH)
            agent.pos.x = agent.pos.x - conf::WIDTH;
        if (agent.pos.y < 0)
            agent.pos.y = conf::HEIGHT + agent.pos.y;
        if (agent.pos.y >= conf::HEIGHT)
            agent.pos.y = agent.pos.y - conf::HEIGHT;
    }

    // process food intake for herbivores
    for (size_t i = 0; i < agents.size(); ++i)
    {

        int cx = (int) agents[i].pos.x / conf::CZ;
        int cy = (int) agents[i].pos.y / conf::CZ;
        float f = food[cx][cy];
        if (f > 0 && agents[i].health < 2)
        {
            // agent eats the food
            float itk = min(f, conf::FOODINTAKE);
            float speedmul = (1 - (abs(agents[i].wheel_left) + abs(agents[i].wheel_right)) / 2) * 0.7 + 0.3;
            itk = itk * agents[i].herbivore * speedmul; //herbivores gain more from ground food
            agents[i].health += itk;
            agents[i].repcounter -= 3 * itk;
            food[cx][cy] -= min(f, conf::FOODWASTE);
        }
    }

    //process giving and receiving of food
    for (size_t i = 0; i < agents.size(); ++i)
    {
        agents[i].dfood = 0;
    }

    for (size_t i = 0; i < agents.size(); ++i)
    {
        if (agents[i].give > 0.5)
        {
            for (size_t j = 0; j < agents.size(); ++j)
            {
                float d = (agents[i].pos - agents[j].pos).length();
                if (d < conf::FOOD_SHARING_DISTANCE)
                {
                    //initiate transfer
                    if (agents[j].health < 2)
                        agents[j].health += conf::FOODTRANSFER;
                    agents[i].health -= conf::FOODTRANSFER;
                    agents[j].dfood += conf::FOODTRANSFER; //only for drawing
                    agents[i].dfood -= conf::FOODTRANSFER;
                }
            }
        }
    }

    // Process spike dynamics for carnivores
    // we dont need to do this TOO often. can save efficiency here since this is n^2 op in #agents
    if (modcounter % 2 == 0)
    {
        for (size_t i = 0; i < agents.size(); ++i)
        {

            // NOTE: herbivore cant attack. TODO: hmmmmm
            // for now ok: I want herbivores to run away from carnivores, not kill them back
            if (agents[i].herbivore > 0.8 || agents[i].spikeLength < 0.2 || agents[i].wheel_left < 0.5 || agents[i].wheel_right < 0.5)
                continue;

            for (size_t j = 0; j < agents.size(); ++j)
            {

                if (i == j) continue;
                float d = (agents[i].pos - agents[j].pos).length();

                if (d < 2 * conf::BOTRADIUS)
                {
                    //these two are in collision and agent i has extended spike and is going decent fast!
                    Vector2f v(1, 0);
                    v.rotate(agents[i].angle);
                    float diff = v.angle_between(agents[j].pos - agents[i].pos);
                    if (fabs(diff) < M_PI / 8)
                    {
                        //bot i is also properly aligned!!! that's a hit
                        /*float mult = 1;
                        if (agents[i].boost)
                            mult = conf::BOOSTSIZEMULT;*/
                        float DMG = conf::SPIKEMULT * agents[i].spikeLength * max(fabs(agents[i].wheel_left), fabs(agents[i].wheel_right)) * conf::BOOSTSIZEMULT;

                        agents[j].health -= DMG;
                        CeilingCap(&agents[i].health, 2.0f); // cap health at 2

                        agents[i].spikeLength = 0; //retract spike back down

                        agents[i].initEvent(40 * DMG, 1, 1, 0); //yellow event means bot has spiked other bot. nice!

                        Vector2f v2(1, 0);
                        v2.rotate(agents[j].angle);
                        float adiff = v.angle_between(v2);
                        if (fabs(adiff) < M_PI / 2)
                        {
                            //this was attack from the back. Retract spike of the other agent (startle!)
                            //this is done so that the other agent cant right away "by accident" attack this agent
                            agents[j].spikeLength = 0;
                        }

                        agents[j].spiked = true; //set a flag saying that this agent was hit this turn
                    }
                }
            }
        }
    }
}

void World::brainsTick()
{
    #pragma omp parallel for
    for (size_t i = 0; i < agents.size(); ++i)
    {
        Agent& agent = agents[i];
        agent.tick();
    }
}

void World::addRandomBots(int num)
{
    for (int i = 0; i < num; ++i)
    {
        Agent a;
        a.id = idcounter;
        ++idcounter;
        agents.push_back(a);
    }
}

void World::positionOfInterest(int type, Vector2f *pos)
{
    if (type == 1)
    {
        //the interest of type 1 is the oldest agent
        int maxage = -1;
        const Agent *oldest_agent = nullptr;
        for (const Agent & agent : agents)
        {
            if (agent.age > maxage)
            {
                maxage = agent.age;
                oldest_agent = &agent;
            }
        }
        if (oldest_agent)
        {
            pos->x = oldest_agent->pos.x;
            pos->y = oldest_agent->pos.y;
        }
    }
    else if (type == 2)
    {
        //interest of type 2 is the selected agent
        const Agent *selected_agent = nullptr;
        for (const Agent & agent : agents)
        {
            if (agent.selectflag)
            {
                selected_agent = &agent;
                break;
            }
        }
        if (selected_agent)
        {
            pos->x = selected_agent->pos.x;
            pos->y = selected_agent->pos.y;
        }
    }

}

void World::addRandomFood()
{
    int fx = randi(0, food_size.x);
    int fy = randi(0, food_size.y);
    food[fx][fy] = conf::FOODMAX;
}
void World::addCarnivore()
{
    Agent a;
    a.id = idcounter;
    idcounter++;
    a.herbivore = randf(0, 0.1);
    agents.push_back(a);
}

void World::addHerbivore()
{
    Agent a;
    a.id = idcounter;
    idcounter++;
    a.herbivore = randf(0.9, 1);
    agents.push_back(a);
}


void World::addNewByCrossover()
{

    //find two success cases
    size_t i1 = randi(0, agents.size());
    size_t i2 = randi(0, agents.size());
    for (size_t i = 0; i < agents.size(); ++i)
    {
        if (agents[i].age > agents[i1].age && randf(0, 1) < 0.1)
        {
            i1 = i;
        }
        if (agents[i].age > agents[i2].age && randf(0, 1) < 0.1 && i != i1)
        {
            i2 = i;
        }
    }

    Agent *a1 = &agents[i1];
    Agent *a2 = &agents[i2];


    //cross brains
    Agent anew = a1->crossover(*a2);


    //maybe do mutation here? I dont know. So far its only crossover
    anew.id = idcounter;
    ++idcounter;
    agents.push_back(anew);
}

void World::reproduce(Agent &ai, float MR, float MR2)
{
    if (randf(0, 1) < 0.04) MR = MR * randf(1, 10);
    if (randf(0, 1) < 0.04) MR2 = MR2 * randf(1, 10);

    ai.initEvent(30, 0, 0.8, 0); //green event means agent reproduced.
    for (int i = 0; i < conf::BABIES; ++i)
    {

        Agent a2 = ai.reproduce(MR, MR2);
        a2.id = idcounter;
        idcounter++;
        agents.push_back(a2);

        //TODO fix recording
        //record this
        //FILE* fp = fopen("log.txt", "a");
        //fprintf(fp, "%i %i %i\n", 1, this->id, a2.id); //1 marks the event: child is born
        //fclose(fp);
    }
}

void World::writeReport()
{
    //TODO fix reporting
    //save all kinds of nice data stuff
//     int numherb=0;
//     int numcarn=0;
//     int topcarn=0;
//     int topherb=0;
//     for(int i=0;i<agents.size();++i)
//     {
//         if (agents[i].herbivore>0.5)
//             numherb++;
//         else
//             numcarn++;
//
//         if (agents[i].herbivore>0.5 && agents[i].gencount>topherb) topherb= agents[i].gencount;
//         if (agents[i].herbivore<0.5 && agents[i].gencount>topcarn) topcarn= agents[i].gencount;
//     }
//
//     FILE* fp = fopen("report.txt", "a");
//     fprintf(fp, "%i %i %i %i\n", numherb, numcarn, topcarn, topherb);
//     fclose(fp);
}


void World::setClosed(bool close)
{
    closed = close;
}

bool World::isClosed() const
{
    return closed;
}


void World::processMouse(int button, int state, int x, int y)
{
    if (state == 0)
    {
        float mind = 1e10;
        Agent *closest_agent = nullptr;

        for (Agent & agent : agents)
        {
            float d = pow(x - agent.pos.x, 2) + pow(y - agent.pos.y, 2);
            if (d < mind)
            {
                mind = d;
                closest_agent = &agent;
            }
            agent.selectflag = false;
        }

        if (closest_agent)
        {
            //toggle selection of this agent
            closest_agent->selectflag = true;
            closest_agent->printSelf();
        }
    }
}

void World::draw(View *view, bool drawfood)
{
    //draw food
    if (drawfood)
    {
        for (int i = 0; i < food_size.x; ++i)
        {
            for (int j = 0; j < food_size.y; ++j)
            {
                float f = 0.5 * food[i][j] / conf::FOODMAX;
                view->drawFood(i, j, f);
            }
        }
    }

    //draw all agents
    for (auto it = agents.begin(); it != agents.end(); ++it)
    {
        view->drawAgent(*it);
    }

    view->drawMisc();
}

std::pair< int, int > World::numHerbCarnivores() const
{
    int numherb = 0;
    int numcarn = 0;
    for (const Agent & agent : agents)
    {
        if (agent.herbivore > 0.5)
            numherb++;
        else
            numcarn++;
    }

    return std::pair<int, int>(numherb, numcarn);
}

size_t World::numAgents() const
{
    return agents.size();
}

int World::epoch() const
{
    return current_epoch;
}


