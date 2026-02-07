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
    class EnumDescription;
  }
  
  namespace Parser
  {
    class EnumExtractor : public Extractor
    {
    public:

      EnumExtractor(Config::ParserConfiguration& config, std::string& cadena);
      virtual ~EnumExtractor();
      
      std::vector<Items::EnumDescription>& GetEnums();      

    protected:

      Config::ParserConfiguration& m_config;
      std::vector<Items::EnumDescription>* m_lstEnums;

      MatchResults Match(std::string& cadena);
      void Parse(const MatchResults& resultat);
      
      MatchResults MatchTypedefEnum(std::string& cadena);
      MatchResults MatchEnum(std::string& cadena);

      void ParseTypedefEnum(const MatchResults& resultat);
      void ParseEnum(const MatchResults& resultat);
      void ParseEnumMembers(const std::string& name, const std::string& cadena);
      bool ParseEnumMember(Items::EnumDescription& edes, std::string& cadena);

    };
  }
}

////////////////////////////////////////////////////////////////////////////////