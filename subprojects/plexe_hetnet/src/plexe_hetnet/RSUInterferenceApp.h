//
// Copyright (C) 2016 David Eckhoff <eckhoff@cs.fau.de>
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

#pragma once

#include "veins/base/modules/BaseApplLayer.h"
#include "veins/modules/utility/Consts80211p.h"
#include "veins/modules/mobility/traci/TraCICommandInterface.h"

namespace plexe {

class RSUInterferenceApp : public veins::BaseApplLayer {
public:
    ~RSUInterferenceApp() override;
    void initialize(int stage) override;

protected:
    void handleLowerMsg(omnetpp::cMessage* msg) override;
    void handleSelfMsg(omnetpp::cMessage* msg) override;

private:
    veins::TraCICommandInterface* traci = nullptr;
    omnetpp::cPar* beaconInterval = nullptr;
    int packetSize = 0;
    omnetpp::cMessage* sendBeaconEvt = nullptr;
    bool polygonDrawn = false;
};

} // namespace plexe
