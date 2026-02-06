//
// Copyright (C) 2012-2025 Michele Segata <segata@ccs-labs.org>
// Copyright (C) 2018-2025 Julian Heinovski <julian.heinovski@ccs-labs.org>
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

package org.car2x.plexe.apps;

import org.car2x.plexe.apps.BaseApp;

simple GeneralPlatooningApp like BaseApp {

parameters:

    // implementation of the join maneuver to be used
    string joinManeuver;
    // implementation of the platoons merge maneuver
    string mergeManeuver;
    // list of controllers to be used in each state (starting with C_0)
    string C_i = default("ACC PLOEG PLOEG CACC");
    // list of time headways to be used in each state
    string h_i = default("1.2 1.2 0.5 0");
    string d_i = default("2 2 2 5");
    // enable/disable artificial failures for testing purposes
    bool enableArtificialFailures = default(true);
    bool useTemporaryLeader = default(true);
    // string of space separated values indicating artificial failure events
    // the following default values describe 4 events for vehicle 3:
    // - failure of first interface at time 10s
    // - failure of second interface at time 40s
    // - recovery of first interface at time 130s
    // - recovery of second interface at time 160s
    string failureVehicles = default("3 3 3 3");
    string failureTimes = default("10 40 130 160");
    string failureEvents = default("1 1 0 0");
    // dummy fallback mechanism, switching directly to the next controller and gap (omitting the gap control mechanism)
    bool skipGapControl = default(false);

    int headerLength @unit("bit") = default(0 bit);
    @display("i=block/app2");
    @class(plexe::GeneralPlatooningApp);

gates:
    input lowerLayerIn;
    output lowerLayerOut;
    input lowerControlIn;
    output lowerControlOut;
}
