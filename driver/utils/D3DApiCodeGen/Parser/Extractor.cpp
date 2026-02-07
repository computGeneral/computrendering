
#include "Parser/Extractor.hpp"

using namespace std;
using namespace D3dApiCodeGen::Parser;

Extractor::Extractor(string& cadena, bool verbose) :
m_cadena(cadena),
m_verbose(verbose)
{
}

void Extractor::Extract()
{
    MatchResults resultat;
    do {
        resultat.Clear();
        resultat = Match(m_cadena);
        if (resultat.matched)
        {
          Parse(resultat);
        }
    } while(resultat.matched);
}
