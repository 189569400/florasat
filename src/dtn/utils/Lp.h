#ifndef LP_H_
#define LP_H_

#include "dtn/Config.h"

#ifdef USE_CPLEX_LIBRARY
#ifdef USE_BOOST_LIBRARIES

#include <ilcplex/ilocplex.h>
#include <vector>
#include <utility>
#include <map>
#include <boost/lexical_cast.hpp>
#include <boost/bimap.hpp>
#include <boost/multi_array.hpp>
#include "RouterGraphInfo.h"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "ContactPlan.h"
#include "TopologyUtils.h"
#include "RouterUtils.h"
#include "ContactPlan.h"

using namespace std;
using namespace boost;

class Lp
{

public:

	/// @brief Construct Lp object from a contactPlan and a traffic Matrix
	/// where traffic[k][k1][k2] contains the traffic in bytes generated in state k
	/// by source node k1 to destination node k2
	/// solve() must be called to solve the constructed model
	/// getSolvedStates must be called to get the solution/solutions
	Lp(ContactPlan *contactPlan, int nodesNumber, map<int, map<int, map<int, double> > > traffic);

	/// @brief Destructor
	virtual ~Lp();

	/// @brief Solve the lp model
	bool solve();

	/// @brief Compute n solutions (if possible) to the model
	bool computeNSolutions(int n, double solnPoolGap = 0.0);

	/// @brief Save model in a text file in fileLocation
	void exportModel(string fileLocation);

	/// @brief Print on screen the solution to the lp model
	/// must be called after solve()
	void printSolution();

	/// @brief Print on screen all solutions to the lp model
	/// must be called after solve()
	void printSolutions();

	/// @brief Obtiene un vector de estados resueltos por el lp
	vector<RouterGraph> getSolvedStates(int nSolution = 0);

	double getObjectiveValue(int nSolution);

	IloNumArray getSolution(int nSolution);

	/// @brief Obtiene un vector de estados resueltos por el lp
	vector<RouterGraph> getSolvedStates(IloNumArray vals);

	/// @brief Gets Routed Traffic
	// traffic_[contactId][pair<k1, k2>] = traffic
	map<int, map<pair<int, int>, double > > getRoutedTraffic(int nodeNumber, int solutionNumber);


	/// @brief Gets Used Contacts in a map that associates capacity used in bytes to a contact id
	map<int, double> getUsedContacts(int solutionNumber);

	/// @brief Gets states intervals
	vector<pair<int, int> > getIntervals() const;

	/// @brief Print on screen the traffic matrix
	void printTraffic();

	/// @brief Print on screen the commodities
	void printCommodities();

	/// @brief Gets Contact Plan
	ContactPlan* getContactPlan();

private:

	/// @brief Populates model_ with variables, objective function and restrictions
	void populateModel();

	/// @brief Populates cplex_ from model_
	void populateCplex();

	/// @brief Populates commodities from traffic matrix
	void populateCommodities();

	/// @brief Gets the traffic generated in one state by a source node k1
	/// to a destination node k2
	double getTraffic(int state, int k1, int k2);

	/// @brief Gets the total Traffic whose destination is node k2
	double getTrafficForDestination(int k2);

	/// Contact Plan
	ContactPlan *contactPlan_;

	/// nodesNumber
	int nodesNumber_;

	/// traffic matrix
	/// traffic[k][k1][k2] contains the traffic in bytes generated in state k
	/// by source node k1 to destination node k2
	map<int, map<int, map<int, double> > > traffic_;

	/// lp states
	vector<RouterGraph> states_;

	/// stateTimes intervals
	vector<pair<int, int> > intervals_;

	/// source-destination traffic pairs
	set<pair<int, int> > commodities_;

	/// Cplex variables
	IloEnv env_;
	IloModel model_;
	IloNumVarArray dVars_;
	IloRangeArray constraints_;
	IloCplex cplex_;

	/// bi-map with relates a string id with an integer id.
	/// Used to set and get variables to the lp model.
	/// It can be accesed in both ways.
	bimap<string, int> variables_;

};

#endif /* LP_H_ */
#endif /* USE_BOOST_LIBRARIES */
#endif /* USE_CPLEX_LIBRARY */

