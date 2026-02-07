/**************************************************************************
 *
 */

////////////////////////////////////////////////////////////////////////////////

#include "Items/StructDescription.hpp"

using namespace std;
using namespace D3dApiCodeGen::Items;

////////////////////////////////////////////////////////////////////////////////

void StructDescription::SetName(const string& name)
{
  m_name = name;
}

////////////////////////////////////////////////////////////////////////////////

string& StructDescription::GetName()
{
  return m_name;
}

////////////////////////////////////////////////////////////////////////////////

bool StructDescription::AddMember(const StructDescriptionMember& member)
{
  pair<string,string> ins_elem(member.name, member.name);
  pair<map<string,string>::iterator, bool> ins_res;
  ins_res = m_mapMembers.insert(ins_elem);
  if (ins_res.second)
  {
    m_lstMembers.push_back(member);
    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////

unsigned int StructDescription::GetMemberCount()
{
  return (unsigned int) m_lstMembers.size();
}

////////////////////////////////////////////////////////////////////////////////

StructDescriptionMember& StructDescription::GetMember(unsigned int position)
{
  static StructDescriptionMember member;
  if (m_lstMembers.size() && position < m_lstMembers.size())
  {
    return m_lstMembers[position];
  }
  return member;
}

////////////////////////////////////////////////////////////////////////////////
