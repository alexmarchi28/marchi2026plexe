//
// Copyright (C) 2012-2021 Michele Segata <segata@ccs-labs.org>
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

#ifndef BASEPROTOCOL_H_
#define BASEPROTOCOL_H_

#include "veins/base/modules/BaseApplLayer.h"
#include "veins/modules/mobility/traci/TraCIMobility.h"
#include "veins/modules/messages/BaseFrame1609_4_m.h"

#include "plexe/messages/PlatooningBeacon_m.h"
#include "plexe/mobility/CommandInterface.h"
#include "plexe/utilities/BasePositionHelper.h"

#include "plexe/driver/PlexeRadioDriverInterface.h"

#include <memory>
#include <tuple>

// maximum number of upper layer apps that can connect (see .ned file)
#define MAX_GATES_COUNT 10

namespace plexe {

using veins::BaseFrame1609_4;

#define N_INTERFACES 3
struct frame_t {
    bool received[N_INTERFACES];
    double delay[N_INTERFACES];
    double receiveTime[N_INTERFACES];
    int seqNr;
};
typedef struct frame_t Frame;

class FramesRingBuffer {
public:
    FramesRingBuffer(int size)
        : frames(nullptr)
        , size(size+1)
        , head(size)
        , empty(true)
        , occupied(0)
    {
        frames = new Frame[this->size];
    }
    ~FramesRingBuffer()
    {
        delete frames;
        frames = nullptr;
    }
    static Frame emptyFrame()
    {
        Frame f = Frame();
        for (int i = 0; i < N_INTERFACES; i++) {
            f.received[i] = false;
            f.delay[i] = 0;
            f.receiveTime[i] = 0;
        }
        f.seqNr = -1;
        return f;
    }
    void frameReceived(int interface, int seqNr, double generationTime, double receiveTime)
    {
        // first frame in the ring buffer, initialize some stuff
        if (empty) {
            insertNewFrame(seqNr);
            updateHeadFrame(interface, generationTime, receiveTime);
            empty = false;
            return;
        }
        // an already known frame received from a different interfaces
        if (frames[head].seqNr == seqNr) {
            updateHeadFrame(interface, generationTime, receiveTime);
            return;
        }
        // new frame
        if (frames[head].seqNr < seqNr) {
            // in case frames are missing completely (not being received from any interface), just add them
            for (int i = frames[head].seqNr+1; i <= seqNr; i++) insertNewFrame(i);
            // for the frame that has just been received, set from which interface it has been received
            updateHeadFrame(interface, generationTime, receiveTime);
        }
        // the remaining case is when the sequence number is old
        // this can happen if the frame has so much delay that it has been received after a new one
        // this should be unlikely, but in case of VLC with re-propagation and a large number of vehicles in the platoon, this might occur
        if (frames[head].seqNr > seqNr) {
            // do we still have this frame in the buffer?
            if (frames[head].seqNr - seqNr < occupied) {
                int position = (head + occupied - (frames[head].seqNr - seqNr)) % occupied;
                updateFrame(position, interface, generationTime, receiveTime);
            }
        }
    }
    int getIndex(int index)
    {
        // 0 === head
        // size-1 === head - size + 1
        return (head - index + occupied) % occupied;
    }
    void getStats(double fer[N_INTERFACES], double delays[N_INTERFACES], double interarrivals[N_INTERFACES], bool ignoreLast = false)
    {
        int received;
        double delay;
        double interarrival;
        double lastReceiveTime;
        bool first;
        // if the buffer is not yet full, compute stats on all samples even if ignoreLast is set
        int firstFrame, lastFrame;
        if (occupied < size) {
            firstFrame = 0;
            lastFrame = occupied;
        }
        else {
            firstFrame = ignoreLast ? 1 : 0;
            lastFrame = ignoreLast ? occupied : occupied - 1;
        }
        for (int i = 0; i < N_INTERFACES; i++) {
            received = 0;
            delay = 0;
            interarrival = 0;
            first = true;
            for (int n = firstFrame; n < lastFrame; n++) {
                Frame* f = &(frames[getIndex(n)]);
                if (f->received[i]) {
                    // count a received frame for the fer
                    received++;
                    // sum the delay for the average delay
                    delay += f->delay[i];
                    // sum the interarrival for the average interarrivals
                    if (!first) interarrival += lastReceiveTime - f->receiveTime[i];
                    else first = false;
                    lastReceiveTime = f->receiveTime[i];
                }
            }
            fer[i] = ((double)(getNFrames() - received)) / getNFrames();
            if (received > 0) delays[i] = delay / received;
            else delays[i] = -1;
            if (received > 1) interarrivals[i] = interarrival / (received-1);
            else interarrivals[i] = -1;
        }
    }
    int getNFrames()
    {
        return occupied < size ? occupied : occupied - 1;
    }

private:
    Frame* frames;
    int size;
    // pointer to the most recent received frame
    int head;
    bool empty;
    int occupied;

