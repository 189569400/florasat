/*
 * SetUtils.h
 *
 *  Created on: Mar 23, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_CORE_UTILS_SETUTILS_H_
#define __FLORA_CORE_UTILS_SETUTILS_H_

#include <algorithm>
#include <set>

namespace flora {
namespace core {
namespace utils {
namespace set {

/** @brief Checks if a set contains a given value. */
template <typename T>
bool contains(const std::set<T>& set, T value) {
    return std::find(set.begin(), set.end(), value) != set.end();
}

template <typename T>
std::string toString(T begin, T end) {
    std::stringstream ss;
    ss << "(";
    bool first = true;
    for (; begin != end; begin++) {
        if (!first)
            ss << ", ";
        ss << *begin;
        first = false;
    }
    ss << ")";
    return ss.str();
}

}  // namespace set
}  // namespace utils
}  // namespace core
}  // namespace flora

#endif  // __FLORA_CORE_UTILS_SETUTILS_H_