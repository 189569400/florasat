#ifndef __LORAMAC_H
#define __LORAMAC_H

#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"
#include "inet/linklayer/contract/IMacProtocol.h"
#include "inet/linklayer/base/MacProtocolBase.h"
#include "inet/common/FSMA.h"
#include "inet/queueing/contract/IPacketQueue.h"
#include "LoRaMacControlInfo_m.h"
#include "LoRaMacFrame_m.h"
#include "inet/common/Protocol.h"
#include <list>

#include "LoRaRadio.h"

namespace flora {

/**
 * Based on CSMA class
 */

class LoRaMac : public MacProtocolBase
{
  private:
    // Signal
    simsignal_t macDPAD;
    simsignal_t macDPADOwnTraffic;

  protected:
    /**
     * @name Configuration parameters
     */
    //@{
    MacAddress address;
    omnetpp::SimTime bdw;
    omnetpp::SimTime DPAD;
    bool useAck = true;
    double bitrate = NaN;
    int headerLength = -1;
    int ackLength = -1;

    simtime_t ackTimeout = -1;
    simtime_t slotTime = -1;
    simtime_t sifsTime = -1;
    simtime_t difsTime = -1;
    simtime_t waitDelay1Time = -1;
    simtime_t listening1Time = -1;
    simtime_t waitDelay2Time = -1;
    simtime_t listening2Time = -1;

    int maxQueueSize = -1;
    int retryLimit = -1;
    int cwMin = -1;
    int cwMax = -1;
    int cwMulticast = -1;
    int sequenceNumber = 0;

    bool isClassA = true;
    bool isClassB = false;
    bool isClassS = false;
    bool iGotBeacon = false;
    bool beaconGuard = false;

    simtime_t beaconStart = 1;
    simtime_t beaconGuardTime = -1;
    simtime_t beaconReservedTime = -1;
    simtime_t beaconPeriodTime = -1;

    simtime_t timeToNextSlot = -1;
    simtime_t classBslotTime = -1;
    int pingOffset = -1;

    simtime_t maxToA = -1;
    simtime_t clockThreshold = -1;
    simtime_t classSslotTime = -1;
    int targetClassSslot = 1;
    int maxClassSslots = 1;

    cOutVector slotSelectionData;
    //cOutVector slotBeginTimes;

    // FSA Game parameters
    double a = 1;
    int realNodeNumber = 0;
    bool FSAGame = false;

    std::list<double> estimations{6.720365127118711,
        23.623166287474454,
        42.205110250618176,
        63.438887867445075,
        80.26383930492202,
        101.28536977083522,
        119.96689034983144,
        140.3690703297854,
        161.02672421808114,
        181.93881452590463,
        204.08868345648122,
        224.3469225876996,
        244.05365483755105,
        266.29803458969064,
        283.4023606542818,
        305.17142587226033,
        323.98347466595965,
        341.29442947235253,
        365.09858449195656,
        384.8839206613554,
        404.3060780292568,
        425.71972858947044,
        440.13715200615917,
        466.9577275677278,
        482.90605214950176,
        502.22062303625194,
        521.1685421613557,
        537.5611233564039,
        557.6960060124214,
        584.3609476288322,
        602.6128762408812,
        620.2978668729891,
        638.9342398002582,
        666.7377798389646,
        684.5004697970262,
        712.8515115330617,
        721.9370705732664,
        742.7243451928766,
        768.3481965800929,
        781.1430211260637,
        800.3113189169366,
        821.3604254292497,
        854.3318202971965,
        868.0956648713773,
        881.6674818568273,
        900.4022230731279,
        924.2939837292506,
        942.3374468449671,
        959.6953207490056,
        983.3965248476301,
        994.721398359928};

    //@}


    /**
     * @name CsmaCaMac state variables
     * Various state information checked and modified according to the state machine.
     */
    //@{
    enum State {
        IDLE,
        TRANSMIT,
        BEACON_RECEPTION,
        WAIT_DELAY,
        PING_SLOT,
        RECEIVING_BEACON,
        RECEIVING,
        WAIT_DELAY_1,
        LISTENING_1,
        RECEIVING_1,
        WAIT_DELAY_2,
        LISTENING_2,
        RECEIVING_2,
        UPLINK_SLOT,
    };

    IRadio *radio = nullptr;
    IRadio::TransmissionState transmissionState = IRadio::TRANSMISSION_STATE_UNDEFINED;
    IRadio::ReceptionState receptionState = IRadio::RECEPTION_STATE_UNDEFINED;

    cFSM fsm;

    /** Remaining backoff period in seconds */
    simtime_t backoffPeriod = -1;