    void insertNewFrame(int seqNr)
    {
        head = (head + 1) % size;
        Frame* f = &frames[head];
        for (int i = 0; i < N_INTERFACES; i++) {
            f->received[i] = false;
            f->delay[i] = 0;
            f->receiveTime[i] = 0;
        }
        f->seqNr = seqNr;
        occupied = std::min(occupied + 1, size);
    }
    void updateHeadFrame(int interface, double generationTime, double receiveTime)
    {
        updateFrame(head, interface, generationTime, receiveTime);
    }
    void updateFrame(int position, int interface, double generationTime, double receiveTime)
    {
        frames[position].received[interface] = true;
        frames[position].delay[interface] = receiveTime - generationTime;
        frames[position].receiveTime[interface] = receiveTime;
    }
};

class InterfaceMonitor {

public:
    enum InterfaceStatus {
        ACTIVE,
        FAILED
    };
    enum CheckStatusResult {
        SIGNAL_FAILURE,
        SIGNAL_RECOVERY,
        NO_CHANGE
    };

    InterfaceMonitor(FramesRingBuffer* buffer, int interface, double pdrThreshold, double deltaT)
        : status(ACTIVE)
        , ringBuffer(buffer)
        , interface(interface)
        , pdrThreshold(pdrThreshold)
        , deltaT(deltaT)
        , recovered(false)
    {

    }

    enum CheckStatusResult checkStatus(double currentTime)
    {
        double fer[N_INTERFACES];
        double delays[N_INTERFACES];
        double interarrivals[N_INTERFACES];
        ringBuffer->getStats(fer, delays, interarrivals, false);
        bool pdrBelowThreshold = (1 - fer[interface] < pdrThreshold);
        enum CheckStatusResult statusResult = NO_CHANGE;
        // we are in an active state and the PDR is below threshold
        if (status == ACTIVE && pdrBelowThreshold) {
            status = FAILED;
            recovered = false;
            statusResult = SIGNAL_FAILURE;
        }
        // we are in a failed state and the PDR is below threshold
        else if (status == FAILED && pdrBelowThreshold) {
            recovered = false;
        }
        // we are in a failed state and the PDR is above threshold
        else if (status == FAILED && !pdrBelowThreshold) {
            // if this is the first time the PDR goes above threshold ...
            if (!recovered) {
                // ... keep track of when the interface first recovers
                timeOfRecovery = currentTime;
                recovered = true;
            }
            // if the PDR is above the threshold for enough time ...
            if (currentTime - timeOfRecovery >= deltaT) {
                // we can signal the recovery and change state to active
                status = ACTIVE;
                statusResult = SIGNAL_RECOVERY;
            }
            // otherwise we remain in the failed state
        }
        return statusResult;
    }

private:

    enum InterfaceStatus status;
    FramesRingBuffer* ringBuffer;
    int interface;
    double pdrThreshold;
    double deltaT;
    double timeOfRecovery;
    bool recovered;

};

class BaseProtocol : public veins::BaseApplLayer {

private:

    // map of radio interfaces from radio ids
    std::map<int, cGate*> radioOuts;

    // map of known beacons (vehicle id, sequence number)
    std::map<int, int> knownBeacons;

    // indicates whether a beacon has already been received or not
    bool isDuplicated(const PlatooningBeacon* beacon);

protected:
    // determines position and role of each vehicle
    BasePositionHelper* positionHelper;

    // id of this vehicle
    int myId;
    // sequence number of sent messages
    int seq_n;
    // vehicle length
    double length;

    // beaconing interval (i.e., update frequency)
    SimTime beaconingInterval;
    // priority used for messages (i.e., the access category)
    int priority;
    // packet size of the platooning message
    int packetSize;

    // input/output gates from/to upper layer
    int upperControlIn, upperControlOut, lowerLayerIn, lowerLayerOut;
    // id range of input gates from upper layer
    int minUpperId, maxUpperId, minUpperControlId, maxUpperControlId;
    // id range of lower radio gates
    int minRadioId, maxRadioId;

    // registered upper layer applications. this is a mapping between
    // beacon id inside packets coming from upper layer and the gate they
    // the application is connected to. convention: id, from app, to app
    typedef cGate OutputGate;
    typedef cGate InputGate;
    typedef cGate ControlInputGate;
    typedef cGate ControlOutputGate;
    typedef std::tuple<InputGate*, OutputGate*, ControlInputGate*, ControlOutputGate*> AppInOut;
    typedef std::vector<AppInOut> AppList;
    typedef std::map<int, AppList> ApplicationMap;
    ApplicationMap apps;
    // number of gates from the array used
    int usedGates;
    // maps of already existing connections
    typedef cGate ThisGate;
    typedef cGate OtherGate;
    typedef std::map<OtherGate*, ThisGate*> GateConnections;
    GateConnections connections;

    // messages for scheduleAt
    cMessage* sendBeacon;
    cMessage* recordData;

    int frameStatsWindow;
    // storage for stats about received beacons
    FramesRingBuffer* leaderFrames;
    FramesRingBuffer* frontFrames;
    InterfaceMonitor* leaderMonitors[N_INTERFACES];
    InterfaceMonitor* frontMonitors[N_INTERFACES];
    // amount of time required to declare an interface as recovered
    double deltaT;
    // PDR threshold use to discriminate between an active and failed interface
    double pdr11p;
    double pdrCV2X;
    double pdrVLC;
    // set vehicle as temporary leader in beacons
    bool temporaryLeader;

