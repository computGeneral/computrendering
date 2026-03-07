/**************************************************************************
 * GPU Types definition file.
 *
 */

#ifndef __GPUTYPES__
#define __GPUTYPES__

#include <stddef.h>
#include <limits.h>

//#define IS_64BITS   (LONG_MAX>2147483647)

// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

#ifdef WIN32
   #define U64FMT "%I64d"
#else
   #define U64FMT "%lld"
   #define  sprintf_s(buf, size, ...)  snprintf(buf, size, __VA_ARGS__)
   #define vsprintf_s(buf, size, ...) vsnprintf(buf, size, __VA_ARGS__)
#endif

// Check GCC
#if __GNUC__
#if __x86_64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

// Big constants 
// _UL - Unsigned long, _SL - Signed long 
#ifdef ENVIRONMENT64  // 64bit machines 
#define _UL(N)      N##UL
#define _SL(N)      N##L
#else          // 32bit machines 
#define _UL(N)      N##ULL
#define _SL(N)      N##LL
#endif

typedef float               F32;     //  float point 32 bit.  
typedef double              F64;     //  float point 64 bit.  

//typedef signed char         S08;      // signed byte: -128..127 
//typedef signed short        S16;     // signed halfword: -32768..32767
//typedef signed int          S32;     // signed word: -2^31..2^31-1 
typedef char        S01;
typedef char        S02;
typedef char        S03;
typedef char        S04;
typedef char        S05;
typedef char        S06;
typedef char        S07;
typedef char        S08;
typedef short int   S09;
typedef short int   S10;
typedef short int   S11;
typedef short int   S12;
typedef short int   S13;
typedef short int   S14;
typedef short int   S15;
typedef short int   S16;
typedef int         S17;
typedef int         S18;
typedef int         S19;
typedef int         S20;
typedef int         S21;
typedef int         S22;
typedef int         S23;
typedef int         S24;
typedef int         S25;
typedef int         S26;
typedef int         S27;
typedef int         S28;
typedef int         S29;
typedef int         S30;
typedef int         S31;
typedef int         S32;
typedef long long   S33;
typedef long long   S34;
typedef long long   S35;
typedef long long   S36;
typedef long long   S37;
typedef long long   S38;
typedef long long   S39;
typedef long long   S40;
typedef long long   S41;
typedef long long   S42;
typedef long long   S43;
typedef long long   S44;
typedef long long   S45;
typedef long long   S46;
typedef long long   S47;
typedef long long   S48;
typedef long long   S49;
typedef long long   S50;
typedef long long   S51;
typedef long long   S52;
typedef long long   S53;
typedef long long   S54;
typedef long long   S55;
typedef long long   S56;
typedef long long   S57;
typedef long long   S58;
typedef long long   S59;
typedef long long   S60;
typedef long long   S61;
typedef long long   S62;
typedef long long   S63;
typedef long long   S64;


//typedef unsigned char       U08;      // unsigned byte: 0..255 
//typedef unsigned short      U16;     // unsigned halfword: 0..65535 
//typedef unsigned int        U32;     // unsigned word: 0..2^32-1 
typedef unsigned char        U01;
typedef unsigned char        U02;
typedef unsigned char        U03;
typedef unsigned char        U04;
typedef unsigned char        U05;
typedef unsigned char        U06;
typedef unsigned char        U07;
typedef unsigned char        U08;
typedef unsigned short int   U09;
typedef unsigned short int   U10;
typedef unsigned short int   U11;
typedef unsigned short int   U12;
typedef unsigned short int   U13;
typedef unsigned short int   U14;
typedef unsigned short int   U15;
typedef unsigned short int   U16;
typedef unsigned int         U17;
typedef unsigned int         U18;
typedef unsigned int         U19;
typedef unsigned int         U20;
typedef unsigned int         U21;
typedef unsigned int         U22;
typedef unsigned int         U23;
typedef unsigned int         U24;
typedef unsigned int         U25;
typedef unsigned int         U26;
typedef unsigned int         U27;
typedef unsigned int         U28;
typedef unsigned int         U29;
typedef unsigned int         U30;
typedef unsigned int         U31;
typedef unsigned int         U32;
typedef unsigned long long   U33;
typedef unsigned long long   U34;
typedef unsigned long long   U35;
typedef unsigned long long   U36;
typedef unsigned long long   U37;
typedef unsigned long long   U38;
typedef unsigned long long   U39;
typedef unsigned long long   U40;
typedef unsigned long long   U41;
typedef unsigned long long   U42;
typedef unsigned long long   U43;
typedef unsigned long long   U44;
typedef unsigned long long   U45;
typedef unsigned long long   U46;
typedef unsigned long long   U47;
typedef unsigned long long   U48;
typedef unsigned long long   U49;
typedef unsigned long long   U50;
typedef unsigned long long   U51;
typedef unsigned long long   U52;
typedef unsigned long long   U53;
typedef unsigned long long   U54;
typedef unsigned long long   U55;
typedef unsigned long long   U56;
typedef unsigned long long   U57;
typedef unsigned long long   U58;
typedef unsigned long long   U59;
typedef unsigned long long   U60;
typedef unsigned long long   U61;
typedef unsigned long long   U62;
typedef unsigned long long   U63;
typedef unsigned long long   U64;

//#if IS_64BITS       // 64bit machines 
typedef unsigned long long  U64;     // unsigned longword: 0..2^64-1 
typedef signed long long    S64;     // signed longword: -2^63..2^63-1 
//#else               // 32bit machines 
//typedef unsigned long long  U64;     // unsigned longword: 0..2^32-1 
//typedef signed long long    S64;     // signed longword: -2^31..2^31-1 
//#endif

#ifdef ENVIRONMENT64
typedef unsigned long long  pointer;    // unsigned integer with the size of a pointer in the machine.  
#else
typedef unsigned long  pointer;         // unsigned integer with the size of a pointer in the machine.  
#endif

typedef U16                f16bit;     //  Storage for float 16 values.  No operations implemented.

#include "Vec4FP32.h"
#include "Vec4Int.h"


/**
 * We also need to define macros for the PRINTF format statements,
 * since on 32bit machines, you typically need a "%lld" to print
 * a 64 bit value, wheareas on 64 bit machines, you simply use "%ld"
 */
#if IS_64BITS
#define FMTU64     "%lu"
#define FMTS64     "%ld"
#define FMTX64     "%lx"
#else
#define FMTU64     "%llu"
#define FMTS64     "%lld"
#define FMTX64     "%llx"
#endif


/**
 * Generic pointer. Be careful with pointer size
 */

typedef char*   genPtr;

#define GPTRSZ  sizeof(genPtr)


/**
 * Register type
 * 64 bits size and contents, with ease to access individual portions
 */
/*
typedef union
{
    U08   u8[8];
    U16  u16[4];
    U32  u32[2];
    //U64  u64;
} reg_t;

*/

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define MAX_INFO_SIZE  255
/**
 * flag * To contain a boolean or very range-limited value. Useful to parse arguments and the like.
 */
typedef char    flag;


/**
 * Other structures
 */

typedef enum {
    BIG_END = 1,
    LITTLE_END
} endian_t;


// GPU Simulator Types (MAL)
enum cgeModelAbstractLevel
{
    CG_BEHV_MODEL, // CG1 Gpu Behavior Model 
    CG_FUNC_MODEL, // CG1 Gpu Function Model 
    CG_ARCH_MODEL, // CG1 Gpu Archtecture Model 
    CG_PERF_MODEL, // CG1 Gpu Performance Model
    CG_ENGY_MODEL, // CG1 Gpu Energy Model
};

#endif
