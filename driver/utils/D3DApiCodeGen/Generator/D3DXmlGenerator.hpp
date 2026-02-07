/**************************************************************************
 *
 */

////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

namespace D3dApiCodeGen
{
  namespace Config
  {
    class GeneratorConfiguration;
  }
  
  namespace Items
  {
    class ClassDescription;
    class StructDescription;
    class EnumDescription;
    class MethodDescription;
  }

  namespace Parser
  {
      class ConfigParser;
  }

  namespace Generator
  {
    class D3DXmlGenerator: public IGenerator
    {
    public:

      /*******************
      Accessors
      *******************/
      void setClasses(std::vector<Items::ClassDescription> &c);
      void setStructs(std::vector<Items::StructDescription> &s);
      void setEnums(std::vector<Items::EnumDescription> &e);

      
      /*******************
      Generates a XML representation of D3D9 Api
      ********************/
      void GenerateCode();

      D3DXmlGenerator();

    private:
        /**************************
        Generate global functions, because
        ConfigParser doesn't parse them.
        **************************/
        void generateFunctions(std::ostream *o);
        void generateMethod(std::ostream *o, Items::MethodDescription &m);
        void generateClass(std::ostream *o, Items::ClassDescription &c);
        void generateEnum(std::ostream *o, Items::EnumDescription &e);
        void generateStruct(std::ostream *o, Items::StructDescription &s);
        std::vector<Items::ClassDescription> classes;
        std::map<std::string, std::string> inheritance;
        std::vector<Items::StructDescription> structs;
        std::vector<Items::EnumDescription> enums;
    };
  }
}

