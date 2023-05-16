#ifndef _DTN_H_
#define _DTN_H_

#include <dtn/node/dtn/ContactPlan.h>
#include <dtn/node/dtn/ContactHistory.h>
#include <dtn/node/dtn/CustodyModel.h>
#include <dtn/node/dtn/SdrModel.h>
#include <routing/dtn/Routing.h>
#include <routing/dtn/RoutingCgrModel350.h>
#include <routing/dtn/RoutingCgrModelRev17.h>
#include <routing/dtn/RoutingCgrModelYen.h>
#include <routing/dtn/RoutingDirect.h>
#include <routing/dtn/RoutingEpidemic.h>
#include <routing/dtn/RoutingSprayAndWait.h>
#include <routing/dtn/RoutingPRoPHET.h>
#include <routing/dtn/RoutingCgrModel350_Probabilistic.h>
#include <routing/dtn/RoutingOpportunistic.h>
#include <routing/dtn/RoutingUncertainUniboCgr.h>
#include <routing/dtn/RoutingBRUF1T.h>
#include <routing/dtn/RoutingORUCOP.h>
#include <routing/dtn/brufncopies/RoutingBRUFNCopies.h>
#include <routing/dtn/cgrbrufpowered/CGRBRUFPowered.h>
#include <cstdio>
#include <string>
#include <omnetpp.h>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <queue>

#include "dtn/node/MsgTypes.h"
#include "dtn/Config.h"
#include "dtn/dtnsim_m.h"

#include "dtn/node/graphics/Graphics.h"
#include "routing/dtn/Routing.h"
#include "dtn/utils/RouterUtils.h"
#include "dtn/utils/TopologyUtils.h"
#include "dtn/utils/RouterUtils.h"
#include "dtn/utils/ContactPlanUtils.h"
#include "dtn/utils/Observer.h"
#include "dtn/utils/MetricCollector.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "routing/dtn/RoutingCgrModel350_2Copies.h"
#include "routing/dtn/RoutingCgrModel350_Hops.h"

using namespace omnetpp;
using namespace std;



class Dtn: public cSimpleModule, public Observer
{
public:
	Dtn();
	virtual ~Dtn();

	virtual void setOnFault(bool onFault);
	virtual void refreshForwarding();
	ContactPlan * getContactPlanPointer();
	virtual void setContactPlan(ContactPlan &contactPlan);
	virtual void setContactTopology(ContactPlan &contactTopology);
	virtual void setMetricCollector(MetricCollector* metricCollector);
	virtual Routing * getRouting();

	virtual void update(void);

	//Opportunistic procedures
	void syncDiscoveredContact(Contact* c, bool start);
	void syncDiscoveredContactFromNeighbor(Contact* c, bool start, int ownEid, int neighborEid);
	void scheduleDiscoveredContactStart(Contact* c);
	void scheduleDiscoveredContactEnd(Contact* c);
	ContactHistory* getContactHistory();
	void addDiscoveredContact(Contact c);
	void removeDiscoveredContact(Contact c);
	void predictAllContacts(double currentTime);
	void coordinateContactStart(Contact* c);
	void coordinateContactEnd(Contact* c);
	void notifyNeighborsAboutDiscoveredContact(Contact* c, bool start, map<int,int>* alreadyInformed);
	void updateDiscoveredContacts(Contact* c);
	map<int,int> getReachableNodes();
	void addCurrentNeighbor(int neighborEid);
	void removeCurrentNeighbor(int neighborEid);
	int checkExistenceOfContact(int sourceEid, int destinationEid, int start);


protected:
	virtual void initialize(int stage);
	virtual int numInitStages() const;
	virtual void handleMessage(cMessage *msg);
	virtual void finish();

	virtual void dispatchBundle(BundlePkt *bundle);

private:

	int eid_;
	bool onFault = false;

	// Pointer to grahics module
	Graphics *graphicsModule;

	// Forwarding threads
	map<int, ForwardingMsgStart *> forwardingMsgs_;

	// Routing and storage
	Routing * routing;

	// Contact Plan to feed CGR
	// and get transmission rates
	ContactPlan contactPlan_;

	// Contact History used to collect all
	// discovered contacts;
	ContactHistory contactHistory_;

	//An observer that collects and evaluates all the necessary simulation metrics
	MetricCollector* metricCollector_;

	// Contact Topology to schedule Contacts
	// and get transmission rates
	ContactPlan contactTopology_;

	CustodyModel custodyModel_;
	double custodyTimeout_;

	SdrModel sdr_;

	// BundlesMap
	bool saveBundleMap_;
	ofstream bundleMap_;

	// Signals
	simsignal_t dtnBundleSentToCom;
	simsignal_t dtnBundleSentToApp;
	simsignal_t dtnBundleSentToAppHopCount;
	simsignal_t dtnBundleSentToAppRevisitedHops;
	simsignal_t dtnBundleReceivedFromCom;
	simsignal_t dtnBundleReceivedFromApp;
	simsignal_t dtnBundleReRouted;
	simsignal_t sdrBundleStored;
	simsignal_t sdrBytesStored;
	simsignal_t routeCgrDijkstraCalls;
	simsignal_t routeCgrDijkstraLoops;
	simsignal_t routeCgrRouteTableEntriesCreated;
	simsignal_t routeCgrRouteTableEntriesExplored;
};

#endif /* DTN_H_ */


