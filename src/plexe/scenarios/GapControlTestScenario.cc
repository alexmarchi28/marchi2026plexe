//
// Copyright (C) 2018-2021 Julian Heinovski <julian.heinovski@ccs-labs.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "plexe/scenarios/GapControlTestScenario.h"

#include "veins/base/utils/FindModule.h"

using namespace veins;

namespace plexe {

Define_Module(GapControlTestScenario);

void GapControlTestScenario::initialize(int stage)
{

    BaseScenario::initialize(stage);

    if (stage == 0)
        // get pointer to application
        appl = FindModule<GeneralPlatooningApp*>::findSubModule(getParentModule());

    if (stage == 2) {
        // average speed
        leaderSpeed = par("leaderSpeed").doubleValue() / 3.6;

        if (positionHelper->isLeader()) {
            // set base cruising speed
            plexeTraciVehicle->setCruiseControlDesiredSpeed(leaderSpeed);
        }
        else {
            // let the follower set a higher desired speed to stay connected
            // to the leader when it is accelerating
            plexeTraciVehicle->setCruiseControlDesiredSpeed(leaderSpeed + 10);
            if (positionHelper->isLast()) {
                startGapControlMsg = new cMessage("startGapControlMsg");
                scheduleAt(5, startGapControlMsg);
            }
        }
    }
}

void GapControlTestScenario::handleSelfMsg(cMessage* msg)
{
    if (msg == startGapControlMsg) {
        auto controller = positionHelper->getController();
        if (controller == plexe::PLOEG)
            appl->startGapControl(1.5, 2, controller);
        else if (controller == plexe::CACC)
            appl->startGapControl(0, 20, controller);
        else if (controller == plexe::ACC)
            appl->startGapControl(2.0, 2, controller);
    }
    BaseScenario::handleSelfMsg(msg);
}

GapControlTestScenario::~GapControlTestScenario()
{
    cancelAndDelete(startGapControlMsg);
    startGapControlMsg = nullptr;
}

} // namespace plexe
