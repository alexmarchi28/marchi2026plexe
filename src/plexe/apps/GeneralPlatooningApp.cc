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

#include "plexe/apps/GeneralPlatooningApp.h"

#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>

#include "plexe/protocols/BaseProtocol.h"
#include "veins/modules/mobility/traci/TraCIColor.h"
#include "veins/modules/mobility/traci/TraCIScenarioManager.h"
#include "veins/modules/messages/BaseFrame1609_4_m.h"
#include "veins/modules/utility/Consts80211p.h"
#include "veins/modules/mac/ieee80211p/Mac1609_4.h"
#include "plexe/messages/PlexeInterfaceControlInfo_m.h"
#include "veins/base/utils/FindModule.h"
#include "plexe/scenarios/ManeuverScenario.h"

using namespace veins;

namespace plexe {

Define_Module(GeneralPlatooningApp);

void GeneralPlatooningApp::initialize(int stage)
{
    BaseApp::initialize(stage);

    if (stage == 1) {
        // connect maneuver application to protocol
        protocol->registerApplication(MANEUVER_TYPE, gate("lowerLayerIn"), gate("lowerLayerOut"), gate("lowerControlIn"), gate("lowerControlOut"));
        // request beacons as well to be able to approach the platoon
        protocol->registerApplication(BaseProtocol::BEACON_TYPE, gate("lowerLayerIn"), gate("lowerLayerOut"), gate("lowerControlIn"), gate("lowerControlOut"));
        // register to the signal indicating failed unicast transmissions
        findHost()->subscribe(Mac1609_4::sigRetriesExceeded, this);

        std::string joinManeuverName = par("joinManeuver").stdstringValue();
        if (joinManeuverName == "JoinAtBack")
            joinManeuver = new JoinAtBack(this);
        else
            throw new cRuntimeError("Invalid join maneuver implementation chosen");

        std::string mergeManeuverName = par("mergeManeuver").stdstringValue();
        if (mergeManeuverName == "MergeAtBack")
            mergeManeuver = new MergeAtBack(this);
        else
            throw new cRuntimeError("Invalid merge maneuver implementation chosen");

        if (positionHelper->isLeader()) setPlatoonRole(PlatoonRole::LEADER);
        else setPlatoonRole(PlatoonRole::FOLLOWER);

        scenario = FindModule<BaseScenario*>::findSubModule(getParentModule());

        useTemporaryLeader = par("useTemporaryLeader").boolValue();
        skipGapControl = par("skipGapControl").boolValue();

        sigInterfaceFailure = findHost()->registerSignal("org_car2x_plexe_protocols_baseProtocol_sigInterfaceFailure");
        sigInterfaceRecovery = findHost()->registerSignal("org_car2x_plexe_protocols_baseProtocol_sigInterfaceRecovery");

        enableArtificialFailures = par("enableArtificialFailures").boolValue();
        if (enableArtificialFailures) {
            // counts how many intefaces have failed so far
            artificiallyFailedCount = 0;

            // parse parameters related to failure events
            std::vector<int> failureVehicles = cStringTokenizer(par("failureVehicles").stringValue()).asIntVector();
            std::vector<double> failureTimes = cStringTokenizer(par("failureTimes").stringValue()).asDoubleVector();
            std::vector<int> failureEvents = cStringTokenizer(par("failureEvents").stringValue()).asIntVector();
            if (failureVehicles.size() != failureTimes.size() && failureVehicles.size() != failureEvents.size()) throw cRuntimeError("Invalid number of elements in failure events parameters");

            // schedule failures for this vehicle
            for (int i = 0; i < failureVehicles.size(); i++) {
                if (failureVehicles[i] != myId) continue;
                FailureMessage* failure = new FailureMessage();
                failure->failure = failureEvents[i];
                scheduleAt(failureTimes[i], failure);
            }
        }
        else {
            // subscribe to the signals sent by the monitor of the real interfaces
            findHost()->subscribe(sigInterfaceFailure, this);
            findHost()->subscribe(sigInterfaceRecovery, this);
        }

        // convert list of controllers into C_i vector
        std::string strCi = par("C_i").stringValue();
        std::vector<std::string> CiVector = cStringTokenizer(strCi.c_str()).asVector();
        if (CiVector.size() != N_INTERFACES + 1) throw new cRuntimeError("Invalid number of controllers in C_i. Got %d, expecting %d", CiVector.size(), N_INTERFACES + 1);
        for (int i = 0; i < static_cast<int>(CiVector.size()); i++) C_i[i] = strToController(CiVector[i].c_str());

        // convert list of headways and distances into h_i and d_i vectors
        std::string strHi = par("h_i").stringValue();
        std::vector<double> hiVector = cStringTokenizer(strHi.c_str()).asDoubleVector();
        if (hiVector.size() != N_INTERFACES + 1) throw new cRuntimeError("Invalid number of controllers in h_i. Got %d, expecting %d", hiVector.size(), N_INTERFACES + 1);
        for (int i = 0; i < static_cast<int>(hiVector.size()); i++) h_i[i] = hiVector[i];

        std::string strDi = par("d_i").stringValue();
        std::vector<double> diVector = cStringTokenizer(strDi.c_str()).asDoubleVector();
        if (diVector.size() != N_INTERFACES + 1) throw new cRuntimeError("Invalid number of controllers in d_i. Got %d, expecting %d", diVector.size(), N_INTERFACES + 1);
        for (int i = 0; i < static_cast<int>(diVector.size()); i++) d_i[i] = diVector[i];

        // INIT procedure
        init();

        eventVehicleId.setName("eventVehicleId");
        eventFailure.setName("eventFailure");
        eventInterface.setName("eventInterface");

        enableLogging();
    }
}

void GeneralPlatooningApp::handleSelfMsg(cMessage* msg)
{
    if (msg == updateGapMsg) {
        updateGap();
        return;
    }
    if (FailureMessage* failure = dynamic_cast<FailureMessage*>(msg)) {
        if (failure->failure) {
            // emulate a different interface type exploiting the count of failed interfaces
            receiveSignal(nullptr, sigInterfaceFailure, (long int)artificiallyFailedCount, nullptr);
            artificiallyFailedCount++;
        }
        else {
            if (artificiallyFailedCount == 0) throw cRuntimeError("Recovery event occurred with no failed interfaces");
            artificiallyFailedCount--;
            receiveSignal(nullptr, sigInterfaceRecovery, (long int)artificiallyFailedCount, nullptr);
        }
        delete failure;
        return;
    }
    if (joinManeuver && joinManeuver->handleSelfMsg(msg)) return;
    if (mergeManeuver && mergeManeuver->handleSelfMsg(msg)) return;
    BaseApp::handleSelfMsg(msg);
}

bool GeneralPlatooningApp::isJoinAllowed() const
{
    return ((role == PlatoonRole::LEADER || role == PlatoonRole::NONE) && !inManeuver);
}

double GeneralPlatooningApp::getStandstillDistance(enum ACTIVE_CONTROLLER controller)
{
    return scenario->getStandstillDistance(controller);
}

double GeneralPlatooningApp::getHeadway(enum ACTIVE_CONTROLLER controller)
{
    return scenario->getHeadway(controller);
}

double GeneralPlatooningApp::getTargetDistance(enum ACTIVE_CONTROLLER controller, double speed)
{
    return scenario->getTargetDistance(controller, speed);
}

double GeneralPlatooningApp::getTargetDistance(double speed)
{
    return scenario->getTargetDistance(speed);
}

enum ACTIVE_CONTROLLER GeneralPlatooningApp::getTargetController()
{
    ManeuverScenario* maneuverScenario = dynamic_cast<ManeuverScenario*>(scenario);
    if (!maneuverScenario) throw cRuntimeError("getTargetController() invoked from a simulation not inheriting from ManeuverScenario");
    return maneuverScenario->getTargetController();
}

enum ACTIVE_CONTROLLER GeneralPlatooningApp::strToController(const char* controller)
{
    if (strcmp(controller, "ACC") == 0) {
        return ACC;
    }
    else if (strcmp(controller, "CACC") == 0) {
        return CACC;
    }
    else if (strcmp(controller, "PLOEG") == 0) {
        return PLOEG;
    }
    else if (strcmp(controller, "CONSENSUS") == 0) {
        return CONSENSUS;
    }
    else if (strcmp(controller, "FLATBED") == 0) {
        return FLATBED;
    }
    else {
        throw cRuntimeError("Invalid controller selected");
    }
}

bool GeneralPlatooningApp::isLeaderBased(enum ACTIVE_CONTROLLER controller)
{
    switch (controller) {
    case CACC:
    case CONSENSUS:
    case FLATBED:
        return true;
    default:
        return false;
    }
}

bool GeneralPlatooningApp::usesTimeHeadway(enum ACTIVE_CONTROLLER controller)
{
    switch (controller) {
    case CACC:
    case FLATBED:
        return false;
    default:
        return true;
    }
}

void GeneralPlatooningApp::startJoinManeuver(int platoonId, int leaderId, int position)
{
    ASSERT(getPlatoonRole() == PlatoonRole::NONE);
    ASSERT(!isInManeuver());

    JoinManeuverParameters params;
    params.platoonId = platoonId;
    params.leaderId = leaderId;
    params.position = position;
    joinManeuver->startManeuver(&params);
}

void GeneralPlatooningApp::startMergeManeuver(int platoonId, int leaderId, int position)
{
    ASSERT(getPlatoonRole() == PlatoonRole::LEADER);
    ASSERT(!isInManeuver());

    JoinManeuverParameters params;
    params.platoonId = platoonId;
    params.leaderId = leaderId;
    params.position = position;
    mergeManeuver->startManeuver(&params);
}

void GeneralPlatooningApp::sendUnicast(cPacket* msg, int destination, short type)
{
    Enter_Method_Silent();
    take(msg);
    sendFrame(msg, destination, type, PlexeRadioInterfaces::VEINS_11P);
}

void GeneralPlatooningApp::handleLowerMsg(cMessage* msg)
{
    BaseFrame1609_4* frame = check_and_cast<BaseFrame1609_4*>(msg);

    cPacket* enc = frame->getEncapsulatedPacket();
    ASSERT2(enc, "received a BaseFrame1609_4s with nothing inside");

    if (enc->getKind() == MANEUVER_TYPE) {
        ManeuverMessage* mm = check_and_cast<ManeuverMessage*>(frame->decapsulate());
        if (UpdatePlatoonData* msg = dynamic_cast<UpdatePlatoonData*>(mm)) {
            handleUpdatePlatoonData(msg);
            delete msg;
        }
        else if (UpdatePlatoonFormation* msg = dynamic_cast<UpdatePlatoonFormation*>(mm)) {
            handleUpdatePlatoonFormation(msg);
            delete msg;
        }
        else {
            onManeuverMessage(mm);
        }
        delete frame;
    }
    else if (enc->getKind() == BaseProtocol::BEACON_TYPE) {
        PlatooningBeacon* pb = check_and_cast<PlatooningBeacon*>(frame->decapsulate());
        onPlatoonBeacon(pb);
        delete frame;
    }
    else {
        BaseApp::handleLowerMsg(msg);
    }
}

void GeneralPlatooningApp::handleUpdatePlatoonData(const UpdatePlatoonData* msg)
{
    if (getPlatoonRole() != PlatoonRole::FOLLOWER) return;
    if (msg->getPlatoonId() != positionHelper->getPlatoonId()) return;
    if (msg->getVehicleId() != positionHelper->getLeaderId()) return;

    handleUpdatePlatoonFormation(msg);
    LOG << positionHelper->getId() << " changing platoon id from " << positionHelper->getPlatoonId() << " to " << msg->getNewPlatoonId() << "\n";
    positionHelper->setPlatoonId(msg->getNewPlatoonId());
}

void GeneralPlatooningApp::handleUpdatePlatoonFormation(const UpdatePlatoonFormation* msg)
{
    if (getPlatoonRole() != PlatoonRole::FOLLOWER) return;
    if (msg->getPlatoonId() != positionHelper->getPlatoonId()) return;
    if (msg->getVehicleId() != positionHelper->getLeaderId()) return;

    // update formation information
    LOG << positionHelper->getId() << " changing platoon formation: ";
    std::vector<int> f;
    for (unsigned int i = 0; i < msg->getPlatoonFormationArraySize(); i++) {
        f.push_back(msg->getPlatoonFormation(i));
        LOG << msg->getPlatoonFormation(i) << " ";
    }
    LOG << "\n";
    positionHelper->setPlatoonFormation(f);
}

void GeneralPlatooningApp::setPlatoonRole(PlatoonRole r)
{
    role = r;
}

void GeneralPlatooningApp::onPlatoonBeacon(const PlatooningBeacon* pb)
{
    joinManeuver->onPlatoonBeacon(pb);
    mergeManeuver->onPlatoonBeacon(pb);
    if (positionHelper->isInSamePlatoon(pb->getVehicleId())) {
        if (pb->getVehicleId() == positionHelper->getLeaderId()) {
            plexeTraciVehicle->setLeaderVehicleData(pb->getControllerAcceleration(), pb->getAcceleration(), pb->getSpeed(), pb->getPositionX(), pb->getPositionY(), pb->getTime());
        }
        if (pb->getVehicleId() == positionHelper->getFrontId()) {
            plexeTraciVehicle->setFrontVehicleData(pb->getControllerAcceleration(), pb->getAcceleration(), pb->getSpeed(), pb->getPositionX(), pb->getPositionY(), pb->getTime());
        }

        struct VEHICLE_DATA vehicleData;
        vehicleData.index = positionHelper->getMemberPosition(pb->getVehicleId());
        vehicleData.acceleration = pb->getAcceleration();
        vehicleData.length = pb->getLength();
        vehicleData.positionX = pb->getPositionX();
        vehicleData.positionY = pb->getPositionY();
        vehicleData.speed = pb->getSpeed();
        vehicleData.time = pb->getTime();
        vehicleData.u = pb->getControllerAcceleration();
        vehicleData.speedX = pb->getSpeedX();
        vehicleData.speedY = pb->getSpeedY();
        vehicleData.angle = pb->getAngle();
        plexeTraciVehicle->setVehicleData(&vehicleData);

        onTemporaryLeader(pb->getVehicleId(), pb->getTemporaryLeader());
    }
    delete pb;
}

void GeneralPlatooningApp::onManeuverMessage(ManeuverMessage* mm)
{
    if (activeManeuver) {
        activeManeuver->onManeuverMessage(mm);
    }
    else {
        joinManeuver->onManeuverMessage(mm);
        mergeManeuver->onManeuverMessage(mm);
    }
    delete mm;
}

void GeneralPlatooningApp::fillManeuverMessage(ManeuverMessage* msg, int vehicleId, std::string externalId, int platoonId, int destinationId)
{
    msg->setKind(MANEUVER_TYPE);
    msg->setVehicleId(vehicleId);
    msg->setExternalId(externalId.c_str());
    msg->setPlatoonId(platoonId);
    msg->setDestinationId(destinationId);
}

UpdatePlatoonData* GeneralPlatooningApp::createUpdatePlatoonData(int vehicleId, std::string externalId, int platoonId, int destinationId, double platoonSpeed, int platoonLane, const std::vector<int>& platoonFormation, int newPlatoonId)
{
    UpdatePlatoonData* msg = new UpdatePlatoonData("UpdatePlatoonData");
    fillManeuverMessage(msg, vehicleId, externalId, platoonId, destinationId);
    msg->setPlatoonSpeed(platoonSpeed);
    msg->setPlatoonLane(platoonLane);
    msg->setPlatoonFormationArraySize(platoonFormation.size());
    for (unsigned int i = 0; i < platoonFormation.size(); i++) {
        msg->setPlatoonFormation(i, platoonFormation[i]);
    }
    msg->setNewPlatoonId(newPlatoonId);
    return msg;
}

UpdatePlatoonFormation* GeneralPlatooningApp::createUpdatePlatoonFormation(int vehicleId, std::string externalId, int platoonId, int destinationId, double platoonSpeed, int platoonLane, const std::vector<int>& platoonFormation)
{
    UpdatePlatoonFormation* msg = new UpdatePlatoonFormation("UpdatePlatoonFormation");
    fillManeuverMessage(msg, vehicleId, externalId, platoonId, destinationId);
    msg->setPlatoonSpeed(platoonSpeed);
    msg->setPlatoonLane(platoonLane);
    msg->setPlatoonFormationArraySize(platoonFormation.size());
    for (unsigned int i = 0; i < platoonFormation.size(); i++) {
        msg->setPlatoonFormation(i, platoonFormation[i]);
    }
    return msg;
}

void GeneralPlatooningApp::receiveSignal(cComponent* src, simsignal_t id, cObject* value, cObject* details)
{
    if (id == Mac1609_4::sigRetriesExceeded) {
        BaseFrame1609_4* frame = check_and_cast<BaseFrame1609_4*>(value);
        ManeuverMessage* mm = check_and_cast<ManeuverMessage*>(frame->getEncapsulatedPacket());
        if (frame) {
            joinManeuver->onFailedTransmissionAttempt(mm);
            mergeManeuver->onFailedTransmissionAttempt(mm);
        }
    }
}

void GeneralPlatooningApp::receiveSignal(cComponent* source, simsignal_t signalID, long l, cObject* details)
{
    if (signalID == sigInterfaceFailure) {
        if (active[l]) {
            active[l] = false;
            failure();
            eventVehicleId.record(myId);
            eventFailure.record(1);
            eventInterface.record(l);
        }
    }
    else if (signalID == sigInterfaceRecovery) {
        if (!active[l]) {
            active[l] = true;
            recover();
            eventVehicleId.record(myId);
            eventFailure.record(0);
            eventInterface.record(l);
        }
    }
}

void GeneralPlatooningApp::scheduleSelfMsg(simtime_t t, cMessage* msg)
{
    scheduleAt(t, msg);
}

void GeneralPlatooningApp::startGapControl(double h_t, double d_t, enum ACTIVE_CONTROLLER controller)
{
    Enter_Method_Silent();

    double rv, currentDistance;
    this->h_t = h_t;
    this->d_t = d_t;
    plexeTraciVehicle->getRadarMeasurements(currentDistance, rv);

    g = currentDistance;
    v = traciVehicle->getSpeed();

    deltaT = 0.1; // s
    deltaG = 1; // m / s
    deltaH = deltaG / v; // s/s = #

    g_t = h_t * v + d_t;

    if (usesTimeHeadway(controller)) {
        g = d_t;
        h = (currentDistance - d_t) / v;
        if (h_t < h) increasingGap = false;
        else increasingGap = true;
    }
    else {
        g = currentDistance;
        h = 0;
        if (g_t < g) increasingGap = false;
        else increasingGap = true;
    }
    if (!updateGapMsg)
        updateGapMsg = new cMessage("updateGap");
    if (!gapControlEnabled)
        scheduleAt(simTime(), updateGapMsg);
    gapControlEnabled = true;
}

bool GeneralPlatooningApp::usingTimeHeadway()
{
    switch (plexeTraciVehicle->getActiveController()) {
    case plexe::CACC:
        return false;
        break;
    case plexe::PLOEG:
    case plexe::ACC:
        return true;
        break;
    default:
        throw new cRuntimeError("Undefined");
        break;
    }
}

void GeneralPlatooningApp::setControllerGap(double h, double d)
{
    plexeTraciVehicle->setCACCConstantSpacing(d);
    plexeTraciVehicle->setPloegCACCParameters(-1, -1, h);
    plexeTraciVehicle->setACCHeadwayTime(h);
}

void GeneralPlatooningApp::debugGapControlInfo()
{
    double rv, currentDistance;
    plexeTraciVehicle->getRadarMeasurements(currentDistance, rv);
    double distanceShouldBe;

    std::cout << std::fixed << std::showpoint;
    std::cout << std::setprecision(2);

    if (usingTimeHeadway()) {
        std::cout << "Regulating time headway ";
        distanceShouldBe = h * v + d_t;
    }
    else {
        std::cout << "Regulating gap ";
        distanceShouldBe = g;
    }
    std::cout << "g_t = h_t (" << h_t << ") * v (" << v << ") + d_t (" << d_t << ") = " << g_t << "\n";
    std::cout << "distance should be d (" << distanceShouldBe << ") (h = " << h << "). Actual distance " << currentDistance << " (remaining " << std::abs(distanceShouldBe - g_t) << ", tracking error " << distanceShouldBe - currentDistance << ")\n";
}

bool GeneralPlatooningApp::isGapReached()
{
    double rv, curDistance;
    plexeTraciVehicle->getRadarMeasurements(curDistance, rv);
    if ((increasingGap && curDistance >= g_t) || (!increasingGap && curDistance <= g_t))
        return true;
    return false;
}

bool GeneralPlatooningApp::isGapControlCompleted()
{
    if (usingTimeHeadway()) {
        if ((increasingGap && h >= h_t) || (!increasingGap && h <= h_t))
            return true;
    }
    else {
        if ((increasingGap && g >= g_t) || (!increasingGap && g <= g_t))
            return true;
    }
    return false;
}

void GeneralPlatooningApp::updateGap()
{
    v = traciVehicle->getSpeed();
    deltaH = deltaG / v; // s/s = #
    g_t = h_t * v + d_t;
    if (isGapControlCompleted()) {
        if (usingTimeHeadway()) {
            h = h_t;
        }
        else {
            g = g_t;
        }
        setControllerGap(h, g);
        if (isGapReached()) {
            gapControlEnabled = false;
            gapReached();
            return;
        }
    }
    else {
        if (usingTimeHeadway()) {
            h = h + (h_t - h > 0 ? 1 : -1) * deltaH * deltaT;
        }
        else {
            g = g + (g_t - g > 0 ? 1 : -1) * deltaG * deltaT;
        }
        setControllerGap(h, g);
    }
    scheduleAt(simTime() + deltaT, updateGapMsg);
}

void GeneralPlatooningApp::init()
{
    i = N_INTERFACES;
    state = FOLLOW;
    // Keep the leader on ACC as configured by traffic/scenario initialization.
    // Applying C_i[i] to the leader can force CACC settings intended for followers,
    // which destabilizes shockwave experiments.
    if (!positionHelper->isLeader()) {
        plexeTraciVehicle->setActiveController(C_i[i]);
        setControllerGap(h_i[i], d_i[i]);
    }
    tempLeaders.insert(0);
    protocol->setTemporaryLeader(false);

    for (int i = 0; i < N_INTERFACES; i++) active[i] = true;
}

void GeneralPlatooningApp::failure()
{
    // if we reached complete failures, we ignored recoveries and i will be stuck to 0
    // we also need to ignore failures after that point
    if (i == 0) return;

    if (skipGapControl) {
        setControllerGap(h_i[i - 1], d_i[i - 1]);
        plexeTraciVehicle->setActiveController(C_i[i - 1]);
        state = FOLLOW;
        i = i - 1;
        return;
    }

    startGapControl(h_i[i - 1], d_i[i - 1], (enum ACTIVE_CONTROLLER)plexeTraciVehicle->getActiveController());
    state = GAP_CONTROL;
    if (isLeaderBased(C_i[i]) && !isLeaderBased(C_i[i - 1])) protocol->setTemporaryLeader(true);
    i = i - 1;
}

void GeneralPlatooningApp::recover()
{
    // we do not consider how to deal with recoveries after complete failure
    if (i == 0) return;

    if (skipGapControl) {
        setControllerGap(h_i[i + 1], d_i[i + 1]);
        plexeTraciVehicle->setActiveController(C_i[i + 1]);
        state = FOLLOW;
        i = i + 1;
        return;
    }

    double v = traciVehicle->getSpeed();
    double g_i = h_i[i] * v + d_i[i];
    if (usesTimeHeadway(C_i[i + 1])) {
        double h = (g_i - d_i[i + 1]) / v;
        setControllerGap(h, d_i[i + 1]);
    }
    else {
        setControllerGap(0, g_i);
    }
    plexeTraciVehicle->setActiveController(C_i[i + 1]);
    startGapControl(h_i[i + 1], d_i[i + 1], C_i[i + 1]);
    state = GAP_CONTROL;
    i = i + 1;
}

void GeneralPlatooningApp::gapReached()
{
    state = FOLLOW;
    plexeTraciVehicle->setActiveController(C_i[i]);
    setControllerGap(h_i[i], d_i[i]);
    if (isLeaderBased(C_i[i])) protocol->setTemporaryLeader(false);
}

void GeneralPlatooningApp::timeout()
{
    if (i > 1) throw cRuntimeError("Got a timeout with more than one active interfaces!");
    failure();
}

void GeneralPlatooningApp::onTemporaryLeader(int veh, bool tempLeader)
{
    if (useTemporaryLeader) {
        // ignore temporary leader advertisement coming from vehicles behind us (or from the original leader)
        int position = positionHelper->getMemberPosition(veh);
        if (position == 0 || position > positionHelper->getPosition()) return;

        if (tempLeader) tempLeaders.insert(position);
        else tempLeaders.erase(position);

        int leader = *(tempLeaders.rbegin());
        if (positionHelper->getMemberPosition(leader) == 0) positionHelper->setTemporaryLeader(false);
        else positionHelper->setTemporaryLeader(true, leader);
    }
}

GeneralPlatooningApp::~GeneralPlatooningApp()
{
    delete joinManeuver;
    delete mergeManeuver;
    cancelAndDelete(updateGapMsg);
    updateGapMsg = nullptr;
}

} // namespace plexe
