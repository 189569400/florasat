/*
 * CustodyModel.h
 *
 *  Created on: Dec 5, 2017
 *      Author: juanfraire
 */

#ifndef __FLORA_ROUTING_CUSTODYMODEL_H
#define __FLORA_ROUTING_CUSTODYMODEL_H

#include "routing/dtn/SdrModel.h"
#include "routing/dtn/MsgTypes.h"
#include <omnetpp.h>

using namespace omnetpp;

namespace flora {
namespace routing {

class CustodyModel {
    public:
        CustodyModel();
        virtual ~CustodyModel();

        // Initialization and configuration
        void setEid(int eid);
        void setSdr(SdrModel * sdr);
        void setCustodyReportByteSize(int custodyReportByteSize);

        // Events from Dtn layer
        DtnRoutingHeader * bundleWithCustodyRequestedArrived(DtnRoutingHeader * bundle);
        DtnRoutingHeader * custodyReportArrived(DtnRoutingHeader * bundle);
        DtnRoutingHeader * custodyTimerExpired(CustodyTimeout * custodyTimeout);

        void printBundlesInCustody(void);

    private:

        DtnRoutingHeader * getNewCustodyReport(bool accept, DtnRoutingHeader * bundle);

        int eid_;
        SdrModel * sdr_;

        int custodyReportByteSize_;
};

}  // namespace routing
}  // namespace flora

#endif /* __FLORA_ROUTING_CUSTODYMODEL_H */
