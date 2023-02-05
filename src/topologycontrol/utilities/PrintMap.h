/*
 * PrintMap.h
 *
 * Created on: Jan 24, 2023
 *     Author: Robin Ohs
 */

#ifndef TOPOLOGYCONTROL_PRINT_MAP_H
#define TOPOLOGYCONTROL_PRINT_MAP_H

#include <string>

namespace flora
{
    namespace utilities
    {
        /** @brief Prints a map where the second argument has a toString method. */
        template <typename Z, typename T>
        void PrintMap(std::map<Z, T> &map)
        {
            using namespace omnetpp;
            for (auto itr = map.begin(); itr != map.end(); ++itr)
            {
                EV << itr->first << "\t can connect to [";
                for (auto i : itr->second)
                {
                    EV << i << ",";
                }
                EV << "]" << endl;
            }
            EV << endl;
        };
    } // utilities

} // flora

#endif