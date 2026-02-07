/**************************************************************************
 *
 */

////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

namespace D3dApiCodeGen
{
  namespace Config
  {
    class GeneratorConfiguration
    {
    public:

      void SetOutputPath(const std::string& path);
      std::string& GetOutputPath();

    protected:

      std::string m_outputPath;

    };
  }
}

////////////////////////////////////////////////////////////////////////////////
