#pragma once

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

namespace D3dApiCodeGen
{
  namespace Items
  {
    struct MethodDescriptionParam
    {
      std::string type;
      std::string name;
    };
    
    class MethodDescription
    {
    public:

      void SetType(const std::string& type);
      std::string& GetType();

      void SetName(const std::string& name);
      std::string& GetName();

      void AddParam(const MethodDescriptionParam& param);
      unsigned int GetParamsCount();
      MethodDescriptionParam& GetParam(unsigned int position);

    protected:

      std::string m_type;
      std::string m_name;
      std::vector<MethodDescriptionParam> m_lstParams;

    };
  }
}

////////////////////////////////////////////////////////////////////////////////
