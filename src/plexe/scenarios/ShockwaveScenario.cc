//
// Copyright (c) 2012-2021 Michele Segata <segata@ccs-labs.org>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "plexe/scenarios/ShockwaveScenario.h"

using namespace veins;

namespace plexe {

Define_Module(ShockwaveScenario);

int ShockwaveScenario::shockwaveCarsCount = 0;

void ShockwaveScenario::initialize(int stage)
{

    BaseScenario::initialize(stage);

    if (stage == 2) {
        // get the speed
        jamSpeed = par("jamSpeed").doubleValue() / 3.6;
        freeSpeed = par("freeSpeed").doubleValue() / 3.6;
        jamDeceleration = par("jamDeceleration").doubleValue();
        jamStart = &par("jamStart");

        startJam = new cMessage("startJam");
        stopJam = new cMessage("stopJam");
        checkSpeed = new cMessage("checkSpeed");

        shockwaveCarIndex = shockwaveCarsCount;
        shockwaveCarsCount++;

        plexeTraciVehicle->setActiveController(plexe::ACC);
        plexeTraciVehicle->setFixedLane(shockwaveCarIndex);
        plexeTraciVehicle->setCruiseControlDesiredSpeed(freeSpeed);

        scheduleAt(simTime() + jamStart->doubleValue(), startJam);

        eventVehicleId.setName("eventVehicleId");
        eventFailure.setName("eventFailure");
        eventInterface.setName("eventInterface");
    }
}

ShockwaveScenario::~ShockwaveScenario()
{
    if (startJam) {
        cancelAndDelete(startJam);
        startJam = nullptr;
    }
    if (stopJam) {
        cancelAndDelete(stopJam);
        stopJam = nullptr;
    }
    if (checkSpeed) {
        cancelAndDelete(checkSpeed);
        checkSpeed = nullptr;
    }
}

void ShockwaveScenario::handleSelfMsg(cMessage* msg)
{
    BaseScenario::handleSelfMsg(msg);
    if (msg == startJam) {
        // start decelerating due to the traffic jam
        plexeTraciVehicle->setFixedAcceleration(1, -jamDeceleration);
        scheduleAt(simTime() + SimTime(0.01), checkSpeed);
        eventVehicleId.record(-1);
        eventFailure.record(2);
        eventInterface.record(0);
    }
    if (msg == checkSpeed) {
        if (mobility->getSpeed() <= jamSpeed) {
            // disable deceleration
            plexeTraciVehicle->setFixedAcceleration(0, 0);
            // set jam speed as cruise speed
            plexeTraciVehicle->setCruiseControlDesiredSpeed(jamSpeed);
        }
        else {
            scheduleAt(simTime() + SimTime(0.01), checkSpeed);
        }
    }
}

}
