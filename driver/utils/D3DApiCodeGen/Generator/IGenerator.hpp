/**************************************************************************
 *
 */

////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

namespace D3dApiCodeGen
{
  namespace Generator
  {
    class IGenerator
    {
    public:

      void SetHeaderComment(const std::string& message);
      std::string& GetHeaderCommment();
      
      virtual void GenerateCode() = 0;

    protected:

      std::string m_headerComment;

      std::ofstream* CreateFilename(const std::string& filename);
      void WriteHeaderComment(std::ofstream* of);
      void CloseFilename(std::ofstream* of);

    };
  }
}

////////////////////////////////////////////////////////////////////////////////
