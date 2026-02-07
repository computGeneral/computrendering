
#include "stdafx.h"
//#include "Vld/vld.h"  // VLD (virtual leak detection) library not included in the repository
#include "Utilities/Exception.hpp"
#include "Config/ConfigManager.hpp"
#include "Config/ParserConfiguration.hpp"
#include "Config/GeneratorConfiguration.hpp"
#include "Items/CppMacro.hpp"
#include "Items/ClassDescription.hpp"
#include "Items/MethodDescription.hpp"
#include "Items/StructDescription.hpp"
#include "Items/EnumDescription.hpp"
#include "Parser/ConfigParser.hpp"
#include "Generator/DXHGenerator.hpp"
#include "Generator/IGenerator.hpp"
#include "Generator/D3DPixPlayerCodeGenerator.hpp"
#include "Generator/D3DXmlGenerator.hpp"

using namespace std;
using namespace D3dApiCodeGen;
using namespace D3dApiCodeGen::Config;
using namespace D3dApiCodeGen::Items;
using namespace D3dApiCodeGen::Parser;
using namespace D3dApiCodeGen::Generator;

bool isPointer(StructDescription s) {
    string n = s.GetName();
    return n.find("*") != n.npos;
}

int main()
{
  try
  {
    ConfigManager          configManager("D3D9ApiCodeGen.cfg");    // Get configuration

    ParserConfiguration    parserConfig;
    configManager.GetParserConfiguration(parserConfig);
    ConfigParser parser(parserConfig);
    parser.ParseApiHeaders();

    GeneratorConfiguration generatorConfig;
    configManager.GetGeneratorConfiguration(generatorConfig);
    D3DPixPlayerCodeGenerator d3dApiCodeGen(generatorConfig); // Generate code for D3D9PixRunPlayer
    d3dApiCodeGen.setClasses(parser.GetClasses());
    d3dApiCodeGen.GenerateCode();


    // Uncomment to generate a XML representation of D3D9API
    
    /** @fix ConfigParser incorrectly identify some
     *  pointer typedefs as structs. This example
     *  generates a struct named D3DDEVINFO_VCACHE,
     *  but also another named *LPD3DDEVINFO_VCACHE,
     *  and this is an error.
     * 
     * typedef struct _D3DDEVINFO_VCACHE {
     *     ...
     *     ...
     * } D3DDEVINFO_VCACHE, *LPD3DDEVINFO_VCACHE;
     **/
    
    // Remove all structs with a * in the name.
    // "remove_if" is not straightforward, search STL
    // docs for its usage.
    
    //vector<StructDescription> &structs = parser.GetStructs();
    //vector<StructDescription>::iterator itNewEnd;
    //itNewEnd = remove_if(structs.begin(), structs.end(), isPointer);
    //structs.erase(itNewEnd, structs.end());

    //D3DXmlGenerator d3dxmlgenerator;
    //d3dxmlgenerator.setClasses(parser.GetClasses());
    //d3dxmlgenerator.setEnums(parser.GetEnums());
    //d3dxmlgenerator.setStructs(parser.GetStructs());

    //d3dxmlgenerator.GenerateCode();

    }
    catch (Exception& e)
    {
        cout << "EXCEPTION INTERNAL: " << e.what() << endl;
    }
    catch (exception& e)
    {
    cout << "EXCEPTION EXTERNAL: " << e.what() << endl;
    }

    return 0;
}
