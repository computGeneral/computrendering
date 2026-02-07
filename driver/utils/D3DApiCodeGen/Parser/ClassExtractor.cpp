
#include "Utilities/Debug.hpp"
#include "Utilities/String.hpp"
#include "Config/ParserConfiguration.hpp"
#include "Items/ClassDescription.hpp"
#include "Items/MethodDescription.hpp"
#include "Parser/ConfigParser.hpp"
#include "Parser/ClassExtractor.hpp"

using namespace std;
using namespace regex;
using namespace D3dApiCodeGen::Config;
using namespace D3dApiCodeGen::Items;
using namespace D3dApiCodeGen::Parser;


const rpattern patro_class("class \\s+ (\\w+) \\s* { \\s* (.*?) \\s* } \\s* ;", EXTENDED | SINGLELINE);
const rpattern patro_class_methods("\\s* (.*?) \\s* ; \\s*", "", EXTENDED | SINGLELINE | GLOBAL | ALLBACKREFS);
const rpattern patro_class_method("(\\w+) \\s+ (\\w+) \\s* \\( \\s* (.*?) \\s* \\)", "", EXTENDED | GLOBAL | ALLBACKREFS);
const rpattern patro_class_method_clean("\\s+", " ", GLOBAL | NOBACKREFS);
const rpattern patro_class_params1("\\s* , \\s*", EXTENDED);
const rpattern patro_class_params2("^(.*?)? \\s* (\\w+)$", EXTENDED);

ClassExtractor::ClassExtractor(ParserConfiguration& config, string& cadena) :
m_config(config),
Extractor(cadena)
{
  m_lstClasses = new vector<ClassDescription>();
}

ClassExtractor::~ClassExtractor()
{
  delete m_lstClasses;
}

vector<ClassDescription>& ClassExtractor::GetClasses()
{
  return *m_lstClasses;
}

ClassExtractor::MatchResults ClassExtractor::Match(string& cadena)
{
  MatchResults resultat;
  
  match_results results;
  match_results::backref_type br = patro_class.match(cadena, results);
  if (br.matched)
  {
    resultat.matched = true;
    resultat.text = results.backref(0).str();
    cadena.erase(results.rstart(0), results.rlength(0));
  }
  
  return resultat;
}

void ClassExtractor::Parse(const ClassExtractor::MatchResults& resultat)
{
  ParseClass(resultat);
}

void ClassExtractor::ParseClass(const ClassExtractor::MatchResults& resultat)
{
  match_results results;
  patro_class.match(resultat.text, results);

  string name = results.backref(1).str();
  string subcadena = results.backref(2).str();

  if (m_config.IsClassParseCandidate(name))
  {
    if (m_verbose)
    {
      cout << "Parsing class " << name << endl;
    }
    ParseClassMethods(name, subcadena);
  }
  else
  {
    if (m_verbose)
    {
      cout << "Parsing class " << name << " -> IGNORED!" << endl;
    }
  }
}

void ClassExtractor::ParseClassMethods(const string& name, const string& cadena)
{    
  match_results results;
  match_results::backref_type br = patro_class_methods.match(cadena, results);
  if (br.matched)
  {
    // Eliminem de la cadena (amb tots els membres del struct) els elements
    // trobats. Si despr�s de fer aix� encara queda alg�n car�cter en la cadena
    // significa que no l'hem parsejada totalment i per tant hi ha errors.
    // [        
    string tmpCadena(cadena);
    subst_results results_test;
    patro_class_methods.substitute(tmpCadena, results_test);
    Utilities::String::TrimString(tmpCadena);
    // ]

    if (tmpCadena.length() == 0)
    {
      ClassDescription classDesc;
      classDesc.SetName(name);

      bool problem = false;
      for (size_t i=0; i < results.cbackrefs() && !problem; i+=2)
      {
        string member(results.backref(i+1).str());
        problem = !ParseClassMethod(classDesc, member);
      }

      if (!problem)
      {
        m_lstClasses->push_back(classDesc);
      }
    }
    else
    {
      cout << "PARSE ERROR: [class " << name << "] not all class methods processed!" << endl;
      cout << "> already remain in the string '" << tmpCadena << "'" << endl;
    }
  }
  else
  {
    cout << "PARSE ERROR: [class " << name << "] no methods found!" << endl;
  }
}

bool ClassExtractor::ParseClassMethod(ClassDescription& classDesc, string& cadena)
{
  // Netejem la cadena de possibles salts de l�nia i espais en blanc [
  subst_results results_clean;
  patro_class_method_clean.substitute(cadena, results_clean);
  Utilities::String::TrimString(cadena);
  // ]

  match_results results;
  match_results::backref_type br = patro_class_method.match(cadena, results);
  if (br.matched)
  {
    // Eliminem de la cadena (amb un membre del struct) els elements trobats
    // Si despr�s de fer aix� encara queda alg�n car�cter en la cadena significa
    // que no l'hem parsejada totalment i per tant hi ha errors.
    // [        
    string tmpCadena(cadena);
    subst_results results_test;
    patro_class_method.substitute(tmpCadena, results_test);
    Utilities::String::TrimString(tmpCadena);
    // ]

    if (tmpCadena.length() == 0)
    {
      MethodDescription methodDesc;
      methodDesc.SetType(results.backref(1).str());
      methodDesc.SetName(results.backref(2).str());

      string params(results.backref(3).str());
      if (ParseClassMethodParams(classDesc, methodDesc, params))
      {
        classDesc.AddMethod(methodDesc);
      }
    }
    else
    {
      cout << "PARSE ERROR: [class " << classDesc.GetName() << "] method could'nt be parsed! '" << cadena << "'" << endl;
      cout << "> already remain in the string '" << tmpCadena << "'" << endl;
      return false;
    }
  }
  else
  {
    // Ignorem els membres d'una classe que no disposen de parentesis, el que
    // significa que es tracta d'atributs i no volem processar-los.
    if (cadena.find('(') != string::npos)
    {
      cout << "PARSE ERROR: [class " << classDesc.GetName() << "] method could'nt be parsed! '" << cadena << "'" << endl;
      return false;
    }
  }

  return true;
}

bool ClassExtractor::ParseClassMethodParams(ClassDescription& classDesc, MethodDescription& methodDesc, string& cadena)
{
  split_results results1;
  if (patro_class_params1.split(cadena, results1))
  {
    split_results::iterator it;
    for (it=results1.begin(); it != results1.end(); it++)
    {
      match_results results2;
      match_results::backref_type br = patro_class_params2.match(*it, results2);
      if (br.matched)
      {
        MethodDescriptionParam cparam;
        cparam.name = results2.backref(2).str();
        cparam.type = results2.backref(1).str();
        methodDesc.AddParam(cparam);
      }
      else
      {
        cout << "PARSE ERROR: [method " << classDesc.GetName() << "::" << methodDesc.GetName() << "] param could'nt be parsed! '" << *it << "'" << endl;
        return false;
      }
    }
  }
  else
  {
    cout << "PARSE ERROR: [method " << classDesc.GetName() << "::" << methodDesc.GetName() << "] found incorrect params! '" << cadena << "'" << endl;
    return false;
  }

  return true;
}