    /** Number of frame retransmission attempts. */
    int retryCounter = -1;

    /** Messages received from upper layer and to be transmitted later */
    cPacketQueue transmissionQueue;

    /** Passive queue module to request messages from */
    cPacketQueue *queueModule = nullptr;
    //@}

    /** @name Timer messages */
    //@{
    /** End of the Short Inter-Frame Time period */
    cMessage *endSifs = nullptr;

    /** End of the Data Inter-Frame Time period */
    cMessage *endDifs = nullptr;

    /** End of the backoff period */
    cMessage *endBackoff = nullptr;

    /** End of the ack timeout */
    cMessage *endAckTimeout = nullptr;

    /** Timeout after the transmission of a Data frame */
    cMessage *endTransmission = nullptr;

    /** Timeout after the reception of a Data frame */
    cMessage *endReception = nullptr;

    /** Timeout after the reception of a Data frame */
    cMessage *droppedPacket = nullptr;

    /** End of the Delay_1 */
    cMessage *endDelay_1 = nullptr;

    /** End of the Listening_1 */
    cMessage *endListening_1 = nullptr;

    /** End of the Delay_2 */
    cMessage *endDelay_2 = nullptr;

    /** End of the Listening_2 */
    cMessage *endListening_2 = nullptr;

    /** Radio state change self message. Currently this is optimized away and sent directly */
    cMessage *mediumStateChange = nullptr;

    /** End of the ping slot period. Start new downlink listening slot */
    cMessage *pingPeriod = nullptr;

    /** End of downlink listening slot */
    cMessage *endPingSlot = nullptr;

    /** End of the beacon period. Start new beacon listening slot */
    cMessage *beaconPeriod = nullptr;

    /** End of the beacon listening slot */
    cMessage *beaconReservedEnd = nullptr;

    /** Start of the beacon guard period */
    cMessage *beaconGuardStart = nullptr;

    /** End of the beacon guard period */
    cMessage *beaconGuardEnd = nullptr;

    /** Start of uplink transmission slot */
    cMessage *beginTXslot= nullptr;
    //@}

    /** @name Statistics */
    //@{
    long numRetry;
    long numSentWithoutRetry;
    long numGivenUp;
    long numCollision;
    long numSent;
    long numReceived;
    long numSentBroadcast;
    long numReceivedBroadcast;
    long numReceivedBeacons;
    //@}

    const char beaconReceivedText[17] = "Beacon received!";


  public:
    /**
     * @name Construction functions
     */
    //@{
    virtual ~LoRaMac();
    //@}
    virtual MacAddress getAddress();

  protected:
    /**
     * @name Initialization functions
     */
    //@{
    /** @brief Initialization of the module and its variables */
    virtual void initialize(int stage) override;
    virtual void finish() override;
    virtual void configureNetworkInterface() override;
    //@}

    /**
     * @name Message handing functions
     * @brief Functions called from other classes to notify about state changes and to handle messages.
     */
    //@{
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void handleUpperMessage(cMessage *msg) override;
    virtual void handleLowerMessage(cMessage *msg) override;
    virtual void handleWithFsm(cMessage *msg);

    virtual void receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details) override;

    virtual Packet *encapsulate(Packet *msg);
    virtual Packet *decapsulate(Packet *frame);
    //@}


    /**
     * @name Frame transmission functions
     */
    //@{
    virtual void sendDataFrame(Packet *frameToSend);
    virtual void sendAckFrame();
    //virtual void sendJoinFrame();
    //@}

    /**
     * @name Utility functions
     */
    //@{
    virtual void finishCurrentTransmission();
    virtual Packet *getCurrentTransmission();

    virtual bool isReceiving();
    virtual bool isAck(const Ptr<const LoRaMacFrame> &frame);
    virtual bool isBroadcast(const Ptr<const LoRaMacFrame> & msg);
    virtual bool isForUs(const Ptr<const LoRaMacFrame> &msg);

    void turnOnReceiver(void);
    void turnOffReceiver(void);

    virtual void calculatePingPeriod(const Ptr<const LoRaMacFrame> &frame);
    virtual void schedulePingPeriod();
    virtual int aesEncrypt(unsigned char *message, int message_len, unsigned char *key, unsigned char *cipher);

    virtual bool isBeacon(const Ptr<const LoRaMacFrame> &frame);
    virtual bool isDownlink(const Ptr<const LoRaMacFrame> &frame);

    virtual bool timeToTrasmit();
    virtual void scheduleULslots();

    virtual void beaconScheduling();
    virtual void increaseBeaconTime();
    //virtual Packet *getCurrentReception();

    //@}
};

} // namespace inet

#endif // ifndef __LORAMAC_H
