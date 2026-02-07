#pragma once

#include "stdafx.h"

namespace D3dApiCodeGen
{
  namespace Parser
  {
    class ConfigParser;
  }

    namespace Config
  {
    class ParserConfiguration;
    class GeneratorConfiguration;

    class ConfigManager
    {
    public:

      ConfigManager(const std::string& filename, bool verbose=false, bool createIfNotExists=false);

      void GetParserConfiguration(ParserConfiguration& config);
      void GetGeneratorConfiguration(GeneratorConfiguration& config);

    protected:

      std::string m_filename;
      bool m_verbose;
      TiXmlDocument m_xmlDocument;

      void AddParserFiles(ParserConfiguration& config);
      void AddParserEnums(ParserConfiguration& config);
      void AddParserStructs(ParserConfiguration& config);
      void AddParserClasses(ParserConfiguration& config);
      void AddParserMacros(ParserConfiguration& config);
      
      bool AddHeader();
      bool AddSection(std::string sectionParent, std::string sectionName);
      bool AddSection(std::string sectionParent, std::string sectionName, std::string sectionText);
      bool AddSectionAttribute(std::string sectionName, std::string attribName, std::string attribValue);
      bool ExistsHeader();
      TiXmlNode* GetSection(std::string sectionName);
      std::vector<std::string>* GetTextList(std::string sectionName, std::string subSectionName);
      void Clear();
      bool FillDefault();
      std::string GetDirectXSDKPath();
      bool Save();

    };
  }
}

