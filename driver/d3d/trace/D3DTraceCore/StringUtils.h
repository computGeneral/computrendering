/**************************************************************************
 *
 */

/*****************************************************************************
String parsing utilities
*****************************************************************************/
#include <string>

#ifndef __STRING_UTILS
#define __STRING_UTILS

std::vector< std::string > explode(std::string s, const std::string& by);
std::vector< std::string > explode_by_any(std::string s, const std::string& characters);
std::string trim(std::string& s,const std::string& drop = " \t");

#endif
