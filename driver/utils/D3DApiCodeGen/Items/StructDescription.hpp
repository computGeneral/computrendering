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
    struct StructDescriptionMember
    {
      std::string type;
      std::string name;
    };

    class StructDescription
    {
    public:

      void SetName(const std::string& name);
      std::string& GetName();

      bool AddMember(const StructDescriptionMember& member);
      unsigned int GetMemberCount();
      StructDescriptionMember& GetMember(unsigned int position);

    protected:

      std::string m_name;
      std::vector<StructDescriptionMember> m_lstMembers;
      std::map<std::string,std::string> m_mapMembers;
    
    };
  }
}

////////////////////////////////////////////////////////////////////////////////
