/**************************************************************************
 *
 */

#include "UserCallTable.h"

#include <cstring>

void initUserCallTable(UserCallTable& uct)
{
    std::memset(&uct, 0, sizeof(uct));
}
