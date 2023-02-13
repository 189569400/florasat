/*
 * PacketHandlerRouting.cc
 *
 *  Created on: Feb 08, 2022
 *      Author: Robin Ohs
 */

#include "PacketHandlerRouting.h"

namespace flora
{

    Define_Module(PacketHandlerRouting);

    void PacketHandlerRouting::initialize(int stage)
    {
        if (stage == 0)
        {
            maxHops = par("maxHops");
        }

        else if (stage == inet::INITSTAGE_APPLICATION_LAYER)
        {
            routing = check_and_cast<DirectedRouting *>(getSystemModule()->getSubmodule("routing"));
            if (routing == nullptr) {
                error("Error in PacketHandlerRouting::initialize(): Routing is nullptr.");
            }

            INorad* noradModule = check_and_cast<INorad *>(getParentModule()->getSubmodule("NoradModule"));
            if (NoradA *noradAModule = dynamic_cast<NoradA *>(noradModule))
            {
                satIndex = noradAModule->getSatelliteNumber();
            }
        }
    }

    void PacketHandlerRouting::handleMessage(cMessage *msg)
    {
        if(!msg->isSelfMessage())
        {
            // EV << "SAT [" << satIndex << "]: Msg arrived on " << msg->getArrivalGate() << endl;
        }
        auto pkt = check_and_cast<inet::Packet*>(msg);


        auto frame = pkt->removeAtFront<RoutingFrame>();
        int hops = frame->getNumHop();
        int sequenceNumber = frame->getSequenceNumber();
        pkt->insertAtFront(frame);

        if(hops > maxHops)
        {
            EV << "SAT [" << satIndex << "]: MSG expired. Delete." << endl;
            bubble("MSG expired.");
            delete msg;
            return;
        }

        insertSatinRoute(pkt);

        auto outputGate = routing->RoutePacket(pkt, getParentModule());

        switch (outputGate)
        {
        case ISL_DOWN:
        {
            cGate *downGate = gate("down1$o");
            if (downGate->getTransmissionChannel()->isBusy())
            {
                // EV << "SAT [" << satIndex << ", " << sequenceNumber << "]: Send down is busy." << endl;
                scheduleAt(downGate->getTransmissionChannel()->getTransmissionFinishTime(), pkt);
            }
            else
            {
                // EV << "SAT [" << satIndex << ", " << sequenceNumber << "]: Send down." << endl;
                send(pkt->dup(), downGate);
            }
            break;
        }
        case ISL_UP:
        {
            cGate *upGate = gate("up1$o");
            if (upGate->getTransmissionChannel()->isBusy())
            {
                // EV << "SAT [" << satIndex << ", " << sequenceNumber << "]: Send up is busy." << endl;
                scheduleAt(upGate->getTransmissionChannel()->getTransmissionFinishTime(), pkt);
            }
            else
            {
                // EV << "SAT [" << satIndex << ", " << sequenceNumber << "]: Send up." << endl;
                send(pkt->dup(), upGate);
            }
            break;
        }

        case ISL_LEFT:
        {
            cGate *leftGate = gate("left1$o");
            if (leftGate->getTransmissionChannel()->isBusy())
            {
                // EV << "SAT [" << satIndex << ", " << sequenceNumber << "]: Send left is busy." << endl;
                scheduleAt(leftGate->getTransmissionChannel()->getTransmissionFinishTime(), pkt);
            }
            else
            {
                // EV << "SAT [" << satIndex << ", " << sequenceNumber << "]: Send left." << endl;
                send(pkt->dup(), leftGate);
            }
            break;
        }
        case ISL_RIGHT:
        {
            cGate *rightGate = gate("right1$o");
            if (rightGate->getTransmissionChannel()->isBusy())
            {
                // EV << "SAT [" << satIndex << ", " << sequenceNumber << "]: Send right is busy." << endl;
                scheduleAt(rightGate->getTransmissionChannel()->getTransmissionFinishTime(), pkt);
            }
            else
            {
                // EV << "SAT [" << satIndex << ", " << sequenceNumber << "]: Send right." << endl;
                send(pkt->dup(), rightGate);
            }
            break;
        }
        case ISL_DOWNLINK:
        {
            cGate *downGate = gate("groundLink1$o");
            if (downGate->getTransmissionChannel()->isBusy())
            {
                // EV << "SAT [" << satIndex << ", " << sequenceNumber << "]: Send ground is busy." << endl;
                scheduleAt(downGate->getTransmissionChannel()->getTransmissionFinishTime(), pkt);
            }
            else
            {
                // EV << "SAT [" << satIndex << ", " << sequenceNumber << "]: Send ground." << endl;
                send(pkt->dup(), downGate);
            }
            break;
        }
        default:
            error("Unexpected gate");
        }
    }

    bool PacketHandlerRouting::isExpired(Packet *pkt)
    {
        auto frame = pkt->removeAtFront<RoutingFrame>();
        int hops = frame->getNumHop();
        pkt->insertAtFront(frame);
        return hops > maxHops;
    }

    void PacketHandlerRouting::insertSatinRoute(Packet *pkt)
    {
        auto frame = pkt->removeAtFront<RoutingFrame>();
        int numHops = frame->getNumHop();
        frame->setNumHop(numHops + 1);
        frame->appendRoute(satIndex);
        frame->appendTimestamps(simTime());
        pkt->insertAtFront(frame);
    }

}
