/**************************************************************************
 *
 */

////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

namespace D3dApiCodeGen
{
  namespace Items
  {
    class MethodDescription;

    class ClassDescription
    {
    public:

      void SetName(const std::string& name);
      std::string& GetName();

      void AddMethod(const MethodDescription& method);
      unsigned int GetMethodsCount();
      MethodDescription& GetMethod(unsigned int position);

    protected:

      std::string m_name;
      std::vector<MethodDescription> m_lstMethods;

    };
  }
}

////////////////////////////////////////////////////////////////////////////////
