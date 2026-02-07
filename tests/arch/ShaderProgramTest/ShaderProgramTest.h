/**************************************************************************
 *
 */

#ifndef SHADERPROGRAMTEST_H
    #define SHADERPROGRAMTEST_H

/// Define globals ///
// #define SHADER_PROGRAM_TEST_DEBUG

#ifdef SHADER_PROGRAM_TEST_DEBUG
    #define SPT_DEBUG(expr) { expr }
#else
#define SPT_DEBUG(expr) {}
#endif

#endif SHADERPROGRAMTEST_H
