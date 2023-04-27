#ifndef __CENTRAL_H_
#define __CENTRAL_H_

#include "dtn/node/dtn/ContactPlan.h"
#include "dtn/node/dtn/Dtn.h"
#include "dtn/node/com/Com.h"
#include <omnetpp.h>
#include <string>
#include <iostream>
#include <map>
#include <utility>
#include <limits>
#include <vector>

#include "dtn/utils/Lp.h"
#include "dtn/utils/TopologyUtils.h"
#include "dtn/utils/RouterUtils.h"
#include "dtn/utils/LpUtils.h"
#include "dtn/utils/MetricCollector.h"
#include "dtn/node/app/App.h"

#include "routing/dtn/RoutingCgrModel350.h"
#include "dtn/node/dtn/SdrModel.h"

#include "dtn/node/MsgTypes.h"
#include "dtn/dtnsim_m.h"

using namespace omnetpp;
using namespace std;


namespace dtnsim
{

class Central: public cSimpleModule
{
public:

	Central();
	virtual ~Central();
	void finish();
	virtual void initialize();
	virtual void handleMessage(cMessage * msg);

private:

	/// @brief Fill flowIds_ structure with App traffic data generators
	void computeFlowIds();

	/// @brief Compute Topology from  contactPlan_ and save it
	/// in dot and pdf files inside "results" folder
	void saveTopology();

	/// @brief Compute Flows from BundleMaps files and save them
	/// in dot and pdf files inside "results" folder
	void saveFlows();

	void saveLpFlows();

	map<int, map<int, map<int, double > > > getTraffics();

	double getState(double trafficStart);

	/// @brief delete contacts from contactPlan_
	/// @param[in] contactsToDelete are contact ids
	/// @param[in] faultsAware specifies if the contact is erased or simply set to deleted
	void deleteContacts(vector<int> contactsToDelete, bool faultsAware);

	/// @brief get nContacts randomly
	/// @return vector of contact ids
	vector<int> getRandomContactIds(int nContacts);

	/// @brief get contacts randomly. Any contact is chosen with probability failureProbability.
	/// @return vector of contact ids
	vector<int> getRandomContactIdsWithFProb(double failureProbability);

	/// @brief goes to the available contacts from the network topology and chooses their id, in case they have to be deleted
	/// @return the ids of the chosen contacts to be deleted
	vector<int> getContactIdsWithSpecificFProb();

	/// @brief get nContacts by iteratively erasing contacts
	/// with highest centrality
	/// @return vector of contact ids
	vector<int> getCentralityContactIds(int nContacts, int nodesNumber);
	int computeNumberOfRoutesThroughContact(int contactId, vector<CgrRoute> shortestPaths);
	set<int> getAffectedContacts(vector<CgrRoute> shortestPaths);

	/// @brief compute total routes from all to all nodes
	/// nodePairsRoutes is returned as output, it will contain an element i->j if there is at least a route between i and j
	int computeTotalRoutesNumber(ContactPlan &contactPlan, int nodesNumber, set<pair<int, int> > &nodePairsRoutes);


	// Nodes Number in the network
	int nodesNumber_;

	// Contact Plan passed to all nodes to feed CGR
	// It can be different to the real topology
	ContactPlan contactPlan_;

	// Contact Plan passed to all nodes to schedule Contacts
	// and get transmission rates
	ContactPlan contactTopology_;

	//Observer used to collect and evaluate all the necessary metrics
	MetricCollector metricCollector_;

	// specify if there are ion nodes in the simulation
	bool ionNodes_;

	// flowIds map: (src,dst) -> flowId
	// save flow ids corresponding to traffic data
	// generated in App layer
	map<pair<int, int>, unsigned int> flowIds_;

	// stat signals
	simsignal_t contactsNumber;
	simsignal_t totalRoutes;
	simsignal_t availableRoutes;
	simsignal_t pairsOfNodesWithAtLeastOneRoute;

};

}


#endif
