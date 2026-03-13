//
// Copyright (C) 2011 David Eckhoff <eckhoff@cs.fau.de>
//
// Documentation for these modules is at http://veins.car2x.org/
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

#include <list>
#include <sstream>

#include "plexe_hetnet/RSUInterferenceApp.h"

#include "veins/base/modules/BaseMobility.h"
#include "veins/base/utils/FindModule.h"
#include "veins/modules/messages/BaseFrame1609_4_m.h"
#include "veins/modules/mobility/traci/TraCIColor.h"
#include "veins/modules/mobility/traci/TraCIScenarioManager.h"

using namespace veins;

namespace plexe {

Define_Module(plexe::RSUInterferenceApp);

RSUInterferenceApp::~RSUInterferenceApp()
{
    cancelAndDelete(sendBeaconEvt);
    sendBeaconEvt = nullptr;
}

void RSUInterferenceApp::initialize(int stage)
{
    BaseApplLayer::initialize(stage);

    if (stage == 0) {
        headerLength = par("headerLength");
        beaconInterval = &par("beaconInterval");
        packetSize = par("packetSize").intValue();
        sendBeaconEvt = new cMessage("sendBeaconEvt");
        polygonDrawn = false;
    }
    else if (stage == 1) {
        const double firstInterval = beaconInterval->doubleValue();
        if (firstInterval > 0) {
            scheduleAt(simTime() + dblrand() * firstInterval, sendBeaconEvt);
        }
    }
}

void RSUInterferenceApp::handleLowerMsg(cMessage* msg)
{
    delete msg;
}

void RSUInterferenceApp::handleSelfMsg(cMessage* msg)
{
    if (msg != sendBeaconEvt) {
        BaseApplLayer::handleSelfMsg(msg);
        return;
    }

    if (!polygonDrawn) {
        auto* manager = FindModule<TraCIScenarioManager*>::findGlobalModule();
        ASSERT(manager);
        traci = manager->getCommandInterface();
        ASSERT(traci);

        auto* mobility = FindModule<BaseMobility*>::findSubModule(getParentModule());
        ASSERT(mobility);

        const Coord position = mobility->getPositionAt(simTime());
        std::list<Coord> points;
        points.push_back(position);
        points.push_back(Coord(position.x + 10, position.y));
        points.push_back(Coord(position.x + 10, position.y + 10));
        points.push_back(Coord(position.x, position.y + 10));

        std::stringstream id;
        id << "RSUPolygon" << getParentModule()->getIndex();
        traci->addPolygon(id.str(), "RSUType", TraCIColor::fromTkColor("red"), true, 1, points);
        polygonDrawn = true;
    }

    auto* frame = new BaseFrame1609_4("RSUInterferenceMessage");
    frame->setByteLength(packetSize);
    frame->setRecipientAddress(LAddress::L2BROADCAST());
    frame->setChannelNumber(static_cast<int>(Channel::cch));
    frame->setUserPriority(4);
    sendDown(frame);

    const double interval = beaconInterval->doubleValue();
    if (interval > 0) {
        scheduleAt(simTime() + interval, sendBeaconEvt);
    }
}

} // namespace plexe
