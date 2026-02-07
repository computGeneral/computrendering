
#pragma once

#include "stdafx.h"
#include "Parser/Extractor.hpp"

namespace D3dApiCodeGen
{
  namespace Parser
  {
    class PreprocessorExtractor : public Extractor
    {
    public:

      PreprocessorExtractor(std::string& cadena);

    protected:

      MatchResults Match(std::string& cadena);
      void Parse(const MatchResults& resultat);

    };
  }
}

