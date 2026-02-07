/**************************************************************************
 *
 */

////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
#include "Generator/IGenerator.hpp"

////////////////////////////////////////////////////////////////////////////////

namespace D3dApiCodeGen
{
  namespace Items
  {
    class ClassDescription;
    class MethodDescription;
  }
  
  namespace Generator
  {
    class ClassGenerator : public IGenerator
    {
    public:

      ClassGenerator(Items::ClassDescription& cdes, const std::string& outputPath);
      
      void GenerateCode();

    protected:

      Items::ClassDescription m_classDescription;
      std::string m_outputPath;
      std::string m_classNameSuffix;

      std::string GetClassName();
      //std::string GetDateTime();
      void GenerateHeaderComment(std::string& comment);
      void GenerateHpp();
      void GenerateCpp();
      void GenerateClassBody(std::ofstream* of);
      void GenerateClassMethods(std::ofstream* of);
      void GenerateClassMethod(std::ofstream* of, unsigned int position);
      void GenerateClassMethodBody(std::ofstream* of, unsigned int position);

    };
  }
}

