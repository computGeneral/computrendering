/**************************************************************************
 *
 *  CG1 GPU definition file 
 *  This file contains definitions and includes for the
 *  main simulation loop 
 *
 */

#ifndef __CG1GPU_SIM_H__
#define __CG1GPU_SIM_H__

#include <string>
#include "MetaStreamTrace.h"

namespace cg1gpu
{

//#ifdef WIN32
//int  abortSignalHandler(int s);
//#else
//void abortSignalHandler(int s);
//#endif
void abortSignalHandler(int s);
void segFaultSignalHandler(int s);
bool MetaTraceSignChecker(MetaStreamHeader *metaTraceHeader);
bool MetaTraceParamChecker(MetaStreamHeader *metaTraceHeader);
bool fileExtensionTester(const std::string file_name, const std::string extension);// case insensitive file extension test

} // namespace cg1gpu;

#endif // #ifndef __CG1GPU_SIM_H__
