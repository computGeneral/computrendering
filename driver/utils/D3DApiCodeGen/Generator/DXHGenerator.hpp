#pragma once

#include "stdafx.h"

namespace D3dApiCodeGen
{
  namespace Config
  {
    class GeneratorConfiguration;
  }
  
  namespace Items
  {
    class ClassDescription;
  }

  namespace Generator
  {
    class IGenerator;
    class ClassGenerator;

    class DXHGenerator
    {
    public:
      
      DXHGenerator(Config::GeneratorConfiguration& config);
      ~DXHGenerator();

      void AddClasses(std::vector<Items::ClassDescription>& classes);
      void GenerateCode();
    
    protected:

      Config::GeneratorConfiguration& m_config;
      std::string m_pathGeneration;
      std::vector<IGenerator*>* m_lstGenerators;

      void CreateGenerationPath();
      void SetGenerationPath(const std::string& path);

    };
  }
}

