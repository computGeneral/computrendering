/**************************************************************************
 *
 */

#ifndef GAL_MACROS
    #define GAL_MACROS

#define GAL_ASSERT_ON

#ifdef GAL_ASSERT_ON
    #define GAL_ASSERT(x) { x }
#else
    #define GAL_ASSERT(x) {}
#endif

#endif // GAL_MACROS
