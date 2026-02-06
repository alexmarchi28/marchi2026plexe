//
// Copyright (C) 2008 Christoph Sommer <christoph.sommer@informatik.uni-erlangen.de>
// Copyright (C) 2012-2021 Michele Segata <segata@ccs-labs.org>
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

import org.car2x.plexe.utilities.BasePositionHelper;
import org.car2x.plexe.scenarios.ShockwaveScenario;
import org.car2x.veins.modules.mobility.traci.TraCIMobility;
import org.car2x.veins.modules.nic.Nic80211p;

module ShockwaveCar
{
    parameters:
        string helper_type;
    gates:
        input radioIn; // gate for sendDirect
    submodules:

        helper: <helper_type> like BasePositionHelper {
            parameters:
                @display("p=90,100");
        }

        scenario: ShockwaveScenario {
            parameters:
                @display("p=60,100");
        }

        mobility: TraCIMobility {
            parameters:
                @display("p=130,172;i=block/cogwheel");
        }
    connections allowunconnected:

}
