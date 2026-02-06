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

#pragma once

#include "plexe/scenarios/BaseScenario.h"
#include "plexe/apps/GeneralPlatooningApp.h"

namespace plexe {

class GapControlTestScenario : public BaseScenario {
public:
    virtual void initialize(int stage) override;
    virtual void handleSelfMsg(cMessage* msg) override;

protected:
    // leader average speed
    double leaderSpeed;
    // application layer, used to start gap control
    GeneralPlatooningApp* appl;

    cMessage* startGapControlMsg;

public:
    GapControlTestScenario()
        : leaderSpeed(0)
        , appl(nullptr)
        , startGapControlMsg(nullptr){};

    virtual ~GapControlTestScenario() override;
};

} // namespace plexe
