//
// Created by asorgejr on 7/11/2021.
//

#pragma once
#include <string>
#include <vector>
#include <regex>
#include <UT/UT_String.h>
#include <OP/OP_Node.h>


namespace ams {

std::vector<OP_Node*> getInputAncestors(const OP_Node& node);


/**
 * @brief replaces elements of string using a regex pattern.
 * @param str the string to modify
 * @param pattern a regex pattern to search for in str
 * @param replace this will replace pattern in str. Using the tilde (~) key and a number,
 * i.e.: ~1, references a numbered capture group specified in the pattern.
 * @return the formatted string.
 */
std::string re_replace(const std::string& str, const std::string& pattern, const std::string& replace);
/**
 * @brief replaces elements of string using a regex pattern.
 * @param str the string to modify
 * @param rgx a regex object to use for searching
 * @param replace this will replace pattern in str. Using the tilde (~) key and a number,
 * i.e.: ~1, references a numbered capture group specified in the pattern.
 * @return the formatted string.
 */
std::string re_replace(const std::string& str, const std::regex& rgx, const std::string& replace);


/**
 * @brief slices a container object.
 * @tparam containerType any container which implements the [] operator.
 * @param arr the container to slice
 * @param s the starting index.
 * @param e the ending index.
 * @return a sliced container.
 */
template <class containerType>
containerType slice(const containerType& arr, size_t s, size_t e) {
  auto start = arr.begin() + s;
  auto end = arr.begin() + e + 1;
  containerType result(e - s + 1);
  copy(start, end, result.begin());
  return result;
}

/**
 * @brief slices a string.
 * @param str the container to slice
 * @param s the starting index.
 * @param e the ending index.
 * @return a sliced string
 */
std::string sslice(const std::string& str, size_t s, size_t e);

}