    // period message used to check for the status of the interfaces
    cMessage* checkLeaderInterfacesStatus;
    cMessage* checkFrontInterfacesStatus;

    // frame error rates for leader and front vehicle
    cOutVector statsIdOut;
    cOutVector leaderFer11pOut, leaderFerVLCOut, leaderFerLTEOut;
    cOutVector frontFer11pOut, frontFerVLCOut, frontFerLTEOut;
    // delay for leader and front vehicle
    cOutVector leaderDelay11pOut, leaderDelayVLCOut, leaderDelayLTEOut;
    cOutVector frontDelay11pOut, frontDelayVLCOut, frontDelayLTEOut;
    // interarrival for leader and front vehicle
    cOutVector leaderInterarrival11pOut, leaderInterarrivalVLCOut, leaderInterarrivalLTEOut;
    cOutVector frontInterarrival11pOut, frontInterarrivalVLCOut, frontInterarrivalLTEOut;

    cOutVector handoverIdOut, handoverStartOut;

    /**
     * NB: this method must be overridden by inheriting classes, BUT THEY MUST invoke the super class
     * method prior processing the message. For example, the start communication event is handled by the
     * BaseProtocol which then calls the startCommunications method. Also statistics are handled
     * by BaseProtocol and are recorder periodically.
     */
    virtual void handleSelfMsg(cMessage* msg) override;

    // TODO: implement method and pass info to upper layer (bogus platooning) as it is (msg)
    virtual void handleLowerMsg(cMessage* msg) override;

    // handle messages coming from above layers
    virtual void handleUpperMsg(cMessage* msg) override;

    // override handleMessage to manager upper layer gate array
    virtual void handleMessage(cMessage* msg) override;

    virtual void sendTo(BaseFrame1609_4* frame, enum PlexeRadioInterfaces interfaces);

    /**
     * Sends a platooning message with all information about the car. This is an utility function for
     * subclasses
     */
    void sendPlatooningMessage(int destinationAddress, enum PlexeRadioInterfaces interfaces = PlexeRadioInterfaces::ALL);

    virtual std::unique_ptr<BaseFrame1609_4> createBeacon(int destinationAddress);

    /**
     * This method must be overridden by subclasses to take decisions
     * about what to do.
     * Passed packet MUST NOT be freed, but just be read. Freeing is a duty of the
     * superclass
     *
     * \param pkt the platooning beacon
     * \param frame the original frame which was containing pkt
     */
    virtual void messageReceived(PlatooningBeacon* pkt, BaseFrame1609_4* frame);

    /**
     * This method must be overridden by subclasses to take decisions
     * about what to do.
     * Passed packet MUST NOT be freed, but just be read. Freeing is a duty of the
     * superclass
     * Differently from the messageReceived method, this method is invoked for frames that has already been received.
     * This can happen, for example, when using multiple communication technologies or redundancy
     *
     * \param pkt the platooning beacon
     * \param frame the original frame which was containing pkt
     */
    virtual void duplicatedMessageReceived(PlatooningBeacon* pkt, BaseFrame1609_4* frame);

    /**
     * These methods signal changes in channel busy status to subclasses
     * or occurrences of collisions.
     * Subclasses which are interested should ovverride these methods.
     */
    virtual void channelBusyStart()
    {
    }
    virtual void channelIdleStart()
    {
    }
    virtual void collision()
    {
    }

    // traci mobility. used for getting/setting info about the car
    veins::TraCIMobility* mobility;
    veins::TraCICommandInterface* traci;
    veins::TraCICommandInterface::Vehicle* traciVehicle;
    traci::CommandInterface* plexeTraci;
    std::unique_ptr<traci::CommandInterface::Vehicle> plexeTraciVehicle;

    simsignal_t lte_stack_phy_handover;

    virtual void receiveSignal(cComponent* src, simsignal_t id, long value, cObject* details) override;

public:
    // id for beacon message
    static const int BEACON_TYPE;

    BaseProtocol()
    {
        sendBeacon = nullptr;
        recordData = nullptr;
        usedGates = 0;
        leaderFrames = nullptr;
        frontFrames = nullptr;
        checkLeaderInterfacesStatus = nullptr;
        checkFrontInterfacesStatus = nullptr;
        temporaryLeader = false;
    }
    virtual ~BaseProtocol();

    virtual void initialize(int stage) override;

    // register a higher level application by its id
    void registerApplication(int applicationId, InputGate* appInputGate, OutputGate* appOutputGate, ControlInputGate* appControlInputGate, ControlOutputGate* appControlOutputGate);

    // signal to registered applications when an interface fails or recovers (and which one)
    static const simsignal_t sigInterfaceFailure;
    static const simsignal_t sigInterfaceRecovery;

    void setTemporaryLeader(bool tempLeader);
};

} // namespace plexe

#endif /* BASEPROTOCOL_H_ */
