/**************************************************************************
 *
 */

////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
#include "Parser/Extractor.hpp"

////////////////////////////////////////////////////////////////////////////////

namespace D3dApiCodeGen
{
  namespace Config
  {
    class ParserConfiguration;
  }

  namespace Items
  {
    class StructDescription;
  }
  
  namespace Parser
  {
    class StructExtractor : public Extractor
    {
    public:

      StructExtractor(Config::ParserConfiguration& config, std::string& cadena);
      virtual ~StructExtractor();

      std::vector<Items::StructDescription>& GetStructs();

    protected:

      Config::ParserConfiguration& m_config;
      std::vector<Items::StructDescription>* m_lstStructs;
      
      MatchResults Match(std::string& cadena);
      void Parse(const MatchResults& resultat);

      MatchResults MatchTypedefStruct(std::string& cadena);
      MatchResults MatchStruct(std::string& cadena);

      void ParseTypedefStruct(const MatchResults& resultat);
      void ParseStruct(const MatchResults& resultat);
      void ParseStructMembers(const std::string& name, const std::string& cadena);
      bool ParseStructMember(Items::StructDescription& sdes, std::string& cadena);

    };
  }
}

////////////////////////////////////////////////////////////////////////////////