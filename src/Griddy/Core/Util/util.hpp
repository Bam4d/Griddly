#pragma once
#include <string>
#include <sstream>
#include <vector>
#include <iterator>

template <typename T>
inline void hash_combine (std::size_t& seed, const T& val)
{
 seed ^= std::hash<T>()(val) + 0x9e3779b9
 + (seed<<6) + (seed>>2);
}


template <typename Out>
void split(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        *result++ = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, std::back_inserter(elems));
  return elems;
}