
#pragma once

#include "stdafx.h"

namespace D3dApiCodeGen
{
  namespace Parser
  {
    class Extractor
    {
    public:
      Extractor(std::string& cadena, bool verbose=false);
      void Extract();

    protected:
      struct MatchResults
      {
        bool matched;
        int type;
        std::string text;

        MatchResults() :
        matched(false),
        type(0)
        {
        }
        
        void Clear()
        {
          matched = false;
          type = 0;
          text.clear();
        }
      };
      
      std::string& m_cadena;
      bool m_verbose;

      virtual MatchResults Match(std::string& cadena) = 0;
      virtual void Parse(const MatchResults& resultat) = 0;
    };
  }
}

