////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

namespace D3dApiCodeGen
{
  namespace Utilities
  {
    class String
    {
    public:
      static void TrimString(std::string& cadena);
      static void ToUpper(std::string& cadena);
      static void ToLower(std::string& cadena);
      static void Replace(std::string &s, std::string &find, std::string &replace);

    };
  }
}

////////////////////////////////////////////////////////////////////////////////
