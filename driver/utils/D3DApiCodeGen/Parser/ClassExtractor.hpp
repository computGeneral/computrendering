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
    class ClassDescription;
    class MethodDescription;
  }
  
  namespace Parser
  {
    class ClassExtractor : public Extractor
    {
    public:

      ClassExtractor(Config::ParserConfiguration& config, std::string& cadena);
      ~ClassExtractor();

      std::vector<Items::ClassDescription>& GetClasses();

    protected:

      Config::ParserConfiguration& m_config;
      std::vector<Items::ClassDescription>* m_lstClasses;

      MatchResults Match(std::string& cadena);
      void Parse(const MatchResults& resultat);

      void ParseClass(const MatchResults& resultat);
      void ParseClassMethods(const std::string& name, const std::string& cadena);
      bool ParseClassMethod(Items::ClassDescription& cdes, std::string& cadena);
      bool ParseClassMethodParams(Items::ClassDescription& cdes, Items::MethodDescription& cmethod, std::string& cadena);

    };
  }
}

////////////////////////////////////////////////////////////////////////////////