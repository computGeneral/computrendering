#pragma once

#include "stdafx.h"

namespace D3dApiCodeGen
{
  namespace Items
  {
    class CppMacro;
  }

  namespace Parser
  {
    class MacroExpander
    {
    public:
      MacroExpander(std::vector<Items::CppMacro>& macros, bool verbose=false);
      void Expand(std::string& cadena);

    protected:
      std::vector<Items::CppMacro>& m_lstMacros;
      bool m_verbose;
      void MatchMacro(Items::CppMacro& macro, std::string& cadena);

    };
  }
}

