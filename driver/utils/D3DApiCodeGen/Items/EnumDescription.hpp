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
    struct EnumDescriptionMember
    {
      std::string name;
    };
    
    class EnumDescription
    {
    public:

      void SetName(const std::string& name);
      std::string& GetName();
      
      bool AddMember(const EnumDescriptionMember& name);
      unsigned int GetMembersCount();
      EnumDescriptionMember& GetMember(unsigned int position);

    protected:

      std::string m_name;
      std::vector<EnumDescriptionMember> m_lstMembers;
      std::map<std::string,std::string> m_mapMembers;

    };
  }
}

////////////////////////////////////////////////////////////////////////////////
