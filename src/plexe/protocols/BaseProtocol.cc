//
// Copyright (C) 2012-2025 Michele Segata <segata@ccs-labs.org>
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

#include "plexe/protocols/BaseProtocol.h"

#include "veins/modules/mac/ieee80211p/Mac1609_4.h"
#include "veins/base/utils/FindModule.h"
#include "veins/modules/messages/BaseFrame1609_4_m.h"

#include "plexe/PlexeManager.h"
#include "plexe/driver/Veins11pRadioDriver.h"
#include "plexe/messages/PlexeInterfaceControlInfo_m.h"

using namespace veins;

namespace plexe {

Define_Module(BaseProtocol);

const simsignal_t BaseProtocol::sigInterfaceFailure = registerSignal("org_car2x_plexe_protocols_baseProtocol_sigInterfaceFailure");
const simsignal_t BaseProtocol::sigInterfaceRecovery = registerSignal("org_car2x_plexe_protocols_baseProtocol_sigInterfaceRecovery");

const int BaseProtocol::BEACON_TYPE = 12345;

// CAREFUL: the index of the interface depends on the order they are connected inside the PlatoonCarHetNet file
#define I11P 0
#define ICV2X 1
#define IVLC 2

void BaseProtocol::initialize(int stage)
{

    BaseApplLayer::initialize(stage);

    if (stage == 0) {

        // init class variables
        sendBeacon = 0;
        seq_n = 0;
        recordData = 0;

        // get gates
        lowerControlIn = findGate("lowerControlIn");
        lowerControlOut = findGate("lowerControlOut");
        lowerLayerIn = findGate("lowerLayerIn");
        lowerLayerOut = findGate("lowerLayerOut");
        minUpperId = gate("upperLayerIn", 0)->getId();
        maxUpperId = gate("upperLayerIn", MAX_GATES_COUNT - 1)->getId();
        minUpperControlId = gate("upperControlIn", 0)->getId();
        maxUpperControlId = gate("upperControlIn", MAX_GATES_COUNT - 1)->getId();
        minRadioId = gate("radiosIn", 0)->getId();
        maxRadioId = gate("radiosIn", gateSize("radiosIn") - 1)->getId();

        // get information about output radio interfaces and store them
        for (int i = 0; i < gateSize("radiosOut"); i++) {
            PlexeRadioDriverInterface* radio = check_and_cast<PlexeRadioDriverInterface*>(gate("radiosOut", i)->getNextGate()->getOwnerModule());
            radioOuts[radio->getDeviceType()] = gate("radiosOut", i);
        }

        // beaconing interval in seconds
        beaconingInterval = SimTime(par("beaconingInterval").doubleValue());
        // platooning message packet size
        packetSize = par("packetSize");
        // priority of platooning message
        priority = par("priority");
        ASSERT2(priority >= 0 && priority <= 7, "priority value must be between 0 and 7");

        // init messages for scheduleAt
        sendBeacon = new cMessage("sendBeacon");
        recordData = new cMessage("recordData");

        statsIdOut.setName("statsId");
        leaderFer11pOut.setName("leaderFer11p");
        leaderFerVLCOut.setName("leaderFerVLC");
        leaderFerLTEOut.setName("leaderFerLTE");
        frontFer11pOut.setName("frontFer11p");
        frontFerVLCOut.setName("frontFerVLC");
        frontFerLTEOut.setName("frontFerLTE");
        leaderDelay11pOut.setName("leaderDelay11p");
        leaderDelayVLCOut.setName("leaderDelayVLC");
        leaderDelayLTEOut.setName("leaderDelayLTE");
        frontDelay11pOut.setName("frontDelay11p");
        frontDelayVLCOut.setName("frontDelayVLC");
        frontDelayLTEOut.setName("frontDelayLTE");
        leaderInterarrival11pOut.setName("leaderInterarrival11p");
        leaderInterarrivalVLCOut.setName("leaderInterarrivalVLC");
        leaderInterarrivalLTEOut.setName("leaderInterarrivalLTE");
        frontInterarrival11pOut.setName("frontInterarrival11p");
        frontInterarrivalVLCOut.setName("frontInterarrivalVLC");
        frontInterarrivalLTEOut.setName("frontInterarrivalLTE");

        handoverIdOut.setName("handoverId");
        handoverStartOut.setName("handoverStart");


        // init statistics collection. round to second
        SimTime rounded = SimTime(floor(simTime().dbl() + 1), SIMTIME_S);
        scheduleAt(rounded, recordData);
    }

    if (stage == 1) {
        // get traci interface
        mobility = veins::TraCIMobilityAccess().get(getParentModule());
        ASSERT(mobility);
        traci = mobility->getCommandInterface();
        ASSERT(traci);
        traciVehicle = mobility->getVehicleCommandInterface();
        ASSERT(traciVehicle);
        auto plexe = FindModule<PlexeManager*>::findGlobalModule();
        ASSERT(plexe);
        plexeTraci = plexe->getCommandInterface();
        plexeTraciVehicle.reset(new traci::CommandInterface::Vehicle(plexeTraci, mobility->getExternalId()));
        positionHelper = FindModule<BasePositionHelper*>::findSubModule(getParentModule());
        ASSERT(positionHelper);

        // this is the id of the vehicle. used also as network address
        myId = positionHelper->getId();
        length = traciVehicle->getLength();
        if (Veins11pRadioDriver* driver = FindModule<Veins11pRadioDriver*>::findSubModule(getParentModule())) {
            driver->registerNode(myId);
        }
        frameStatsWindow = par("frameStatsWindow");
        leaderFrames = new FramesRingBuffer(frameStatsWindow);
        frontFrames = new FramesRingBuffer(frameStatsWindow);

        lte_stack_phy_handover = findHost()->registerSignal("lte_stack_phy_handover");
        findHost()->subscribe(lte_stack_phy_handover, this);

        if (!positionHelper->isLeader()) {
            deltaT = par("deltaT").doubleValue();
            minFramesForFailure = par("minFramesForFailure").intValue();
            pdr11p = par("pdr11p").doubleValue();
            pdrCV2X = par("pdrCV2X").doubleValue();
            pdrVLC = par("pdrVLC").doubleValue();
            double pdrs[N_INTERFACES];
            pdrs[I11P] = pdr11p;
            pdrs[ICV2X] = pdrCV2X;
            pdrs[IVLC] = pdrVLC;

            checkLeaderInterfacesStatus = new cMessage("checkLeaderInterfacesStatus");
            checkFrontInterfacesStatus = new cMessage("checkFrontInterfacesStatus");
            for (int i = 0; i < N_INTERFACES; i++) {
                leaderMonitors[i] = new InterfaceMonitor(leaderFrames, i, pdrs[i], deltaT, minFramesForFailure);
                frontMonitors[i] = new InterfaceMonitor(frontFrames, i, pdrs[i], deltaT, minFramesForFailure);
            }
        }
        else {
            for (int i = 0; i < N_INTERFACES; i++) {
                leaderMonitors[i] = nullptr;
                frontMonitors[i] = nullptr;
            }
        }
    }
}

BaseProtocol::~BaseProtocol()
{
    delete leaderFrames;
    delete frontFrames;
    cancelAndDelete(sendBeacon);
    sendBeacon = nullptr;
    cancelAndDelete(recordData);
    recordData = nullptr;
    cancelAndDelete(checkLeaderInterfacesStatus);
    checkLeaderInterfacesStatus = nullptr;
    cancelAndDelete(checkFrontInterfacesStatus);
    checkFrontInterfacesStatus = nullptr;
    for (int i = 0; i < N_INTERFACES; i++) {
        delete leaderMonitors[i];
        delete frontMonitors[i];
    }
}

void BaseProtocol::handleSelfMsg(cMessage* msg)
{

    if (msg == recordData) {

        double fer[N_INTERFACES];
        double delays[N_INTERFACES];
        double interarrivals[N_INTERFACES];

        statsIdOut.record(myId);

        leaderFrames->getStats(fer, delays, interarrivals, true);
        leaderFer11pOut.record(fer[I11P]);
        leaderFerVLCOut.record(fer[IVLC]);
        leaderFerLTEOut.record(fer[ICV2X]);
        leaderDelay11pOut.record(delays[I11P]);
        leaderDelayVLCOut.record(delays[IVLC]);
        leaderDelayLTEOut.record(delays[ICV2X]);
        leaderInterarrival11pOut.record(interarrivals[I11P]);
        leaderInterarrivalVLCOut.record(interarrivals[IVLC]);
        leaderInterarrivalLTEOut.record(interarrivals[ICV2X]);

        frontFrames->getStats(fer, delays, interarrivals, true);
        frontFer11pOut.record(fer[I11P]);
        frontFerVLCOut.record(fer[IVLC]);
        frontFerLTEOut.record(fer[ICV2X]);
        frontDelay11pOut.record(delays[I11P]);
        frontDelayVLCOut.record(delays[IVLC]);
        frontDelayLTEOut.record(delays[ICV2X]);
        frontInterarrival11pOut.record(interarrivals[I11P]);
        frontInterarrivalVLCOut.record(interarrivals[IVLC]);
        frontInterarrivalLTEOut.record(interarrivals[ICV2X]);

        scheduleAt(simTime() + SimTime(1, SIMTIME_S), recordData);
    }
    else if (msg == checkLeaderInterfacesStatus) {
        for (int i = 0; i < N_INTERFACES; i++) {
            switch (leaderMonitors[i]->checkStatus(simTime().dbl())) {
            case InterfaceMonitor::SIGNAL_FAILURE:
                emit(sigInterfaceFailure, i);
                break;
            case InterfaceMonitor::SIGNAL_RECOVERY:
                emit(sigInterfaceRecovery, i);
                break;
            default:
                break;
            }
        }
    }
    else if (msg == checkFrontInterfacesStatus) {
        for (int i = 0; i < N_INTERFACES; i++) {
            switch (frontMonitors[i]->checkStatus(simTime().dbl())) {
            case InterfaceMonitor::SIGNAL_FAILURE:
                emit(sigInterfaceFailure, i);
                break;
            case InterfaceMonitor::SIGNAL_RECOVERY:
                emit(sigInterfaceRecovery, i);
                break;
            default:
                break;
            }
        }
    }
}

void BaseProtocol::sendPlatooningMessage(int destinationAddress, enum PlexeRadioInterfaces interfaces)
{
    sendTo(createBeacon(destinationAddress).release(), interfaces);
}

void BaseProtocol::sendTo(BaseFrame1609_4* frame, enum PlexeRadioInterfaces interfaces)
{
    for (auto interface : radioOuts) {
        if (interface.first & interfaces) {
            BaseFrame1609_4* dup = frame->dup();
            if (frame->getControlInfo()) dup->setControlInfo(frame->getControlInfo()->dup());
            send(dup, interface.second);
        }
    }
    delete frame;
}

void BaseProtocol::setTemporaryLeader(bool tempLeader)
{
    temporaryLeader = tempLeader;
}

std::unique_ptr<BaseFrame1609_4> BaseProtocol::createBeacon(int destinationAddress)
{
    // vehicle's data to be included in the message
    VEHICLE_DATA data;
    // get information about the vehicle via traci
    plexeTraciVehicle->getVehicleData(&data);

    // create and send beacon
    auto wsm = veins::make_unique<BaseFrame1609_4>("", BEACON_TYPE);
    wsm->setRecipientAddress(LAddress::L2BROADCAST());
    wsm->setChannelNumber(static_cast<int>(Channel::cch));
    wsm->setUserPriority(priority);

    // create platooning beacon with data about the car
    PlatooningBeacon* pkt = new PlatooningBeacon();
    pkt->setControllerAcceleration(data.u);
    pkt->setAcceleration(data.acceleration);
    pkt->setSpeed(data.speed);
    pkt->setVehicleId(myId);
    pkt->setPositionX(data.positionX);
    pkt->setPositionY(data.positionY);
    // set the time to now
    pkt->setTime(data.time);
    pkt->setLength(length);
    pkt->setSpeedX(data.speedX);
    pkt->setSpeedY(data.speedY);
    pkt->setAngle(data.angle);
    pkt->setKind(BEACON_TYPE);
    pkt->setByteLength(packetSize);
    pkt->setSequenceNumber(seq_n++);
    pkt->setTemporaryLeader(temporaryLeader);

    wsm->encapsulate(pkt);

    return wsm;
}

bool BaseProtocol::isDuplicated(const PlatooningBeacon* beacon)
{
    auto sequenceNumber = knownBeacons.find(beacon->getVehicleId());
    if (sequenceNumber == knownBeacons.end()) return false;
    if (beacon->getSequenceNumber() > sequenceNumber->second) return false;
    return true;
}

void BaseProtocol::handleMessage(cMessage* msg)
{
    if (msg->getArrivalGateId() >= minUpperId && msg->getArrivalGateId() <= maxUpperId)
        handleUpperMsg(msg);
    else if (msg->getArrivalGateId() >= minUpperControlId && msg->getArrivalGateId() <= maxUpperControlId)
        handleUpperControl(msg);
    else if (msg->getArrivalGateId() >= minRadioId && msg->getArrivalGateId() <= maxRadioId)
        handleLowerMsg(msg);
    else
        BaseApplLayer::handleMessage(msg);
}

void BaseProtocol::handleLowerMsg(cMessage* msg)
{
    BaseFrame1609_4* frame = check_and_cast<BaseFrame1609_4*>(msg);
    ASSERT2(frame, "received a frame not of type BaseFrame1609_4");

    cPacket* enc = frame->getEncapsulatedPacket();

    if (PlatooningBeacon* epkt = dynamic_cast<PlatooningBeacon*>(enc)) {

        if (positionHelper->getLeaderId() == epkt->getVehicleId()) {
            leaderFrames->frameReceived(msg->getArrivalGate()->getIndex(), epkt->getSequenceNumber(), epkt->getCreationTime().dbl(), simTime().dbl());
            if (checkLeaderInterfacesStatus && !checkLeaderInterfacesStatus->isScheduled())
                scheduleAt(simTime() + SimTime(50, SimTimeUnit::SIMTIME_MS), checkLeaderInterfacesStatus);

        }
        if (positionHelper->getFrontId() == epkt->getVehicleId()) {
            frontFrames->frameReceived(msg->getArrivalGate()->getIndex(), epkt->getSequenceNumber(), epkt->getCreationTime().dbl(), simTime().dbl());
            if (checkFrontInterfacesStatus && !checkFrontInterfacesStatus->isScheduled())
                scheduleAt(simTime() + SimTime(50, SimTimeUnit::SIMTIME_MS), checkFrontInterfacesStatus);
        }

        // if we're using multiple radios simultaneously, we might get duplicated beacons
        if (isDuplicated(epkt)) {
            duplicatedMessageReceived(epkt, frame);
            delete frame;
            return;
        }
        knownBeacons[epkt->getVehicleId()] = epkt->getSequenceNumber();

        // invoke messageReceived() method of subclass
        messageReceived(epkt, frame);

    }

    // find the application responsible for this beacon
    ApplicationMap::iterator app = apps.find(frame->getKind());
    if (app != apps.end() && app->second.size() != 0) {
        AppList applications = app->second;
        for (AppList::iterator i = applications.begin(); i != applications.end(); i++) {
            // send the message to the applications responsible for it
            send(frame->dup(), std::get<1>(*i));
        }
    }
    delete frame;
}

void BaseProtocol::receiveSignal(cComponent* src, simsignal_t id, bool value, cObject* details)
{
    if (id == lte_stack_phy_handover) {
        handoverIdOut.record(myId);
        handoverStartOut.record(value ? 0 : 1);
    }
    else {
        BaseApplLayer::receiveSignal(src, id, value, details);
    }
}

void BaseProtocol::receiveSignal(cComponent* src, simsignal_t id, long value, cObject* details)
{
    if (id == lte_stack_phy_handover) {
        handoverIdOut.record(myId);
        handoverStartOut.record(value ? 0 : 1);
    }
    else {
        BaseApplLayer::receiveSignal(src, id, value, details);
    }
}

void BaseProtocol::handleUpperMsg(cMessage* msg)
{
    PlexeInterfaceControlInfo* itf = dynamic_cast<PlexeInterfaceControlInfo*>(msg->getControlInfo());
    BaseFrame1609_4* frame = check_and_cast<BaseFrame1609_4*>(msg);

    enum PlexeRadioInterfaces interfaces;
    if (itf)
        interfaces = (enum PlexeRadioInterfaces) itf->getInterfaces();
    else
        interfaces = PlexeRadioInterfaces::VEINS_11P;

    sendTo(frame, interfaces);
}

void BaseProtocol::messageReceived(PlatooningBeacon* pkt, BaseFrame1609_4* frame)
{
}

void BaseProtocol::duplicatedMessageReceived(PlatooningBeacon* pkt, BaseFrame1609_4* frame)
{
}

void BaseProtocol::registerApplication(int applicationId, InputGate* appInputGate, OutputGate* appOutputGate, ControlInputGate* appControlInputGate, ControlOutputGate* appControlOutputGate)
{
    if (usedGates == MAX_GATES_COUNT) throw cRuntimeError("BaseProtocol: application with id=%d tried to register, but no space left", applicationId);
    // connect gates, if not already connected. a gate might be already
    // connected if an application is registering for multiple packet types
    cGate* upperIn;
    cGate* upperOut;
    cGate* upperCntIn;
    cGate* upperCntOut;
    if (!appInputGate->isConnected() || !appOutputGate->isConnected() || !appControlInputGate->isConnected() || !appControlOutputGate->isConnected()) {
        if (appInputGate->isConnected() || appOutputGate->isConnected() || appControlInputGate->isConnected() || appControlOutputGate->isConnected()) throw cRuntimeError("BaseProtocol: the application should not be connected but one of its gates is connected");
        upperOut = gate("upperLayerOut", usedGates);
        upperOut->connectTo(appInputGate);
        upperIn = gate("upperLayerIn", usedGates);
        appOutputGate->connectTo(upperIn);
        connections[appInputGate] = upperOut;
        connections[appOutputGate] = upperIn;
        upperCntOut = gate("upperControlOut", usedGates);
        upperCntOut->connectTo(appControlInputGate);
        upperCntIn = gate("upperControlIn", usedGates);
        appControlOutputGate->connectTo(upperCntIn);
        connections[appControlInputGate] = upperCntOut;
        connections[appControlOutputGate] = upperCntIn;
        usedGates++;
    }
    else {
        // find BaseProtocol gates already connected to the application
        GateConnections::iterator gate;
        gate = connections.find(appOutputGate);
        if (gate == connections.end()) throw cRuntimeError("BaseProtocol: gate should already be connected by not found in the connection list");
        upperIn = gate->second;
        gate = connections.find(appInputGate);
        if (gate == connections.end()) throw cRuntimeError("BaseProtocol: gate should already be connected by not found in the connection list");
        upperOut = gate->second;
        gate = connections.find(appControlOutputGate);
        if (gate == connections.end()) throw cRuntimeError("BaseProtocol: gate should already be connected by not found in the connection list");
        upperCntIn = gate->second;
        gate = connections.find(appControlInputGate);
        if (gate == connections.end()) throw cRuntimeError("BaseProtocol: gate should already be connected by not found in the connection list");
        upperCntOut = gate->second;
    }
    // save the mapping in the connection
    apps[applicationId].push_back(AppInOut(upperIn, upperOut, upperCntIn, upperCntOut));
}

} // namespace plexe
