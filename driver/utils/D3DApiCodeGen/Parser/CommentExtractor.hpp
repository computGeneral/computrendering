#pragma once

#include "stdafx.h"
#include "Parser/Extractor.hpp"

namespace D3dApiCodeGen
{
  namespace Parser
  {
    class CommentExtractor : public Extractor
    {
    public:
      CommentExtractor(std::string& cadena);

    protected:
      MatchResults Match(std::string& cadena);
      void Parse(const MatchResults& resultat);

    };
  }
}
