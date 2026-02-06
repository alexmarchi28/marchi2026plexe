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

#ifndef SHOCKWAVESCENARIO_H_
#define SHOCKWAVESCENARIO_H_

#include "plexe/scenarios/BaseScenario.h"

#include "plexe/apps/BaseApp.h"

namespace plexe {

class ShockwaveScenario : public BaseScenario {

public:
    virtual void initialize(int stage);

protected:
    // speed while in jam and while in free flow
    double jamSpeed, freeSpeed;
    // deceleration from free speed to jam speed
    double jamDeceleration;
    // start of jam/free periods
    cPar* jamStart;
    // start/stop jam messages
    cMessage *startJam, *stopJam;
    cMessage* checkSpeed;
    // count of shockwave cars in the scenario used to determine the index
    static int shockwaveCarsCount;
    // index of this shockwave car
    int shockwaveCarIndex;

    cOutVector eventVehicleId, eventFailure, eventInterface;

public:
    ShockwaveScenario()
    {
        jamSpeed = 0;
        freeSpeed = 0;
        jamDeceleration = 0;
        jamStart = nullptr;
        startJam = nullptr;
        stopJam = nullptr;
        checkSpeed = nullptr;
        shockwaveCarIndex = 0;
    }
    virtual ~ShockwaveScenario();

protected:
    virtual void handleSelfMsg(cMessage* msg);
};

}

#endif
