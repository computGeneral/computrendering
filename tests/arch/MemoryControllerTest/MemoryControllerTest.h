/**************************************************************************
 *
 */

#ifndef MEMORYCONTROLLERTEST_H
    #define MEMORYCONTROLLERTEST_H

/// Define globals ///
// #define MEMORY_CONTROLLER_TEST_DEBUG

#ifdef MEMORY_CONTROLLER_TEST_DEBUG
    #define MCT_DEBUG(expr) { expr }
#else
#define MCT_DEBUG(expr) {}
#endif

#endif MEMORYCONTROLLERTEST_H
