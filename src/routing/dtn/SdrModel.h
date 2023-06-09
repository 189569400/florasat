/*
 * SdrModel.h
 *
 *  Created on: Nov 25, 2016
 *      Author: juanfraire
 */

#ifndef __FLORA_ROUTING_SDRMODEL_H
#define __FLORA_ROUTING_SDRMODEL_H

#include <routing/dtn/contactplan/ContactPlan.h>
#include <routing/dtn/SdrStatus.h>
#include <map>
#include <omnetpp.h>
#include "routing/dtn/utils/Subject.h"

#include "routing/DtnRoutingHeader_m.h"
#include "assert.h"

namespace flora {
namespace routing {

class SdrModel: public Subject {

public:
	SdrModel();
	virtual ~SdrModel();

	// Initialization and configuration
	virtual void setEid(int eid);
	virtual void setNodesNumber(int nodesNumber);
	virtual void setContactPlan(ContactPlan *contactPlan);
	virtual void setSize(int size);
	virtual void freeSdr(int eid);

	// Get information
	virtual int getBundlesCountInSdr();
	virtual int getBundlesCountInContact(int cid);
	virtual int getBundlesCountInLimbo();
	virtual list<DtnRoutingHeader*> * getBundlesInLimbo();
	virtual int getBytesStoredInSdr();
	virtual int getBytesStoredToNeighbor(int eid);
	virtual vector<int> getBundleSizesStoredToNeighbor(int eid);
	virtual vector<int> getBundleSizesStoredToNeighborWithHigherPriority(int eid, bool critical);
	virtual SdrStatus getSdrStatus();
	virtual DtnRoutingHeader *getEnqueuedBundle(long bundleId);
	bool isSdrFreeSpace(int sizeNewPacket);

	// Enqueue and dequeue from perContactBundleQueue_
	virtual bool enqueueBundleToContact(DtnRoutingHeader *bundle, int contactId);
	virtual bool isBundleForContact(int contactId);
	virtual DtnRoutingHeader *getNextBundleForContact(int contactId);
	virtual void popNextBundleForContact(int contactId);

	// Enqueue and dequeue from genericBundleQueue_
	virtual bool enqueueBundle(DtnRoutingHeader *bundle);
	virtual void removeBundle(long bundleId);
	virtual list<DtnRoutingHeader*> getCarryingBundles();

	// Enqueue and dequeue from transmittedBundlesInCustody_
	virtual bool enqueueTransmittedBundleInCustody(DtnRoutingHeader *bundle);
	virtual void removeTransmittedBundleInCustody(long bundleId);
	virtual DtnRoutingHeader *getTransmittedBundleInCustody(long bundleId);
	virtual list<DtnRoutingHeader*> getTransmittedBundlesInCustody();

private:

	int size_;  		// Capacity of sdr in bytes
	int eid_;  			// Local eid of the node
	int nodesNumber_;	// Number of nodes in the network
	int bytesStored_;	// Total Bytes stored in Sdr
	int bundlesNumber_;	// Total bundles enqueued in all sdr queues (index, generic, in custody)

	ContactPlan *contactPlan_;

	// Indexed queues where index can be used by routing algorithms
	// to enqueue bundles to specific contacts or nodes. When there
	// is no need for an indexed queue, a generic one can be used instead
	map<int, list<DtnRoutingHeader*> > perContactBundleQueue_;
	map<int, list<DtnRoutingHeader*> > perNodeBundleQueue_;
	list<DtnRoutingHeader*> genericBundleQueue_;

	// A separate area of memory to store transmitted bundles for which
	// the current node is custodian. Bundles are removed as custody reports
	// arrives with either custody acceptance or rejection of a remote node
	list<DtnRoutingHeader*> transmittedBundlesInCustody_;

};

}  // namespace routing
}  // namespace flora

#endif /* __FLORA_ROUTING_SDRMODEL_H */
