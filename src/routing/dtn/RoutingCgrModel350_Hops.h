
#ifndef SRC_NODE_DTN_ROUTINGCGRMODEL_3_H_
#define SRC_NODE_DTN_ROUTINGCGRMODEL_3_H_

#include <routing/dtn/CgrRoute.h>
#include <routing/dtn/RoutingDeterministic.h>
#include <dtn/node/dtn/SdrModel.h>

class RoutingCgrModel350_Hops: public RoutingDeterministic
{
public:
	RoutingCgrModel350_Hops(int eid, SdrModel * sdr, ContactPlan * contactPlan, bool printDebug);
	virtual ~RoutingCgrModel350_Hops();
	virtual void routeAndQueueBundle(BundlePkt *bundle, double simTime);
	virtual CgrRoute* getCgrBestRoute(BundlePkt * bundle, double simTime);
	virtual vector<CgrRoute> getCgrRoutes(BundlePkt * bundle, double simTime);

	// stats recollection
	int getDijkstraCalls();
	int getDijkstraLoops();
	int getRouteTableEntriesExplored();
	int getChangedRoutes();


private:

	int dijkstraCalls;
	int dijkstraLoops;
	int tableEntriesExplored;
	int changedRoutes;

	bool printDebug_ = false;
	int eid_;
	SdrModel * sdr_;
	ContactPlan * contactPlan_;

	/////////////////////////////////////////////////
	// Ion Cgr Functions based in libcgr.c (v 3.5.0):
	/////////////////////////////////////////////////
#define	MIN_CONFIDENCE_IMPROVEMENT	(.05)
#define MIN_DTN_DELIVERY_CONFIDENCE	(.80)
#define MAX_XMIT_COPIES (20)
#define	MAX_SPEED_MPH	(150000)

	typedef struct
	{
		int neighborNodeNbr;
		int contactId; // This is not, in ION
		double forfeitTime;
		double arrivalTime;
		float confidence;
		unsigned int hopCount; // hops from dest. node.
		double meanStartTime;
		CgrRoute * route; // pointer to route so we can decrement capacities
		//Scalar	overbooked; 	//Bytes needing reforward.
		//Scalar	protected; 		//Bytes not overbooked.
	} ProximateNode;

	typedef struct
	{
		Contact * contact;
		Contact * predecessor;	// predecessor Contact
		double arrivalTime;
		double capacity; 	// in Bytes
		bool visited;
		bool suppressed;
	} Work;

	map<int, vector<CgrRoute> > routeList_;
	double routeListLastEditTime = -1;

	void cgrForward(BundlePkt * bundle, double simTime);
	void identifyProximateNodes(BundlePkt * bundle, double simTime, vector<int> excludedNodes, vector<ProximateNode> * proximateNodes);
	void loadRouteList(int terminusNode, double simTime);

	void findNextBestRoute(Contact * rootContact, int terminusNode, CgrRoute * route);
	void tryRoute(BundlePkt * bundle, CgrRoute * route, vector<ProximateNode> * proximateNodes);
	void recomputeRouteForContact();
	void enqueueToNeighbor(BundlePkt * bundle, ProximateNode * selectedNeighbor);
	void enqueueToLimbo(BundlePkt * bundle);
	void bpEnqueue(BundlePkt * bundle, ProximateNode * selectedNeighbor);
};

#endif /* SRC_NODE_DTN_ROUTINGCGRMODEL_3_H_ */
