/**************************************************************************
 *
 */

////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

namespace D3dApiCodeGen
{
  namespace Items
  {
    class CppMacro
    {
    public:

      CppMacro(const std::string& left, const std::string& right);

      inline std::string& Left(void) {return m_left;}
      inline std::string& Right(void) {return m_right;}

    protected:

      std::string m_left;
      std::string m_right;

    };
  }
}

////////////////////////////////////////////////////////////////////////////////
