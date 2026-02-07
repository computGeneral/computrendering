#include "Utilities/Exception.hpp"
#include "Utilities/FileSystem.hpp"
#include "Config/GeneratorConfiguration.hpp"
#include "Items/ClassDescription.hpp"
#include "Items/MethodDescription.hpp"
#include "Generator/IGenerator.hpp"
#include "Generator/ClassGenerator.hpp"
#include "Generator/DXHGenerator.hpp"

using namespace std;
using namespace D3dApiCodeGen;
using namespace D3dApiCodeGen::Config;
using namespace D3dApiCodeGen::Items;
using namespace D3dApiCodeGen::Generator;


DXHGenerator::DXHGenerator(GeneratorConfiguration& config) :
m_config(config)
{
  m_lstGenerators = new vector<IGenerator*>();
  SetGenerationPath(m_config.GetOutputPath());
}


DXHGenerator::~DXHGenerator()
{
  vector<IGenerator*>::iterator it;
  for (it=m_lstGenerators->begin(); it != m_lstGenerators->end(); it++)
  {
    delete (*it);
  }
  delete m_lstGenerators;
}

void DXHGenerator::AddClasses(vector<ClassDescription>& classes)
{
  vector<ClassDescription>::iterator it;
  for (it=classes.begin(); it != classes.end(); it++)
  {
    ClassGenerator* cgen = new ClassGenerator(*it, m_pathGeneration);
    m_lstGenerators->push_back(cgen);
  }
}

void DXHGenerator::SetGenerationPath(const string& path)
{
  if (!path.empty() && path[1] == ':')
  {
    m_pathGeneration = path;
    Utilities::FileSystem::DirectoryAddBackslash(m_pathGeneration);
  }
  else
  {
    if (path.empty())
    {
      m_pathGeneration.clear();
      m_pathGeneration = ".\\";
    }
    else
    {
      m_pathGeneration = Utilities::FileSystem::GetCurrentDirectory();
      m_pathGeneration += path;
      Utilities::FileSystem::DirectoryAddBackslash(m_pathGeneration);
    }
  }
}

void DXHGenerator::GenerateCode()
{
  CreateGenerationPath();

  vector<IGenerator*>::iterator it;
  for (it=m_lstGenerators->begin(); it != m_lstGenerators->end(); it++)
  {
    (*it)->GenerateCode();
  }
}

void DXHGenerator::CreateGenerationPath()
{
  if (!m_pathGeneration.empty() && !Utilities::FileSystem::DirectoryExists(m_pathGeneration))
  {
    cout << "Try to create directory '" << m_pathGeneration << "'..." << endl;
    if (Utilities::FileSystem::CreateDirectory(m_pathGeneration, true))
    {
      cout << "Directory created OK" << endl;
    }
    if (!Utilities::FileSystem::DirectoryExists(m_pathGeneration))
    {
      Exception e("could'nt create path '" + m_pathGeneration + "'");
      throw e;
    }
  }
}

