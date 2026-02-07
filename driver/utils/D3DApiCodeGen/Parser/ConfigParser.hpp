
#pragma once

#include "stdafx.h"

namespace D3dApiCodeGen
{
  namespace Config
  {
    class ParserConfiguration;
  }

  namespace Items
  {
    class CppMacro;
    class EnumDescription;
    class StructDescription;
    class ClassDescription;
  }

  namespace Parser
  {
    class ConfigParser
    {
    public:
      ConfigParser(Config::ParserConfiguration& config, bool verbose=false);
      virtual ~ConfigParser();

      void ParseApiHeaders();

      std::vector<Items::ClassDescription>&  GetClasses();
      std::vector<Items::StructDescription>& GetStructs();
      std::vector<Items::EnumDescription>&   GetEnums();

    protected:
      bool                                   m_verbose;
      Config::ParserConfiguration&           m_config;
      std::vector<Items::EnumDescription>*   m_lstEnums;
      std::vector<Items::StructDescription>* m_lstStructs;
      std::vector<Items::ClassDescription>*  m_lstClasses;
      
      void ParseApiHeader(const std::string& filename);
      void ReadFileData(const std::string& filename, std::string& cadena);

      void AddEnums(std::vector<Items::EnumDescription>& enums);
      void AddStructs(std::vector<Items::StructDescription>& structs);
      void AddClasses(std::vector<Items::ClassDescription>& classes);
    };
  }  
}

