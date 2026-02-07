
#include "Items/ClassDescription.hpp"
#include "Items/MethodDescription.hpp"
#include "Utilities/String.hpp"
#include "Utilities/FileSystem.hpp"

#include "Parser/ConfigParser.hpp"
#include "Generator/IGenerator.hpp"
#include <algorithm>
#include "Generator/D3DPixPlayerCodeGenerator.hpp"


using namespace D3dApiCodeGen;
using namespace D3dApiCodeGen::Items;
using namespace D3dApiCodeGen::Parser;
using namespace D3dApiCodeGen::Generator;
using namespace D3dApiCodeGen::Utilities;
using namespace std;

void PerParameterGenerator::setParameter(MethodDescriptionParam p) {
    param = p;

    // Remove "CONST " from the type because we won't use it and it
    // forces us to initialize some variables.
    param.type = removeConst(param.type);

    // A REFGUID or RIID variable is a reference to a GUID and must be initialized,
    // so we translate it to GUID.
    param.type = translateReferencesToGuid(param.type);
}



void PerParameterGenerator::generateCodeBeforeCall(ofstream *out) {

    bool isVoid;
    bool isHiddenPointer;
    bool isLockedRect;
    bool isLockedBox;

    switch(getType()) {
        case VALUE_PARAMETER:

            isVoid = (param.type.find("void") != param.type.npos);

            isHiddenPointer = (param.type.find("HDC") != param.type.npos) |
                              (param.type.find("HWND") != param.type.npos) |
                              (param.type.find("HMONITOR") != param.type.npos);

            if (isVoid || isHiddenPointer || isIrregularPointer(param.type))
            {
                /***********************
                EXAMPLE:

                    HANDLE *SizeToLock

                    PIXPointer ov_SizeToLock;
                    HANDLE *sv_SizeToLock;
                    reader.readParameter<PIXPointer>(&ov_SizeToLock);
                    sv_SizeToLock = (HANLDE *) ov_SizeToLock;
                ***********************/

                *out << "PIXPointer ov_" << param.name << ";" << endl;
                *out << param.type << " sv_" << param.name << ";" << endl;
                *out << "reader.readParameter<PIXPointer>(&"
                    << "ov_" << param.name << ");" << endl;
                *out << "sv_" << param.name << " = (" << param.type << ") pointer(ov_" << param.name << ");"
                    << endl << endl;
            }
            else
            {
                /***********************
                EXAMPLE:

                    UINT SizeToLock

                    UINT ov_SizeToLock;
                    UINT sv_SizeToLock;
                    reader.readParameter<UINT>(&ov_SizeToLock);
                    sv_SizeToLock = ov_SizeToLock;
                ***********************/

                *out << param.type << " ov_" << param.name << ";" << endl;
                *out << param.type << " sv_" << param.name << ";" << endl;
                *out << "reader.readParameter<" << param.type << ">(&"
                    << "ov_" << param.name << ");" << endl;
                *out << "sv_" << param.name << " = " << "ov_" << param.name << ";"
                    << endl << endl;
            }
            break;

        case VALUE_POINTER_PARAMETER:

            isLockedRect = (param.type.find("D3DLOCKED_RECT") != param.type.npos);
            isLockedBox = (param.type.find("D3DLOCKED_BOX") != param.type.npos);

            if (isLockedRect)
            {
                /***************************
                EXAMPLE:

                    D3DLOCKED_RECT * pLockedRect;

                    PIXPointer opv_pLockedRect;
                    D3DLOCKED_RECT_COMP_WIN32 ov_pLockedRect;
                    D3DLOCKED_RECT * spv_pLockedRect;
                    D3DLOCKED_RECT   sv_pLockedRect;
                    reader.readParameter<PIXPointer>(&opv_pLockedRect);
                    reader.readParameter<D3DLOCKED_RECT_COMP_WIN32>(&ov_pLockedRect);
                    spv_pLockedRect = ((opv_pLockedRect == 0 ) ? 0 : &sv_pLockedRect);
                    sv_pLockedRect.Pitch = ov_pLockedRect.Pitch;
                    sv_pLockedRect.pBits = (void *) pointer(ov_pLockedRect.pBits);
                ***************************/
                *out << "PIXPointer opv_" << param.name << ";" << endl;
                *out << "D3DLOCKED_RECT_COMP_WIN32 ov_" << param.name << ";" << endl;
                *out << param.type << " spv_" << param.name << ";" << endl;
                *out << removeLastAsterisk(param.type) << " sv_" << param.name << ";" << endl;
                *out << "reader.readParameter<PIXPointer>(&"
                    << "opv_" << param.name << ");" << endl;
                *out << "reader.readParameter<D3DLOCKED_RECT_COMP_WIN32>(&"
                    << "ov_" << param.name << ");" << endl;
                *out << "spv_" << param.name << " = " << "(opv_" << param.name << " == 0 ) ? 0: &sv_"
                    << param.name << ";" << endl;
                *out << "sv_" << param.name << ".Pitch = " << "ov_" << param.name << ".Pitch;" << endl;
                *out << "sv_" << param.name << ".pBits = " << "(void *) pointer(ov_" << param.name << ".pBits);" << endl;
                *out << endl;
            }
            else if (isLockedBox)
            {
                /***************************
                EXAMPLE:

                    D3DLOCKED_BOX * pLockedBox;

                    PIXPointer opv_pLockedBox;
                    D3DLOCKED_BOX_COMP_WIN32 ov_pLockedBox;
                    D3DLOCKED_BOX * spv_pLockedBox;
                    D3DLOCKED_BOX   sv_pLockedBox;
                    reader.readParameter<PIXPointer>(&opv_pLockedBox);
                    reader.readParameter<D3DLOCKED_BOX_COMP_WIN32>(&ov_pLockedBox);
                    spv_pLockedBox = ((opv_pLockedBox == 0 ) ? 0 : &sv_pLockedBox);
                    sv_pLockedBox.RowPitch = ov_pLockedBox.RowPitch;
                    sv_pLockedBox.SlicePitch = ov_pLockedBox.SlicePitch;
                    sv_pLockedBox.pBits = (void *) pointer(ov_pLockedBox.pBits);
                ***************************/
                *out << "PIXPointer opv_" << param.name << ";" << endl;
                *out << "D3DLOCKED_BOX_COMP_WIN32 ov_" << param.name << ";" << endl;
                *out << param.type << " spv_" << param.name << ";" << endl;
                *out << removeLastAsterisk(param.type) << " sv_" << param.name << ";" << endl;
                *out << "reader.readParameter<PIXPointer>(&"
                    << "opv_" << param.name << ");" << endl;
                *out << "reader.readParameter<D3DLOCKED_BOX_COMP_WIN32>(&"
                    << "ov_" << param.name << ");" << endl;
                *out << "spv_" << param.name << " = " << "(opv_" << param.name << " == 0 ) ? 0: &sv_"
                    << param.name << ";" << endl;
                *out << "sv_" << param.name << ".RowPitch = " << "ov_" << param.name << ".RowPitch;" << endl;
                *out << "sv_" << param.name << ".SlicePitch = " << "ov_" << param.name << ".SlicePitch;" << endl;
                *out << "sv_" << param.name << ".pBits = " << "(void *) pointer(ov_" << param.name << ".pBits);" << endl;
                *out << endl;
            }
            else
            {
                /***************************
                EXAMPLE:

                    D3DMATRIX * pMatrix;

                    PIXPointer opv_pMatrix;
                    D3DMATRIX   ov_pMatrix;
                    D3DMATRIX * spv_pMatrix;
                    D3DMATRIX   sv_pMatrix;
                    reader.readParameter<PIXPointer>(&opv_pMatrix);
                    reader.readParameter<D3DMATRIX>(&ov_pMatrix);
                    spv_pMatrix = ((opv_pMatrix == 0 ) ? 0 : &sv_pMatrix);
                    sv_pMatrix = ov_pMatrix;
                ***************************/
                *out << "PIXPointer opv_" << param.name << ";" << endl;
                *out << removeLastAsterisk(param.type) << " ov_" << param.name << ";" << endl;
                *out << param.type << " spv_" << param.name << ";" << endl;
                *out << removeLastAsterisk(param.type) << " sv_" << param.name << ";" << endl;
                *out << "reader.readParameter<PIXPointer>(&"
                    << "opv_" << param.name << ");" << endl;
                *out << "reader.readParameter<" << removeLastAsterisk(param.type) << ">(&"
                    << "ov_" << param.name << ");" << endl;
                *out << "spv_" << param.name << " = " << "(opv_" << param.name << " == 0 ) ? 0: &sv_"
                    << param.name << ";" << endl;
                *out << "sv_" << param.name << " = " << "ov_" << param.name << ";"
                    << endl << endl;
            }

            break;

        case INTERFACE_POINTER_PARAMETER:

            /***************************
            EXAMPLE:

                IDirect3DTexture9 * pTexture;

                PIXPointer oip_pTexture;
                IDirect3DTexture9 * sip_pTexture;
                reader.readParameter<PIXPointer>(&oip_pTexture);
                sip_pTexture = static_cast<IDirect3DTexture9 *>(status.getSubstitute(oip_pTexture));
            ***************************/

            *out << "PIXPointer oip_" << param.name << ";" << endl;
            *out << param.type << " sip_" << param.name << ";" << endl;
            *out << "reader.readParameter<PIXPointer>(&"
                << "oip_" << param.name << ");" << endl;
            *out << "sip_" << param.name << " = " << "static_cast<"
                << param.type << ">(status.getSubstitute(oip_"
                << param.name << "));"
                << endl << endl;
            break;
        case POINTER_INTERFACE_POINTER_PARAMETER:

            /***************************
            EXAMPLE:

                IDirect3DTexture9 * ppTexture;

                PIXPointer opip_ppTexture;
                PIXPointer oip_ppTexture;
                IDirect3DTexture9 ** spip_ppTexture;
                IDirect3DTexture9 *sip_ppTexture;
                reader.readParameter<PIXPointer>(&opip_ppTexture);
                reader.readParameter<PIXPointer>(&oip_ppTexture);
                spip_ppTexture = &sip_ppTexture;
            *****************************/

            *out << "PIXPointer opip_" << param.name  << ";" << endl;
            *out << "PIXPointer oip_" << param.name << ";" << endl;
            *out << param.type << " spip_" << param.name  << ";" << endl;
            *out << removeLastAsterisk(param.type) << " sip_" << param.name << ";" << endl;

            *out << "reader.readParameter<PIXPointer>(&"
                << "opip_" << param.name << ");" << endl;
            *out << "reader.readParameter<PIXPointer>(&"
                << "oip_" << param.name << ");" << endl;
            *out << "spip_" << param.name << " = " << "&sip_" << param.name << ";"
                << endl << endl;
            break;
    }

}

string PerParameterGenerator::removeConst( string s) {
    size_t offset = s.find("CONST ");
    if( offset != s.npos )
        return s.substr(offset + strlen("CONST ") - 1 );
    else return s;
}

string PerParameterGenerator::translateReferencesToGuid(string s) {
    if ( ( s.find("REFGUID") != s.npos ) | (s.find("REFIID") != s.npos ) )
        return "GUID";
    else
        return s;
}

bool PerParameterGenerator::isIrregularPointer(string s) {
    return (
        (s.find("HANDLE") != string::npos) |
        (s.find("RGNDATA") != string::npos) |
        (s.find("D3DVERTEXELEMENT9") != string::npos) |
        (s.find("DWORD") != string::npos)
        );
}


string PerParameterGenerator::removeLastAsterisk( string s) {
    size_t offset = s.find_last_of('*');
    return s.substr(0, offset);
}

void PerParameterGenerator::generateCodeAfterCall(ofstream *out) {
    switch (getType()) {
        case POINTER_INTERFACE_POINTER_PARAMETER:
			*out << "if (sv_Return == D3D_OK)" << endl;
            *out << "    status.setSubstitute("
                << "oip_" << param.name << ", "
                << "sip_" << param.name << ");"
                << endl << endl;
			break;
    }
}

void PerParameterGenerator::generateCodeForCall(ofstream *out) {
    switch(getType()) {
        case VALUE_PARAMETER:
            *out << "sv_" << param.name ;
            break;
        case INTERFACE_POINTER_PARAMETER:
            *out << "sip_" << param.name;
            break;
        case POINTER_INTERFACE_POINTER_PARAMETER:
            *out << "spip_" << param.name;
            break;
        case VALUE_POINTER_PARAMETER:
            *out << "spv_" << param.name;
            break;
    }
}

PerParameterGenerator::ParameterType PerParameterGenerator::getType() {

    bool isInterface = (( param.type.find("IDirect3D9")                  != param.type.npos ) |
                        ( param.type.find("IDirect3DDevice9")            != param.type.npos ) |
                        ( param.type.find("IDirect3DSwapChain9")         != param.type.npos ) |
                        ( param.type.find("IDirect3DTexture9")           != param.type.npos ) |
                        ( param.type.find("IDirect3DVolumeTexture9")     != param.type.npos ) |
                        ( param.type.find("IDirect3DCubeTexture9")       != param.type.npos ) |
                        ( param.type.find("IDirect3DVertexBuffer9")      != param.type.npos ) |
                        ( param.type.find("IDirect3DIndexBuffer9")       != param.type.npos ) |
                        ( param.type.find("IDirect3DSurface9")           != param.type.npos ) |
                        ( param.type.find("IDirect3DVolume9")            != param.type.npos ) |
                        ( param.type.find("IDirect3DVertexDeclaration9") != param.type.npos ) |
                        ( param.type.find("IDirect3DVertexShader9")      != param.type.npos ) |
                        ( param.type.find("IDirect3DPixelShader9")       != param.type.npos ) |
                        ( param.type.find("IDirect3DStateBlock9")        != param.type.npos ) |
                        ( param.type.find("IDirect3DQuery9")             != param.type.npos ) |
                        ( param.type.find("IDirect3DBaseTexture9")       != param.type.npos ) |
                        ( param.type.find("IDirect3DResource9")          != param.type.npos )
                        );

    bool isVoid = (param.type.find("void") != param.type.npos);

    if (param.type.find_last_of('*') != param.type.npos ) {
        if (isIrregularPointer(param.type))
            return VALUE_PARAMETER;
        else {
            string derreferenced = removeLastAsterisk(param.type);
            if (derreferenced.find_last_of('*') != param.type.npos ) {
                if( isInterface ) return POINTER_INTERFACE_POINTER_PARAMETER;
                else return VALUE_PARAMETER;
            }
            else {
                if( isInterface ) return INTERFACE_POINTER_PARAMETER;
                else if(isVoid) return VALUE_PARAMETER;
                else return VALUE_POINTER_PARAMETER;
            }
        }
    }
    else
        return VALUE_PARAMETER;
}

void PerMethodGenerator::generateSwitchBranch(ofstream *out) {
        *out    << "case " << composeFieldName()  << ":" << endl
                << composeMethodName() << "();" << endl
                << "break;" << endl;
}

void PerMethodGenerator::generateEnumField(ofstream *out) {
    *out << composeFieldName() << " = " << exMethod.CID << "," << endl;
}

void PerMethodGenerator::generateDeclaration(ofstream *out) {
    *out << "void " << composeMethodName() << "();" << endl;
}

void PerMethodGenerator::generateDefinition(ofstream *out) {

    *out << "void D3D9PixRunPlayer::" << composeMethodName() << "() {" << endl << endl;

    // Panic
    *out << "#ifdef " << composeMethodName() << "_PANIC" << endl;
    *out << "panic(\"D3D9PixRunPlayer\", \"" << composeMethodName() << "\""
            ", \"Not supported D3D9 call found in trace file.\");" << endl;
    *out << "#endif" << endl << endl;

    // Variables needed for autogeneration
    bool isVoidReturn = false;
    bool isHRESULTReturn = false;
    bool hasParams = false;
    MethodDescriptionParam returnDescription;
    MethodDescriptionParam thisDescription;

    // Update variables needed by autogeneration
    if(exMethod.autogenerate) {
        isVoidReturn = (method.GetType().find("void") != method.GetType().npos);
        isHRESULTReturn = (method.GetType().find("HRESULT") != method.GetType().npos);
        returnDescription.type = method.GetType();
        returnDescription.name = "Return";
        thisDescription.type = exMethod.ClassName;
        thisDescription.type.append(" *");
        thisDescription.name = "This";

        /** @fix
         *  ConfigParser returns a param with name
         * "void" when there are no params.
         */
        hasParams = method.GetParam(0).name != "void";
    }


    // GENERATE CODE BEFORE CALL

    if(exMethod.autogenerate) {
        // Auto generate a parameter for hold the return value if needed
        if( !isVoidReturn ) {
            perParameterGenerator.setParameter(returnDescription);
            perParameterGenerator.generateCodeBeforeCall(out);
        }
        //  Auto generate a parameter for hold 'this' pointer
        perParameterGenerator.setParameter(thisDescription);
        perParameterGenerator.generateCodeBeforeCall(out);

        // Auto generate the others
        if( hasParams ) {
            for(unsigned int i = 0;i < method.GetParamsCount(); i++) {
                perParameterGenerator.setParameter(method.GetParam(i));
                perParameterGenerator.generateCodeBeforeCall(out);
            }
        }
    }

    *out << "#ifdef " << composeMethodName() << "_SPECIFIC_PRE" << endl;
    *out << composeMethodName() << "_SPECIFIC_PRE" << endl;
    *out << "#endif" << endl << endl;


    *out << "#ifdef " << composeMethodName() << "_USER_PRE" << endl;
    *out << composeMethodName() << "_USER_PRE" << endl;
    *out << "#endif" << endl << endl;

    // GENERATE CODE FOR THE CALL

    if(exMethod.autogenerate)
    {
        //  Generate check for the interface pointer.
        *out << "if (";
        perParameterGenerator.setParameter(thisDescription);
        perParameterGenerator.generateCodeForCall(out);
        *out << " == NULL)" << endl;
        *out << "{" << endl;
        *out << "    includelog::logfile().write(includelog::Panic, \"Ignoring call to NULL interface pointer for ";
        *out << composeMethodName() << " interface call.\\n\");" << endl;
        *out << "    if (!status.isEnabledContinueOnNULLHack())" << endl;
        *out << "        panic(\"D3D9PixRunPlayer\", \"" << composeMethodName() << "\""
            ", \"Calling to D3D9 interface with NULL pointer.\");" << endl;
        if (!isVoidReturn)
        {
            *out << "    sv_Return = -1;" << endl;
        }
        *out << "}" << endl;
        *out << "else" << endl;
        *out << "    ";

        if(!isVoidReturn)
        {
            perParameterGenerator.setParameter(returnDescription);
            perParameterGenerator.generateCodeForCall(out);
            *out << " = ";
        }


        perParameterGenerator.setParameter(thisDescription);
        perParameterGenerator.generateCodeForCall(out);

        *out << " -> ";

        *out << method.GetName() << "(" << endl;
        if( hasParams ) {
            for(unsigned int i = 0;i < method.GetParamsCount(); i++) {
                *out << ((i == 0)?"":" ,");
                perParameterGenerator.setParameter(method.GetParam(i));
                perParameterGenerator.generateCodeForCall(out);
            }
        }
        *out << ");" << endl << endl;
    }

    // GENERATE CODE AFTER THE CALL

    *out << "#ifdef " << composeMethodName() << "_USER_POST" << endl;
    *out << composeMethodName() << "_USER_POST" << endl;
    *out << "#endif" << endl << endl;

    *out << "#ifdef " << composeMethodName() << "_SPECIFIC_POST" << endl;
    *out << composeMethodName() << "_SPECIFIC_POST" << endl;
    *out << "#endif" << endl << endl;

    //  Generate code for release.
    if(exMethod.autogenerate)
    {
        if (method.GetName().compare("Release") == 0)
        {
            *out << "if (sv_Return == 0)" << endl;
            *out << "    status.removeSubstitute(oip_" << thisDescription.name << ");" << endl;
        }
    }
        
    if(exMethod.autogenerate)
    {
        if (isHRESULTReturn)
        {
            *out << "if (sv_Return != D3D_OK)" << endl;
            *out << "    includelog::logfile().write(includelog::Panic, \"Call to ";
            *out << composeMethodName() << " didn't return OK.\\n\");" << endl << endl;
        }
    }
        
    if(exMethod.autogenerate) {
        if( hasParams ) {
            for(unsigned int i = 0;i < method.GetParamsCount(); i++) {
                perParameterGenerator.setParameter(method.GetParam(i));
                perParameterGenerator.generateCodeAfterCall(out);
            }
        }
    }

    *out << "}" << endl << endl;
}

void PerMethodGenerator::setMethodInfo( Items::MethodDescription &md,
            ExtendedMethodDescription &exmd) {
    exMethod = exmd;
    method = md;
}

string PerMethodGenerator::composeMethodName() {

    string className = exMethod.ClassName;
    string methodName = method.GetName();
    String::ToUpper(className);
    String::ToUpper(methodName);
    string result = "D3D9OP_";
    if(className.length() > 0) {
        result.append(className);
        result.append("_");
    }
    result.append(methodName);

    return result;
}

string PerMethodGenerator::composeFieldName() {

    // Uppercase
    string className = exMethod.ClassName;
    String::ToUpper( className );
    string methodName = method.GetName();
    String::ToUpper( methodName );

    // Concat
    string result;
    result.append("D3D9CID_");
    if(className.length() > 0) {
        result.append(className);
        result.append("_");
    }
    result.append(methodName);

    return result;
}


void D3DPixPlayerCodeGenerator::GenerateCode()
{
    // Open streams
    string &path   = configuration.GetOutputPath();
    string pathDec = path;
    string pathDef = path;
    string pathSw  = path;
    string pathEn  = path;

    pathDec.append("D3D9PixRunDeclarations.gen");
    pathDef.append("D3D9PixRunDefinitions.gen");
    pathSw.append("D3D9PixRunSwitchBranches.gen");
    pathEn.append("D3D9PixRunEnum.gen");

    ofstream *outDec = CreateFilename(pathDec);
    ofstream *outDef = CreateFilename(pathDef);
    ofstream *outSw = CreateFilename(pathSw);
    ofstream *outEn = CreateFilename(pathEn);


    // Iterate through classes and methods generating code using method generator
    vector< ClassDescription > :: iterator itC;
    for( itC = classes.begin(); itC != classes.end(); itC++ )
    {
        for( unsigned int i = 0; i < (*itC).GetMethodsCount(); i++ )
        {
            // Get CID of this method
            string methodClassName = (*itC).GetName();
            string methodName = (*itC).GetMethod(i).GetName();
            pair<string, string> name;
            name.first = methodClassName;
            name.second = methodName;
            int CID = nameToCID[name];

            // Get extended method description
            ExtendedMethodDescription exMethod = extendedMethodDescriptions[CID];


            // Configure perMethodGenerator
            perMethodGenerator.setMethodInfo((*itC).GetMethod(i), exMethod);

            //Generate code
            perMethodGenerator.generateDeclaration(outDec);
            perMethodGenerator.generateDefinition(outDef);
            perMethodGenerator.generateSwitchBranch(outSw);
            perMethodGenerator.generateEnumField(outEn);
        }
    }
    // Free resources
    CloseFilename(outDec);
    CloseFilename(outDef);
    CloseFilename(outSw);
    CloseFilename(outEn);
}



void D3DPixPlayerCodeGenerator::setClasses(vector<ClassDescription> &c) {
    classes = c;
    onSetClasses();
}

void D3DPixPlayerCodeGenerator::onSetClasses() 
{
    /***************************************************************
    Global methods (p.e. Direct3DCreate9) not listed by ConfigParser,
    we will hardcode its info in a additional class with empty name.
    ***************************************************************/
    ClassDescription global;
    global.SetName("");

    MethodDescription method;
    MethodDescriptionParam param;

    method.SetName("Direct3DCreate9");
    method.SetType("IDirect3D9 *");
    param.name = "SDKVersion";
    param.type = "UINT";
    method.AddParam(param);
    global.AddMethod(method);

    method.SetName("DebugSetMute");
    method.SetType("");
    global.AddMethod(method);

    method.SetName("UNUSED_208");
    method.SetType("");
    global.AddMethod(method);

    method.SetName("Direct3DShaderValidatorCreate9");
    method.SetType("");
    global.AddMethod(method);

    method.SetName("UNUSED_210");
    method.SetType("");
    global.AddMethod(method);

    method.SetName("UNUSED_211");
    method.SetType("");
    global.AddMethod(method);

    method.SetName("D3DPERF_BeginEvent");
    method.SetType("");
    global.AddMethod(method);

    method.SetName("D3DPERF_EndEvent");
    method.SetType("");
    global.AddMethod(method);

    method.SetName("D3DPERF_SetMarker");
    method.SetType("");
    global.AddMethod(method);

    method.SetName("D3DPERF_SetRegion");
    method.SetType("");
    global.AddMethod(method);

    method.SetName("D3DPERF_QueryRepeatFrame");
    method.SetType("");
    global.AddMethod(method);

    method.SetName("D3DPERF_SetOptions");
    method.SetType("");
    global.AddMethod(method);

    method.SetName("D3DPERF_GetStatus");
    method.SetType("");
    global.AddMethod(method);

    classes.push_back(global);

    // Class methods as have correlative CID's, we must only hardcode
    // the first CID of each class.
    std::map< std::string , int > classToInitialCID;
    // Direct3D 9
    classToInitialCID[ ""                            ] = 206;
    classToInitialCID[ "IDirect3D9"                  ] = 219;
    classToInitialCID[ "IDirect3DDevice9"            ] = 236;
    classToInitialCID[ "IDirect3DSwapChain9"         ] = 355;
    classToInitialCID[ "IDirect3DTexture9"           ] = 365;
    classToInitialCID[ "IDirect3DVolumeTexture9"     ] = 387;
    classToInitialCID[ "IDirect3DCubeTexture9"       ] = 409;
    classToInitialCID[ "IDirect3DVertexBuffer9"      ] = 431;
    classToInitialCID[ "IDirect3DIndexBuffer9"       ] = 445;
    classToInitialCID[ "IDirect3DSurface9"           ] = 459;
    classToInitialCID[ "IDirect3DVolume9"            ] = 476;
    classToInitialCID[ "IDirect3DVertexDeclaration9" ] = 487;
    classToInitialCID[ "IDirect3DVertexShader9"      ] = 492;
    classToInitialCID[ "IDirect3DPixelShader9"       ] = 497;
    classToInitialCID[ "IDirect3DStateBlock9"        ] = 502;
    classToInitialCID[ "IDirect3DQuery9"             ] = 508;
    classToInitialCID[ "IDirect3DVideoDevice9"       ] = 516;
    classToInitialCID[ "IDirect3DDXVADevice9"        ] = 525;
    // Direct3D 11
    classToInitialCID["D3D11CreateDevice"            ] = 2000;
    classToInitialCID["D3D11CreateDeviceAndSwapChain"] = 2001;
    classToInitialCID["ID3D11DepthStencilState"      ] = 2011;
    classToInitialCID["ID3D11BlendState"             ] = 2019;
    classToInitialCID["ID3D11RasterizerState"        ] = 2027;
    classToInitialCID["ID3D11Buffer"                 ] = 2035;
    classToInitialCID["ID3D11Texture1D"              ] = 2046;
    classToInitialCID["ID3D11Texture2D"              ] = 2057;
    classToInitialCID["ID3D11Texture3D"              ] = 2068;
    classToInitialCID["ID3D11ShaderResourceView"     ] = 2079;
    classToInitialCID["ID3D11RenderTargetView"       ] = 2088;
    classToInitialCID["ID3D11DepthStencilView"       ] = 2097;
    classToInitialCID["ID3D11UnorderedAccessView"    ] = 2106;
    classToInitialCID["ID3D11VertexShader"           ] = 2115;
    classToInitialCID["ID3D11HullShader"             ] = 2122;
    classToInitialCID["ID3D11DomainShader"           ] = 2129;
    classToInitialCID["ID3D11GeometryShader"         ] = 2136;
    classToInitialCID["ID3D11PixelShader"            ] = 2143;
    classToInitialCID["ID3D11ComputeShader"          ] = 2150;
    classToInitialCID["ID3D11InputLayout"            ] = 2157;
    classToInitialCID["ID3D11SamplerState"           ] = 2164;
    classToInitialCID["ID3D11Query"                  ] = 2172;
    classToInitialCID["ID3D11Predicate"              ] = 2181;
    classToInitialCID["ID3D11Counter"                ] = 2190;
    classToInitialCID["ID3D11ClassInstance"          ] = 2199;
    classToInitialCID["ID3D11ClassLinkage"           ] = 2210;
    classToInitialCID["ID3D11CommandList"            ] = 2219;
    classToInitialCID["ID3D11DeviceContext"          ] = 2228;
    classToInitialCID["ID3D11Device"                 ] = 2342;

    /**************************************************************
    Iterate through classes and methods assigning CID's and names
    **************************************************************/
    vector< ClassDescription > :: iterator itC;

    for( itC = classes.begin(); itC != classes.end(); itC++ ) {
        string className = (*itC).GetName();
        int iniCID = classToInitialCID[ className ];
        for( unsigned int i = 0; i < (*itC).GetMethodsCount(); i++ ) {
                string methodName = (*itC).GetMethod(i).GetName();
                nameToCID[pair<string, string>(className,methodName)] = classToInitialCID[ className ] + i;
        }
    }

    /**************************************************************
    Iterate through classes and methods assigning default extended method description
    **************************************************************/

    for( itC = classes.begin(); itC != classes.end(); itC++ ) {
        string className = (*itC).GetName();
        int iniCID = classToInitialCID[ className ];
        for( unsigned int i = 0; i < (*itC).GetMethodsCount(); i++ ) {
                string methodName = (*itC).GetMethod(i).GetName();
                ExtendedMethodDescription extended;
                extended.CID = nameToCID[pair<string, string>(className,methodName)];
                extended.ClassName = className;
                extendedMethodDescriptions[extended.CID] = extended;
        }
    }

    // Hardcode extended method description for some operations
    // Autogenerated                                       // 219: "IDirect3D9::QueryInterface",                                          
    extendedMethodDescriptions[220].autogenerate = true;   // 220: "IDirect3D9::AddRef",
    extendedMethodDescriptions[221].autogenerate = true;   // 221: "IDirect3D9::Release",
       // 222: "IDirect3D9::RegisterSoftwareDevice",
       // 223: "IDirect3D9::GetAdapterCount",
       // 224: "IDirect3D9::GetAdapterIdentifier",
       // 225: "IDirect3D9::GetAdapterModeCount",
       // 226: "IDirect3D9::EnumAdapterModes",
       // 227: "IDirect3D9::GetAdapterDisplayMode",
       // 228: "IDirect3D9::CheckDeviceType",
       // 229: "IDirect3D9::CheckDeviceFormat",
       // 230: "IDirect3D9::CheckDeviceMultiSampleType",
       // 231: "IDirect3D9::CheckDepthStencilMatch",
       // 232: "IDirect3D9::CheckDeviceFormatConversion",
       // 233: "IDirect3D9::GetDeviceCaps",
       // 234: "IDirect3D9::GetAdapterMonitor",
    extendedMethodDescriptions[235].autogenerate = true;   // 235: "IDirect3D9::CreateDevice",
       // 236: "IDirect3DDevice9::QueryInterface",
    extendedMethodDescriptions[237].autogenerate = true;   // 237: "IDirect3DDevice9::AddRef",
    extendedMethodDescriptions[238].autogenerate = true;   // 238: "IDirect3DDevice9::Release",
    extendedMethodDescriptions[241].autogenerate = true;   // 239: "IDirect3DDevice9::TestCooperativeLevel",
    extendedMethodDescriptions[242].autogenerate = true;   // 240: "IDirect3DDevice9::GetAvailableTextureMem",
    extendedMethodDescriptions[246].autogenerate = true;   // 241: "IDirect3DDevice9::EvictManagedResources",
    extendedMethodDescriptions[247].autogenerate = true;   // 242: "IDirect3DDevice9::GetDirect3D",
    extendedMethodDescriptions[248].autogenerate = true;   // 243: "IDirect3DDevice9::GetDeviceCaps",
    extendedMethodDescriptions[249].autogenerate = true;   // 244: "IDirect3DDevice9::GetDisplayMode",
    extendedMethodDescriptions[250].autogenerate = true;   // 245: "IDirect3DDevice9::GetCreationParameters",
    extendedMethodDescriptions[252].autogenerate = true;   // 246: "IDirect3DDevice9::SetCursorProperties",
    extendedMethodDescriptions[253].autogenerate = true;   // 247: "IDirect3DDevice9::SetCursorPosition",
    extendedMethodDescriptions[254].autogenerate = true;   // 248: "IDirect3DDevice9::ShowCursor",
    extendedMethodDescriptions[256].autogenerate = true;   // 249: "IDirect3DDevice9::CreateAdditionalSwapChain",
    extendedMethodDescriptions[257].autogenerate = true;   // 250: "IDirect3DDevice9::GetSwapChain",
    extendedMethodDescriptions[259].autogenerate = true;   // 251: "IDirect3DDevice9::GetNumberOfSwapChains",
    extendedMethodDescriptions[260].autogenerate = true;   // 252: "IDirect3DDevice9::Reset",
    extendedMethodDescriptions[261].autogenerate = true;   // 253: "IDirect3DDevice9::Present",
    extendedMethodDescriptions[262].autogenerate = true;   // 254: "IDirect3DDevice9::GetBackBuffer",
    extendedMethodDescriptions[263].autogenerate = true;   // 255: "IDirect3DDevice9::GetRasterStatus",
    extendedMethodDescriptions[264].autogenerate = true;   // 256: "IDirect3DDevice9::SetDialogBoxMode",
    extendedMethodDescriptions[265].autogenerate = true;   // 257: "IDirect3DDevice9::SetGammaRamp",
    extendedMethodDescriptions[266].autogenerate = true;   // 258: "IDirect3DDevice9::GetGammaRamp",
    extendedMethodDescriptions[267].autogenerate = true;   // 259: "IDirect3DDevice9::CreateTexture",
    extendedMethodDescriptions[270].autogenerate = true;   // 260: "IDirect3DDevice9::CreateVolumeTexture",
    extendedMethodDescriptions[271].autogenerate = true;   // 261: "IDirect3DDevice9::CreateCubeTexture",
    extendedMethodDescriptions[272].autogenerate = true;   // 262: "IDirect3DDevice9::CreateVertexBuffer",
    extendedMethodDescriptions[273].autogenerate = true;   // 263: "IDirect3DDevice9::CreateIndexBuffer",
    extendedMethodDescriptions[274].autogenerate = true;   // 264: "IDirect3DDevice9::CreateRenderTarget",
    extendedMethodDescriptions[275].autogenerate = true;   // 265: "IDirect3DDevice9::CreateDepthStencilSurface",
    extendedMethodDescriptions[276].autogenerate = true;   // 266: "IDirect3DDevice9::UpdateSurface",
    extendedMethodDescriptions[277].autogenerate = true;   // 267: "IDirect3DDevice9::UpdateTexture",
    extendedMethodDescriptions[278].autogenerate = true;   // 268: "IDirect3DDevice9::GetRenderTargetData",
    extendedMethodDescriptions[279].autogenerate = true;   // 269: "IDirect3DDevice9::GetFrontBufferData",
    extendedMethodDescriptions[280].autogenerate = true;   // 270: "IDirect3DDevice9::StretchRect",
    extendedMethodDescriptions[282].autogenerate = true;   // 271: "IDirect3DDevice9::ColorFill",
    extendedMethodDescriptions[283].autogenerate = true;   // 272: "IDirect3DDevice9::CreateOffscreenPlainSurface",
    extendedMethodDescriptions[285].autogenerate = true;   // 273: "IDirect3DDevice9::SetRenderTarget",
    extendedMethodDescriptions[287].autogenerate = true;   // 274: "IDirect3DDevice9::GetRenderTarget",
    extendedMethodDescriptions[289].autogenerate = true;   // 275: "IDirect3DDevice9::SetDepthStencilSurface",
    extendedMethodDescriptions[291].autogenerate = true;   // 276: "IDirect3DDevice9::GetDepthStencilSurface",
    extendedMethodDescriptions[293].autogenerate = true;   // 277: "IDirect3DDevice9::BeginScene",
    extendedMethodDescriptions[295].autogenerate = true;   // 278: "IDirect3DDevice9::EndScene",
    extendedMethodDescriptions[296].autogenerate = true;   // 279: "IDirect3DDevice9::Clear",
    extendedMethodDescriptions[297].autogenerate = true;   // 280: "IDirect3DDevice9::SetTransform",
    extendedMethodDescriptions[298].autogenerate = true;   // 281: "IDirect3DDevice9::GetTransform",
    extendedMethodDescriptions[300].autogenerate = true;   // 282: "IDirect3DDevice9::MultiplyTransform",
    extendedMethodDescriptions[301].autogenerate = true;   // 283: "IDirect3DDevice9::SetViewport",
    extendedMethodDescriptions[303].autogenerate = true;   // 284: "IDirect3DDevice9::GetViewport",
    extendedMethodDescriptions[305].autogenerate = true;   // 285: "IDirect3DDevice9::SetMaterial",
    extendedMethodDescriptions[307].autogenerate = true;   // 286: "IDirect3DDevice9::GetMaterial",
    extendedMethodDescriptions[309].autogenerate = true;   // 287: "IDirect3DDevice9::SetLight",
    extendedMethodDescriptions[311].autogenerate = true;   // 288: "IDirect3DDevice9::GetLight",
    extendedMethodDescriptions[313].autogenerate = true;   // 289: "IDirect3DDevice9::LightEnable",
    extendedMethodDescriptions[315].autogenerate = true;   // 290: "IDirect3DDevice9::GetLightEnable",
    extendedMethodDescriptions[317].autogenerate = true;   // 291: "IDirect3DDevice9::SetClipPlane",
    extendedMethodDescriptions[318].autogenerate = true;   // 292: "IDirect3DDevice9::GetClipPlane",
    extendedMethodDescriptions[319].autogenerate = true;   // 293: "IDirect3DDevice9::SetRenderState",
    extendedMethodDescriptions[320].autogenerate = true;   // 294: "IDirect3DDevice9::GetRenderState",
    extendedMethodDescriptions[321].autogenerate = true;   // 295: "IDirect3DDevice9::CreateStateBlock",
    extendedMethodDescriptions[322].autogenerate = true;   // 296: "IDirect3DDevice9::BeginStateBlock",
    extendedMethodDescriptions[323].autogenerate = true;   // 297: "IDirect3DDevice9::EndStateBlock",
    extendedMethodDescriptions[324].autogenerate = true;   // 298: "IDirect3DDevice9::SetClipStatus",
    extendedMethodDescriptions[325].autogenerate = true;   // 299: "IDirect3DDevice9::GetClipStatus",
    extendedMethodDescriptions[327].autogenerate = true;   // 300: "IDirect3DDevice9::GetTexture",
    extendedMethodDescriptions[328].autogenerate = true;   // 301: "IDirect3DDevice9::SetTexture",
    extendedMethodDescriptions[329].autogenerate = true;   // 302: "IDirect3DDevice9::GetTextureStageState",
    extendedMethodDescriptions[330].autogenerate = true;   // 303: "IDirect3DDevice9::SetTextureStageState",
    extendedMethodDescriptions[332].autogenerate = true;   // 304: "IDirect3DDevice9::GetSamplerState",
    extendedMethodDescriptions[334].autogenerate = true;   // 305: "IDirect3DDevice9::SetSamplerState",
    extendedMethodDescriptions[336].autogenerate = true;   // 306: "IDirect3DDevice9::ValidateDevice",
    extendedMethodDescriptions[337].autogenerate = true;   // 307: "IDirect3DDevice9::SetPaletteEntries",
    extendedMethodDescriptions[338].autogenerate = true;   // 308: "IDirect3DDevice9::GetPaletteEntries",
    extendedMethodDescriptions[340].autogenerate = true;   // 309: "IDirect3DDevice9::SetCurrentTexturePalette",
    extendedMethodDescriptions[341].autogenerate = true;   // 310: "IDirect3DDevice9::GetCurrentTexturePalette",
    extendedMethodDescriptions[342].autogenerate = true;   // 311: "IDirect3DDevice9::SetScissorRect",
    extendedMethodDescriptions[343].autogenerate = true;   // 312: "IDirect3DDevice9::GetScissorRect",
    extendedMethodDescriptions[344].autogenerate = true;   // 313: "IDirect3DDevice9::SetSoftwareVertexProcessing",
    extendedMethodDescriptions[345].autogenerate = true;   // 314: "IDirect3DDevice9::GetSoftwareVertexProcessing",
    extendedMethodDescriptions[347].autogenerate = true;   // 315: "IDirect3DDevice9::SetNPatchMode",
    extendedMethodDescriptions[349].autogenerate = true;   // 316: "IDirect3DDevice9::GetNPatchMode",
    extendedMethodDescriptions[353].autogenerate = true;   // 317: "IDirect3DDevice9::DrawPrimitive",
    extendedMethodDescriptions[356].autogenerate = true;   // 318: "IDirect3DDevice9::DrawIndexedPrimitive",
    extendedMethodDescriptions[357].autogenerate = true;   // 319: "IDirect3DDevice9::DrawPrimitiveUP",
    extendedMethodDescriptions[358].autogenerate = true;   // 320: "IDirect3DDevice9::DrawIndexedPrimitiveUP",
    extendedMethodDescriptions[360].autogenerate = true;   // 321: "IDirect3DDevice9::ProcessVertices",
    extendedMethodDescriptions[363].autogenerate = true;   // 322: "IDirect3DDevice9::CreateVertexDeclaration",
    extendedMethodDescriptions[366].autogenerate = true;   // 323: "IDirect3DDevice9::SetVertexDeclaration",
    extendedMethodDescriptions[367].autogenerate = true;   // 324: "IDirect3DDevice9::GetVertexDeclaration",
    extendedMethodDescriptions[368].autogenerate = true;   // 325: "IDirect3DDevice9::SetFVF",
    extendedMethodDescriptions[372].autogenerate = true;   // 326: "IDirect3DDevice9::GetFVF",
    extendedMethodDescriptions[374].autogenerate = true;   // 327: "IDirect3DDevice9::CreateVertexShader",
    extendedMethodDescriptions[376].autogenerate = true;   // 328: "IDirect3DDevice9::SetVertexShader",
    extendedMethodDescriptions[379].autogenerate = true;   // 329: "IDirect3DDevice9::GetVertexShader",
    extendedMethodDescriptions[381].autogenerate = true;   // 330: "IDirect3DDevice9::SetVertexShaderConstantF",
    extendedMethodDescriptions[383].autogenerate = true;   // 331: "IDirect3DDevice9::GetVertexShaderConstantF",
    extendedMethodDescriptions[384].autogenerate = true;   // 332: "IDirect3DDevice9::SetVertexShaderConstantI",
    extendedMethodDescriptions[385].autogenerate = true;   // 333: "IDirect3DDevice9::GetVertexShaderConstantI",
    extendedMethodDescriptions[386].autogenerate = true;   // 334: "IDirect3DDevice9::SetVertexShaderConstantB",
    extendedMethodDescriptions[388].autogenerate = true;   // 335: "IDirect3DDevice9::GetVertexShaderConstantB",
    extendedMethodDescriptions[389].autogenerate = true;   // 336: "IDirect3DDevice9::SetStreamSource",
    extendedMethodDescriptions[390].autogenerate = true;   // 337: "IDirect3DDevice9::GetStreamSource",
    extendedMethodDescriptions[394].autogenerate = true;   // 338: "IDirect3DDevice9::SetStreamSourceFreq",
    extendedMethodDescriptions[396].autogenerate = true;   // 339: "IDirect3DDevice9::GetStreamSourceFreq",
    extendedMethodDescriptions[398].autogenerate = true;   // 340: "IDirect3DDevice9::SetIndices",
       // 341: "IDirect3DDevice9::GetIndices",
       // 342: "IDirect3DDevice9::CreatePixelShader",
       // 343: "IDirect3DDevice9::SetPixelShader",
       // 344: "IDirect3DDevice9::GetPixelShader",
       // 345: "IDirect3DDevice9::SetPixelShaderConstantF",
       // 346: "IDirect3DDevice9::GetPixelShaderConstantF",
       // 347: "IDirect3DDevice9::SetPixelShaderConstantI",
       // 348: "IDirect3DDevice9::GetPixelShaderConstantI",
       // 349: "IDirect3DDevice9::SetPixelShaderConstantB",
       // 350: "IDirect3DDevice9::GetPixelShaderConstantB",
       // 351: "IDirect3DDevice9::DrawRectPatch",
       // 352: "IDirect3DDevice9::DrawTriPatch",
       // 353: "IDirect3DDevice9::DeletePatch",
       // 354: "IDirect3DDevice9::CreateQuery",
       // 355: "IDirect3DSwapChain9::QueryInterface",
       // 356: "IDirect3DSwapChain9::AddRef",
       // 357: "IDirect3DSwapChain9::Release",
       // 358: "IDirect3DSwapChain9::Present",
       // 359: "IDirect3DSwapChain9::GetFrontBufferData",
       // 360: "IDirect3DSwapChain9::GetBackBuffer",
       // 361: "IDirect3DSwapChain9::GetRasterStatus",
       // 362: "IDirect3DSwapChain9::GetDisplayMode",
       // 363: "IDirect3DSwapChain9::GetDevice",
       // 364: "IDirect3DSwapChain9::GetPresentParameters",
       // 365: "IDirect3DTexture9::QueryInterface",
       // 366: "IDirect3DTexture9::AddRef",
       // 367: "IDirect3DTexture9::Release",
       // 368: "IDirect3DTexture9::GetDevice",
       // 369: "IDirect3DTexture9::SetPrivateData",
       // 370: "IDirect3DTexture9::GetPrivateData",
       // 371: "IDirect3DTexture9::FreePrivateData",
       // 372: "IDirect3DTexture9::SetPriority",
       // 373: "IDirect3DTexture9::GetPriority",
       // 374: "IDirect3DTexture9::PreLoad",
       // 375: "IDirect3DTexture9::GetType",
       // 376: "IDirect3DTexture9::SetLOD",
       // 377: "IDirect3DTexture9::GetLOD",
       // 378: "IDirect3DTexture9::GetLevelCount",
       // 379: "IDirect3DTexture9::SetAutoGenFilterType",
       // 380: "IDirect3DTexture9::GetAutoGenFilterType",
       // 381: "IDirect3DTexture9::GenerateMipSubLevels",
       // 382: "IDirect3DTexture9::GetLevelDesc",
       // 383: "IDirect3DTexture9::GetSurfaceLevel",
       // 384: "IDirect3DTexture9::LockRect",
       // 385: "IDirect3DTexture9::UnlockRect",
       // 386: "IDirect3DTexture9::AddDirtyRect",
                                                           // 387: "IDirect3DVolumeTexture9::QueryInterface",
                                                           // 388: "IDirect3DVolumeTexture9::AddRef",
                                                           // 389: "IDirect3DVolumeTexture9::Release",
                                                           // 390: "IDirect3DVolumeTexture9::GetDevice",
                                                           // 391: "IDirect3DVolumeTexture9::SetPrivateData",
                                                           // 392: "IDirect3DVolumeTexture9::GetPrivateData",
                                                           // 393: "IDirect3DVolumeTexture9::FreePrivateData",
                                                           // 394: "IDirect3DVolumeTexture9::SetPriority",
                                                           // 395: "IDirect3DVolumeTexture9::GetPriority",
                                                           // 396: "IDirect3DVolumeTexture9::PreLoad",
                                                           // 397: "IDirect3DVolumeTexture9::GetType",
                                                           // 398: "IDirect3DVolumeTexture9::SetLOD",
                                                           // 399: "IDirect3DVolumeTexture9::GetLOD",
                                                           // 400: "IDirect3DVolumeTexture9::GetLevelCount",
    extendedMethodDescriptions[401].autogenerate = true;   // 401: "IDirect3DVolumeTexture9::SetAutoGenFilterType",
                                                           // 402: "IDirect3DVolumeTexture9::GetAutoGenFilterType",
    extendedMethodDescriptions[403].autogenerate = true;   // 403: "IDirect3DVolumeTexture9::GenerateMipSubLevels",
                                                           // 404: "IDirect3DVolumeTexture9::GetLevelDesc",
    extendedMethodDescriptions[405].autogenerate = true;   // 405: "IDirect3DVolumeTexture9::GetVolumeLevel",
    extendedMethodDescriptions[406].autogenerate = true;   // 406: "IDirect3DVolumeTexture9::LockBox",
    extendedMethodDescriptions[407].autogenerate = true;   // 407: "IDirect3DVolumeTexture9::UnlockBox",
    extendedMethodDescriptions[408].autogenerate = true;   // 408: "IDirect3DVolumeTexture9::AddDirtyBox",
                                                           // 409: "IDirect3DCubeTexture9::QueryInterface",
    extendedMethodDescriptions[410].autogenerate = true;   // 410: "IDirect3DCubeTexture9::AddRef",
    extendedMethodDescriptions[411].autogenerate = true;   // 411: "IDirect3DCubeTexture9::Release",
    extendedMethodDescriptions[412].autogenerate = true;   // 412: "IDirect3DCubeTexture9::GetDevice",
                                                           // 413: "IDirect3DCubeTexture9::SetPrivateData",
                                                           // 414: "IDirect3DCubeTexture9::GetPrivateData",
                                                           // 415: "IDirect3DCubeTexture9::FreePrivateData",
    extendedMethodDescriptions[416].autogenerate = true;   // 416: "IDirect3DCubeTexture9::SetPriority",
                                                           // 417: "IDirect3DCubeTexture9::GetPriority",
    extendedMethodDescriptions[418].autogenerate = true;   // 418: "IDirect3DCubeTexture9::PreLoad",
                                                           // 419: "IDirect3DCubeTexture9::GetType",
    extendedMethodDescriptions[420].autogenerate = true;   // 420: "IDirect3DCubeTexture9::SetLOD",
                                                           // 421: "IDirect3DCubeTexture9::GetLOD",
                                                           // 422: "IDirect3DCubeTexture9::GetLevelCount",
    extendedMethodDescriptions[423].autogenerate = true;   // 423: "IDirect3DCubeTexture9::SetAutoGenFilterType",
                                                           // 424: "IDirect3DCubeTexture9::GetAutoGenFilterType",
    extendedMethodDescriptions[425].autogenerate = true;   // 425: "IDirect3DCubeTexture9::GenerateMipSubLevels",
                                                           // 426: "IDirect3DCubeTexture9::GetLevelDesc",
    extendedMethodDescriptions[427].autogenerate = true;   // 427: "IDirect3DCubeTexture9::GetCubeMapSurface",
    extendedMethodDescriptions[428].autogenerate = true;   // 428: "IDirect3DCubeTexture9::LockRect",
    extendedMethodDescriptions[429].autogenerate = true;   // 429: "IDirect3DCubeTexture9::UnlockRect",
    extendedMethodDescriptions[430].autogenerate = true;   // 430: "IDirect3DCubeTexture9::AddDirtyRect",
                                                           // 431: "IDirect3DVertexBuffer9::QueryInterface",
    extendedMethodDescriptions[432].autogenerate = true;   // 432: "IDirect3DVertexBuffer9::AddRef",
    extendedMethodDescriptions[433].autogenerate = true;   // 433: "IDirect3DVertexBuffer9::Release",
    extendedMethodDescriptions[434].autogenerate = true;   // 434: "IDirect3DVertexBuffer9::GetDevice",
                                                           // 435: "IDirect3DVertexBuffer9::SetPrivateData",
                                                           // 436: "IDirect3DVertexBuffer9::GetPrivateData",
                                                           // 437: "IDirect3DVertexBuffer9::FreePrivateData",
    extendedMethodDescriptions[438].autogenerate = true;   // 438: "IDirect3DVertexBuffer9::SetPriority",
                                                           // 439: "IDirect3DVertexBuffer9::GetPriority",
    extendedMethodDescriptions[440].autogenerate = true;   // 440: "IDirect3DVertexBuffer9::PreLoad",
                                                           // 441: "IDirect3DVertexBuffer9::GetType",
    extendedMethodDescriptions[442].autogenerate = true;   // 442: "IDirect3DVertexBuffer9::Lock",
    extendedMethodDescriptions[443].autogenerate = true;   // 443: "IDirect3DVertexBuffer9::Unlock",
                                                           // 444: "IDirect3DVertexBuffer9::GetDesc",
                                                           // 445: "IDirect3DIndexBuffer9::QueryInterface",
    extendedMethodDescriptions[446].autogenerate = true;   // 446: "IDirect3DIndexBuffer9::AddRef",
    extendedMethodDescriptions[447].autogenerate = true;   // 447: "IDirect3DIndexBuffer9::Release",
    extendedMethodDescriptions[448].autogenerate = true;   // 448: "IDirect3DIndexBuffer9::GetDevice",
                                                           // 449: "IDirect3DIndexBuffer9::SetPrivateData",
                                                           // 450: "IDirect3DIndexBuffer9::GetPrivateData",
                                                           // 451: "IDirect3DIndexBuffer9::FreePrivateData",
    extendedMethodDescriptions[452].autogenerate = true;   // 452: "IDirect3DIndexBuffer9::SetPriority",
                                                           // 453: "IDirect3DIndexBuffer9::GetPriority",
    extendedMethodDescriptions[454].autogenerate = true;   // 454: "IDirect3DIndexBuffer9::PreLoad",
                                                           // 455: "IDirect3DIndexBuffer9::GetType",
    extendedMethodDescriptions[456].autogenerate = true;   // 456: "IDirect3DIndexBuffer9::Lock",
    extendedMethodDescriptions[457].autogenerate = true;   // 457: "IDirect3DIndexBuffer9::Unlock",
                                                           // 458: "IDirect3DIndexBuffer9::GetDesc",
                                                           // 459: "IDirect3DSurface9::QueryInterface",
    extendedMethodDescriptions[460].autogenerate = true;   // 460: "IDirect3DSurface9::AddRef",
    extendedMethodDescriptions[461].autogenerate = true;   // 461: "IDirect3DSurface9::Release",
    extendedMethodDescriptions[462].autogenerate = true;   // 462: "IDirect3DSurface9::GetDevice",
       // 463: "IDirect3DSurface9::SetPrivateData",
       // 464: "IDirect3DSurface9::GetPrivateData",
       // 465: "IDirect3DSurface9::FreePrivateData",
    extendedMethodDescriptions[466].autogenerate = true;   // 466: "IDirect3DSurface9::SetPriority",
       // 467: "IDirect3DSurface9::GetPriority",
    extendedMethodDescriptions[468].autogenerate = true;   // 468: "IDirect3DSurface9::PreLoad",
                                                           // 469: "IDirect3DSurface9::GetType",
    extendedMethodDescriptions[470].autogenerate = true;   // 470: "IDirect3DSurface9::GetContainer",
                                                           // 471: "IDirect3DSurface9::GetDesc",
    extendedMethodDescriptions[472].autogenerate = true;   // 472: "IDirect3DSurface9::LockRect",
    extendedMethodDescriptions[473].autogenerate = true;   // 473: "IDirect3DSurface9::UnlockRect",
                                                           // 474: "IDirect3DSurface9::GetDC",
                                                           // 475: "IDirect3DSurface9::ReleaseDC",
                                                           // 476: "IDirect3DVolume9::QueryInterface",
     extendedMethodDescriptions[477].autogenerate = true;  // 477: "IDirect3DVolume9::AddRef",
     extendedMethodDescriptions[478].autogenerate = true;  // 478: "IDirect3DVolume9::Release",
     extendedMethodDescriptions[479].autogenerate = true;  // 479: "IDirect3DVolume9::GetDevice",
                                                           // 480: "IDirect3DVolume9::SetPrivateData",
                                                           // 481: "IDirect3DVolume9::GetPrivateData",
                                                           // 482: "IDirect3DVolume9::FreePrivateData",
     extendedMethodDescriptions[483].autogenerate = true;  // 483: "IDirect3DVolume9::GetContainer",
                                                           // 484: "IDirect3DVolume9::GetDesc",
     extendedMethodDescriptions[485].autogenerate = true;  // 485: "IDirect3DVolume9::LockBox",
     extendedMethodDescriptions[486].autogenerate = true;  // 486: "IDirect3DVolume9::UnlockBox",
                                                           // 487: "IDirect3DVertexDeclaration9::QueryInterface",
     extendedMethodDescriptions[488].autogenerate = true;  // 488: "IDirect3DVertexDeclaration9::AddRef",
     extendedMethodDescriptions[489].autogenerate = true;  // 489: "IDirect3DVertexDeclaration9::Release",
     extendedMethodDescriptions[490].autogenerate = true;  // 490: "IDirect3DVertexDeclaration9::GetDevice",
                                                           // 491: "IDirect3DVertexDeclaration9::GetDeclaration",
                                                           // 492: "IDirect3DVertexShader9::QueryInterface",
     extendedMethodDescriptions[493].autogenerate = true;  // 493: "IDirect3DVertexShader9::AddRef",
     extendedMethodDescriptions[494].autogenerate = true;  // 494: "IDirect3DVertexShader9::Release",
     extendedMethodDescriptions[495].autogenerate = true;  // 495: "IDirect3DVertexShader9::GetDevice",
                                                           // 496: "IDirect3DVertexShader9::GetFunction",
                                                           // 497: "IDirect3DPixelShader9::QueryInterface",
     extendedMethodDescriptions[498].autogenerate = true;  // 498: "IDirect3DPixelShader9::AddRef",
     extendedMethodDescriptions[499].autogenerate = true;  // 499: "IDirect3DPixelShader9::Release",
     extendedMethodDescriptions[500].autogenerate = true;  // 500: "IDirect3DPixelShader9::GetDevice",
                                                           // 501: "IDirect3DPixelShader9::GetFunction",
                                                           // 502: "IDirect3DStateBlock9::QueryInterface",
     extendedMethodDescriptions[503].autogenerate = true;  // 503: "IDirect3DStateBlock9::AddRef",
     extendedMethodDescriptions[504].autogenerate = true;  // 504: "IDirect3DStateBlock9::Release",
     extendedMethodDescriptions[505].autogenerate = true;  // 505: "IDirect3DStateBlock9::GetDevice",
     extendedMethodDescriptions[506].autogenerate = true;  // 506: "IDirect3DStateBlock9::Capture",
     extendedMethodDescriptions[507].autogenerate = true;  // 507: "IDirect3DStateBlock9::Apply",
                                                           // 508: "IDirect3DQuery9::QueryInterface",
                                                           // 509: "IDirect3DQuery9::AddRef",
                                                           // 510: "IDirect3DQuery9::Release",
                                                           // 511: "IDirect3DQuery9::GetDevice",
                                                           // 512: "IDirect3DQuery9::GetType",
                                                           // 513: "IDirect3DQuery9::GetDataSize",
                                                           // 514: "IDirect3DQuery9::Issue",
                                                           // 515: "IDirect3DQuery9::GetData",
                                                           // 516: "IDirect3DVideoDevice9::QueryInterface",
                                                           // 517: "IDirect3DVideoDevice9::AddRef",
                                                           // 518: "IDirect3DVideoDevice9::Release",
                                                           // 519: "IDirect3DVideoDevice9::CreateSurface",
                                                           // 520: "IDirect3DVideoDevice9::GetDXVACompressedBufferInfo",
                                                           // 521: "IDirect3DVideoDevice9::GetDXVAGuids",
                                                           // 522: "IDirect3DVideoDevice9::GetDXVAInternalInfo",
                                                           // 523: "IDirect3DVideoDevice9::GetUncompressedDXVAFormats",
                                                           // 524: "IDirect3DVideoDevice9::CreateDXVADevice",
                                                           // 525: "IDirect3DDXVADevice9::QueryInterface",
                                                           // 526: "IDirect3DDXVADevice9::AddRef",
                                                           // 527: "IDirect3DDXVADevice9::Release",
                                                           // 528: "IDirect3DDXVADevice9::BeginFrame",
                                                           // 529: "IDirect3DDXVADevice9::EndFrame",
                                                           // 530: "IDirect3DDXVADevice9::Execute",
                                                           // 531: "IDirect3DDXVADevice9::QueryStatus",
    // Direct3D 11
    extendedMethodDescriptions[2000].autogenerate = true; // "D3D11CreateDevice",
    extendedMethodDescriptions[2001].autogenerate = true; // "D3D11CreateDeviceAndSwapChain",
    extendedMethodDescriptions[2011].autogenerate = true; // "ID3D11DepthStencilState::QueryInterface",
    extendedMethodDescriptions[2012].autogenerate = true; // "ID3D11DepthStencilState::AddRef",
    extendedMethodDescriptions[2013].autogenerate = true; // "ID3D11DepthStencilState::Release",
    extendedMethodDescriptions[2014].autogenerate = true; // "ID3D11DepthStencilState::GetDevice",
    extendedMethodDescriptions[2015].autogenerate = true; // "ID3D11DepthStencilState::GetPrivateData",
    extendedMethodDescriptions[2016].autogenerate = true; // "ID3D11DepthStencilState::SetPrivateData",
    extendedMethodDescriptions[2017].autogenerate = true; // "ID3D11DepthStencilState::SetPrivateDataInterface",
    extendedMethodDescriptions[2018].autogenerate = true; // "ID3D11DepthStencilState::GetDesc",
    extendedMethodDescriptions[2019].autogenerate = true; // "ID3D11BlendState::QueryInterface",
    extendedMethodDescriptions[2020].autogenerate = true; // "ID3D11BlendState::AddRef",
    extendedMethodDescriptions[2021].autogenerate = true; // "ID3D11BlendState::Release",
    extendedMethodDescriptions[2022].autogenerate = true; // "ID3D11BlendState::GetDevice",
    extendedMethodDescriptions[2023].autogenerate = true; // "ID3D11BlendState::GetPrivateData",
    extendedMethodDescriptions[2024].autogenerate = true; // "ID3D11BlendState::SetPrivateData",
    extendedMethodDescriptions[2025].autogenerate = true; // "ID3D11BlendState::SetPrivateDataInterface",
    extendedMethodDescriptions[2026].autogenerate = true; // "ID3D11BlendState::GetDesc",
    extendedMethodDescriptions[2027].autogenerate = true; // "ID3D11RasterizerState::QueryInterface",
    extendedMethodDescriptions[2028].autogenerate = true; // "ID3D11RasterizerState::AddRef",
    extendedMethodDescriptions[2029].autogenerate = true; // "ID3D11RasterizerState::Release",
    extendedMethodDescriptions[2030].autogenerate = true; // "ID3D11RasterizerState::GetDevice",
    extendedMethodDescriptions[2031].autogenerate = true; // "ID3D11RasterizerState::GetPrivateData",
    extendedMethodDescriptions[2032].autogenerate = true; // "ID3D11RasterizerState::SetPrivateData",
    extendedMethodDescriptions[2033].autogenerate = true; // "ID3D11RasterizerState::SetPrivateDataInterface",
    extendedMethodDescriptions[2034].autogenerate = true; // "ID3D11RasterizerState::GetDesc",
    extendedMethodDescriptions[2035].autogenerate = true; // "ID3D11Buffer::QueryInterface",
    extendedMethodDescriptions[2036].autogenerate = true; // "ID3D11Buffer::AddRef",
    extendedMethodDescriptions[2037].autogenerate = true; // "ID3D11Buffer::Release",
    extendedMethodDescriptions[2038].autogenerate = true; // "ID3D11Buffer::GetDevice",
    extendedMethodDescriptions[2039].autogenerate = true; // "ID3D11Buffer::GetPrivateData",
    extendedMethodDescriptions[2040].autogenerate = true; // "ID3D11Buffer::SetPrivateData",
    extendedMethodDescriptions[2041].autogenerate = true; // "ID3D11Buffer::SetPrivateDataInterface",
    extendedMethodDescriptions[2042].autogenerate = true; // "ID3D11Buffer::GetType",
    extendedMethodDescriptions[2043].autogenerate = true; // "ID3D11Buffer::SetEvictionPriority",
    extendedMethodDescriptions[2044].autogenerate = true; // "ID3D11Buffer::GetEvictionPriority",
    extendedMethodDescriptions[2045].autogenerate = true; // "ID3D11Buffer::GetDesc",
    extendedMethodDescriptions[2046].autogenerate = true; // "ID3D11Texture1D::QueryInterface",
    extendedMethodDescriptions[2047].autogenerate = true; // "ID3D11Texture1D::AddRef",
    extendedMethodDescriptions[2048].autogenerate = true; // "ID3D11Texture1D::Release",
    extendedMethodDescriptions[2049].autogenerate = true; // "ID3D11Texture1D::GetDevice",
    extendedMethodDescriptions[2050].autogenerate = true; // "ID3D11Texture1D::GetPrivateData",
    extendedMethodDescriptions[2051].autogenerate = true; // "ID3D11Texture1D::SetPrivateData",
    extendedMethodDescriptions[2052].autogenerate = true; // "ID3D11Texture1D::SetPrivateDataInterface",
    extendedMethodDescriptions[2053].autogenerate = true; // "ID3D11Texture1D::GetType",
    extendedMethodDescriptions[2054].autogenerate = true; // "ID3D11Texture1D::SetEvictionPriority",
    extendedMethodDescriptions[2055].autogenerate = true; // "ID3D11Texture1D::GetEvictionPriority",
    extendedMethodDescriptions[2056].autogenerate = true; // "ID3D11Texture1D::GetDesc",
    extendedMethodDescriptions[2057].autogenerate = true; // "ID3D11Texture2D::QueryInterface",
    extendedMethodDescriptions[2058].autogenerate = true; // "ID3D11Texture2D::AddRef",
    extendedMethodDescriptions[2059].autogenerate = true; // "ID3D11Texture2D::Release",
    extendedMethodDescriptions[2060].autogenerate = true; // "ID3D11Texture2D::GetDevice",
    extendedMethodDescriptions[2061].autogenerate = true; // "ID3D11Texture2D::GetPrivateData",
    extendedMethodDescriptions[2062].autogenerate = true; // "ID3D11Texture2D::SetPrivateData",
    extendedMethodDescriptions[2063].autogenerate = true; // "ID3D11Texture2D::SetPrivateDataInterface",
    extendedMethodDescriptions[2064].autogenerate = true; // "ID3D11Texture2D::GetType",
    extendedMethodDescriptions[2065].autogenerate = true; // "ID3D11Texture2D::SetEvictionPriority",
    extendedMethodDescriptions[2066].autogenerate = true; // "ID3D11Texture2D::GetEvictionPriority",
    extendedMethodDescriptions[2067].autogenerate = true; // "ID3D11Texture2D::GetDesc",
    extendedMethodDescriptions[2068].autogenerate = true; // "ID3D11Texture3D::QueryInterface",
    extendedMethodDescriptions[2069].autogenerate = true; // "ID3D11Texture3D::AddRef",
    extendedMethodDescriptions[2070].autogenerate = true; // "ID3D11Texture3D::Release",
    extendedMethodDescriptions[2071].autogenerate = true; // "ID3D11Texture3D::GetDevice",
    extendedMethodDescriptions[2072].autogenerate = true; // "ID3D11Texture3D::GetPrivateData",
    extendedMethodDescriptions[2073].autogenerate = true; // "ID3D11Texture3D::SetPrivateData",
    extendedMethodDescriptions[2074].autogenerate = true; // "ID3D11Texture3D::SetPrivateDataInterface",
    extendedMethodDescriptions[2075].autogenerate = true; // "ID3D11Texture3D::GetType",
    extendedMethodDescriptions[2076].autogenerate = true; // "ID3D11Texture3D::SetEvictionPriority",
    extendedMethodDescriptions[2077].autogenerate = true; // "ID3D11Texture3D::GetEvictionPriority",
    extendedMethodDescriptions[2078].autogenerate = true; // "ID3D11Texture3D::GetDesc",
    extendedMethodDescriptions[2079].autogenerate = true; // "ID3D11ShaderResourceView::QueryInterface",
    extendedMethodDescriptions[2080].autogenerate = true; // "ID3D11ShaderResourceView::AddRef",
    extendedMethodDescriptions[2081].autogenerate = true; // "ID3D11ShaderResourceView::Release",
    extendedMethodDescriptions[2082].autogenerate = true; // "ID3D11ShaderResourceView::GetDevice",
    extendedMethodDescriptions[2083].autogenerate = true; // "ID3D11ShaderResourceView::GetPrivateData",
    extendedMethodDescriptions[2084].autogenerate = true; // "ID3D11ShaderResourceView::SetPrivateData",
    extendedMethodDescriptions[2085].autogenerate = true; // "ID3D11ShaderResourceView::SetPrivateDataInterface",
    extendedMethodDescriptions[2086].autogenerate = true; // "ID3D11ShaderResourceView::GetResource",
    extendedMethodDescriptions[2087].autogenerate = true; // "ID3D11ShaderResourceView::GetDesc",
    extendedMethodDescriptions[2088].autogenerate = true; // "ID3D11RenderTargetView::QueryInterface",
    extendedMethodDescriptions[2089].autogenerate = true; // "ID3D11RenderTargetView::AddRef",
    extendedMethodDescriptions[2090].autogenerate = true; // "ID3D11RenderTargetView::Release",
    extendedMethodDescriptions[2091].autogenerate = true; // "ID3D11RenderTargetView::GetDevice",
    extendedMethodDescriptions[2092].autogenerate = true; // "ID3D11RenderTargetView::GetPrivateData",
    extendedMethodDescriptions[2093].autogenerate = true; // "ID3D11RenderTargetView::SetPrivateData",
    extendedMethodDescriptions[2094].autogenerate = true; // "ID3D11RenderTargetView::SetPrivateDataInterface",
    extendedMethodDescriptions[2095].autogenerate = true; // "ID3D11RenderTargetView::GetResource",
    extendedMethodDescriptions[2096].autogenerate = true; // "ID3D11RenderTargetView::GetDesc",
    extendedMethodDescriptions[2097].autogenerate = true; // "ID3D11DepthStencilView::QueryInterface",
    extendedMethodDescriptions[2098].autogenerate = true; // "ID3D11DepthStencilView::AddRef",
    extendedMethodDescriptions[2099].autogenerate = true; // "ID3D11DepthStencilView::Release",
    extendedMethodDescriptions[2100].autogenerate = true; // "ID3D11DepthStencilView::GetDevice",
    extendedMethodDescriptions[2101].autogenerate = true; // "ID3D11DepthStencilView::GetPrivateData",
    extendedMethodDescriptions[2102].autogenerate = true; // "ID3D11DepthStencilView::SetPrivateData",
    extendedMethodDescriptions[2103].autogenerate = true; // "ID3D11DepthStencilView::SetPrivateDataInterface",
    extendedMethodDescriptions[2104].autogenerate = true; // "ID3D11DepthStencilView::GetResource",
    extendedMethodDescriptions[2105].autogenerate = true; // "ID3D11DepthStencilView::GetDesc",
    extendedMethodDescriptions[2106].autogenerate = true; // "ID3D11UnorderedAccessView::QueryInterface",
    extendedMethodDescriptions[2107].autogenerate = true; // "ID3D11UnorderedAccessView::AddRef",
    extendedMethodDescriptions[2108].autogenerate = true; // "ID3D11UnorderedAccessView::Release",
    extendedMethodDescriptions[2109].autogenerate = true; // "ID3D11UnorderedAccessView::GetDevice",
    extendedMethodDescriptions[2110].autogenerate = true; // "ID3D11UnorderedAccessView::GetPrivateData",
    extendedMethodDescriptions[2111].autogenerate = true; // "ID3D11UnorderedAccessView::SetPrivateData",
    extendedMethodDescriptions[2112].autogenerate = true; // "ID3D11UnorderedAccessView::SetPrivateDataInterface",
    extendedMethodDescriptions[2113].autogenerate = true; // "ID3D11UnorderedAccessView::GetResource",
    extendedMethodDescriptions[2114].autogenerate = true; // "ID3D11UnorderedAccessView::GetDesc",
    extendedMethodDescriptions[2115].autogenerate = true; // "ID3D11VertexShader::QueryInterface",
    extendedMethodDescriptions[2116].autogenerate = true; // "ID3D11VertexShader::AddRef",
    extendedMethodDescriptions[2117].autogenerate = true; // "ID3D11VertexShader::Release",
    extendedMethodDescriptions[2118].autogenerate = true; // "ID3D11VertexShader::GetDevice",
    extendedMethodDescriptions[2119].autogenerate = true; // "ID3D11VertexShader::GetPrivateData",
    extendedMethodDescriptions[2120].autogenerate = true; // "ID3D11VertexShader::SetPrivateData",
    extendedMethodDescriptions[2121].autogenerate = true; // "ID3D11VertexShader::SetPrivateDataInterface",
    extendedMethodDescriptions[2122].autogenerate = true; // "ID3D11HullShader::QueryInterface",
    extendedMethodDescriptions[2123].autogenerate = true; // "ID3D11HullShader::AddRef",
    extendedMethodDescriptions[2124].autogenerate = true; // "ID3D11HullShader::Release",
    extendedMethodDescriptions[2125].autogenerate = true; // "ID3D11HullShader::GetDevice",
    extendedMethodDescriptions[2126].autogenerate = true; // "ID3D11HullShader::GetPrivateData",
    extendedMethodDescriptions[2127].autogenerate = true; // "ID3D11HullShader::SetPrivateData",
    extendedMethodDescriptions[2128].autogenerate = true; // "ID3D11HullShader::SetPrivateDataInterface",
    extendedMethodDescriptions[2129].autogenerate = true; // "ID3D11DomainShader::QueryInterface",
    extendedMethodDescriptions[2130].autogenerate = true; // "ID3D11DomainShader::AddRef",
    extendedMethodDescriptions[2131].autogenerate = true; // "ID3D11DomainShader::Release",
    extendedMethodDescriptions[2132].autogenerate = true; // "ID3D11DomainShader::GetDevice",
    extendedMethodDescriptions[2133].autogenerate = true; // "ID3D11DomainShader::GetPrivateData",
    extendedMethodDescriptions[2134].autogenerate = true; // "ID3D11DomainShader::SetPrivateData",
    extendedMethodDescriptions[2135].autogenerate = true; // "ID3D11DomainShader::SetPrivateDataInterface",
    extendedMethodDescriptions[2136].autogenerate = true; // "ID3D11GeometryShader::QueryInterface",
    extendedMethodDescriptions[2137].autogenerate = true; // "ID3D11GeometryShader::AddRef",
    extendedMethodDescriptions[2138].autogenerate = true; // "ID3D11GeometryShader::Release",
    extendedMethodDescriptions[2139].autogenerate = true; // "ID3D11GeometryShader::GetDevice",
    extendedMethodDescriptions[2140].autogenerate = true; // "ID3D11GeometryShader::GetPrivateData",
    extendedMethodDescriptions[2141].autogenerate = true; // "ID3D11GeometryShader::SetPrivateData",
    extendedMethodDescriptions[2142].autogenerate = true; // "ID3D11GeometryShader::SetPrivateDataInterface",
    extendedMethodDescriptions[2143].autogenerate = true; // "ID3D11PixelShader::QueryInterface",
    extendedMethodDescriptions[2144].autogenerate = true; // "ID3D11PixelShader::AddRef",
    extendedMethodDescriptions[2145].autogenerate = true; // "ID3D11PixelShader::Release",
    extendedMethodDescriptions[2146].autogenerate = true; // "ID3D11PixelShader::GetDevice",
    extendedMethodDescriptions[2147].autogenerate = true; // "ID3D11PixelShader::GetPrivateData",
    extendedMethodDescriptions[2148].autogenerate = true; // "ID3D11PixelShader::SetPrivateData",
    extendedMethodDescriptions[2149].autogenerate = true; // "ID3D11PixelShader::SetPrivateDataInterface",
    extendedMethodDescriptions[2150].autogenerate = true; // "ID3D11ComputeShader::QueryInterface",
    extendedMethodDescriptions[2151].autogenerate = true; // "ID3D11ComputeShader::AddRef",
    extendedMethodDescriptions[2152].autogenerate = true; // "ID3D11ComputeShader::Release",
    extendedMethodDescriptions[2153].autogenerate = true; // "ID3D11ComputeShader::GetDevice",
    extendedMethodDescriptions[2154].autogenerate = true; // "ID3D11ComputeShader::GetPrivateData",
    extendedMethodDescriptions[2155].autogenerate = true; // "ID3D11ComputeShader::SetPrivateData",
    extendedMethodDescriptions[2156].autogenerate = true; // "ID3D11ComputeShader::SetPrivateDataInterface",
    extendedMethodDescriptions[2157].autogenerate = true; // "ID3D11InputLayout::QueryInterface",
    extendedMethodDescriptions[2158].autogenerate = true; // "ID3D11InputLayout::AddRef",
    extendedMethodDescriptions[2159].autogenerate = true; // "ID3D11InputLayout::Release",
    extendedMethodDescriptions[2160].autogenerate = true; // "ID3D11InputLayout::GetDevice",
    extendedMethodDescriptions[2161].autogenerate = true; // "ID3D11InputLayout::GetPrivateData",
    extendedMethodDescriptions[2162].autogenerate = true; // "ID3D11InputLayout::SetPrivateData",
    extendedMethodDescriptions[2163].autogenerate = true; // "ID3D11InputLayout::SetPrivateDataInterface",
    extendedMethodDescriptions[2164].autogenerate = true; // "ID3D11SamplerState::QueryInterface",
    extendedMethodDescriptions[2165].autogenerate = true; // "ID3D11SamplerState::AddRef",
    extendedMethodDescriptions[2166].autogenerate = true; // "ID3D11SamplerState::Release",
    extendedMethodDescriptions[2167].autogenerate = true; // "ID3D11SamplerState::GetDevice",
    extendedMethodDescriptions[2168].autogenerate = true; // "ID3D11SamplerState::GetPrivateData",
    extendedMethodDescriptions[2169].autogenerate = true; // "ID3D11SamplerState::SetPrivateData",
    extendedMethodDescriptions[2170].autogenerate = true; // "ID3D11SamplerState::SetPrivateDataInterface",
    extendedMethodDescriptions[2171].autogenerate = true; // "ID3D11SamplerState::GetDesc",
    extendedMethodDescriptions[2172].autogenerate = true; // "ID3D11Query::QueryInterface",
    extendedMethodDescriptions[2173].autogenerate = true; // "ID3D11Query::AddRef",
    extendedMethodDescriptions[2174].autogenerate = true; // "ID3D11Query::Release",
    extendedMethodDescriptions[2175].autogenerate = true; // "ID3D11Query::GetDevice",
    extendedMethodDescriptions[2176].autogenerate = true; // "ID3D11Query::GetPrivateData",
    extendedMethodDescriptions[2177].autogenerate = true; // "ID3D11Query::SetPrivateData",
    extendedMethodDescriptions[2178].autogenerate = true; // "ID3D11Query::SetPrivateDataInterface",
    extendedMethodDescriptions[2179].autogenerate = true; // "ID3D11Query::GetDataSize",
    extendedMethodDescriptions[2180].autogenerate = true; // "ID3D11Query::GetDesc",
    extendedMethodDescriptions[2181].autogenerate = true; // "ID3D11Predicate::QueryInterface",
    extendedMethodDescriptions[2182].autogenerate = true; // "ID3D11Predicate::AddRef",
    extendedMethodDescriptions[2183].autogenerate = true; // "ID3D11Predicate::Release",
    extendedMethodDescriptions[2184].autogenerate = true; // "ID3D11Predicate::GetDevice",
    extendedMethodDescriptions[2185].autogenerate = true; // "ID3D11Predicate::GetPrivateData",
    extendedMethodDescriptions[2186].autogenerate = true; // "ID3D11Predicate::SetPrivateData",
    extendedMethodDescriptions[2187].autogenerate = true; // "ID3D11Predicate::SetPrivateDataInterface",
    extendedMethodDescriptions[2188].autogenerate = true; // "ID3D11Predicate::GetDataSize",
    extendedMethodDescriptions[2189].autogenerate = true; // "ID3D11Predicate::GetDesc",
    extendedMethodDescriptions[2190].autogenerate = true; // "ID3D11Counter::QueryInterface",
    extendedMethodDescriptions[2191].autogenerate = true; // "ID3D11Counter::AddRef",
    extendedMethodDescriptions[2192].autogenerate = true; // "ID3D11Counter::Release",
    extendedMethodDescriptions[2193].autogenerate = true; // "ID3D11Counter::GetDevice",
    extendedMethodDescriptions[2194].autogenerate = true; // "ID3D11Counter::GetPrivateData",
    extendedMethodDescriptions[2195].autogenerate = true; // "ID3D11Counter::SetPrivateData",
    extendedMethodDescriptions[2196].autogenerate = true; // "ID3D11Counter::SetPrivateDataInterface",
    extendedMethodDescriptions[2197].autogenerate = true; // "ID3D11Counter::GetDataSize",
    extendedMethodDescriptions[2198].autogenerate = true; // "ID3D11Counter::GetDesc",
    extendedMethodDescriptions[2199].autogenerate = true; // "ID3D11ClassInstance::QueryInterface",
    extendedMethodDescriptions[2200].autogenerate = true; // "ID3D11ClassInstance::AddRef",
    extendedMethodDescriptions[2201].autogenerate = true; // "ID3D11ClassInstance::Release",
    extendedMethodDescriptions[2202].autogenerate = true; // "ID3D11ClassInstance::GetDevice",
    extendedMethodDescriptions[2203].autogenerate = true; // "ID3D11ClassInstance::GetPrivateData",
    extendedMethodDescriptions[2204].autogenerate = true; // "ID3D11ClassInstance::SetPrivateData",
    extendedMethodDescriptions[2205].autogenerate = true; // "ID3D11ClassInstance::SetPrivateDataInterface",
    extendedMethodDescriptions[2206].autogenerate = true; // "ID3D11ClassInstance::GetClassLinkage",
    extendedMethodDescriptions[2207].autogenerate = true; // "ID3D11ClassInstance::GetDesc",
    extendedMethodDescriptions[2208].autogenerate = true; // "ID3D11ClassInstance::GetInstanceName",
    extendedMethodDescriptions[2209].autogenerate = true; // "ID3D11ClassInstance::GetTypeName",
    extendedMethodDescriptions[2210].autogenerate = true; // "ID3D11ClassLinkage::QueryInterface",
    extendedMethodDescriptions[2211].autogenerate = true; // "ID3D11ClassLinkage::AddRef",
    extendedMethodDescriptions[2212].autogenerate = true; // "ID3D11ClassLinkage::Release",
    extendedMethodDescriptions[2213].autogenerate = true; // "ID3D11ClassLinkage::GetDevice",
    extendedMethodDescriptions[2214].autogenerate = true; // "ID3D11ClassLinkage::GetPrivateData",
    extendedMethodDescriptions[2215].autogenerate = true; // "ID3D11ClassLinkage::SetPrivateData",
    extendedMethodDescriptions[2216].autogenerate = true; // "ID3D11ClassLinkage::SetPrivateDataInterface",
    extendedMethodDescriptions[2217].autogenerate = true; // "ID3D11ClassLinkage::GetClassInstance",
    extendedMethodDescriptions[2218].autogenerate = true; // "ID3D11ClassLinkage::CreateClassInstance",
    extendedMethodDescriptions[2219].autogenerate = true; // "ID3D11CommandList::QueryInterface",
    extendedMethodDescriptions[2220].autogenerate = true; // "ID3D11CommandList::AddRef",
    extendedMethodDescriptions[2221].autogenerate = true; // "ID3D11CommandList::Release",
    extendedMethodDescriptions[2222].autogenerate = true; // "ID3D11CommandList::GetDevice",
    extendedMethodDescriptions[2223].autogenerate = true; // "ID3D11CommandList::GetPrivateData",
    extendedMethodDescriptions[2224].autogenerate = true; // "ID3D11CommandList::SetPrivateData",
    extendedMethodDescriptions[2225].autogenerate = true; // "ID3D11CommandList::SetPrivateDataInterface",
    extendedMethodDescriptions[2226].autogenerate = true; // "ID3D11CommandList::GetContextFlags",
    extendedMethodDescriptions[2228].autogenerate = true; // "ID3D11DeviceContext::AddRef",
    extendedMethodDescriptions[2229].autogenerate = true; // "ID3D11DeviceContext::Release",
    extendedMethodDescriptions[2230].autogenerate = true; // "ID3D11DeviceContext::GetDevice",
    extendedMethodDescriptions[2231].autogenerate = true; // "ID3D11DeviceContext::GetPrivateData",
    extendedMethodDescriptions[2232].autogenerate = true; // "ID3D11DeviceContext::SetPrivateData",
    extendedMethodDescriptions[2233].autogenerate = true; // "ID3D11DeviceContext::SetPrivateDataInterface",
    extendedMethodDescriptions[2234].autogenerate = true; // "ID3D11DeviceContext::VSSetConstantBuffers",
    extendedMethodDescriptions[2235].autogenerate = true; // "ID3D11DeviceContext::PSSetShaderResources",
    extendedMethodDescriptions[2236].autogenerate = true; // "ID3D11DeviceContext::PSSetShader",
    extendedMethodDescriptions[2237].autogenerate = true; // "ID3D11DeviceContext::PSSetSamplers",
    extendedMethodDescriptions[2238].autogenerate = true; // "ID3D11DeviceContext::VSSetShader",
    extendedMethodDescriptions[2239].autogenerate = true; // "ID3D11DeviceContext::DrawIndexed",
    extendedMethodDescriptions[2240].autogenerate = true; // "ID3D11DeviceContext::Draw",
    extendedMethodDescriptions[2241].autogenerate = true; // "ID3D11DeviceContext::Map",
    extendedMethodDescriptions[2242].autogenerate = true; // "ID3D11DeviceContext::Unmap",
    extendedMethodDescriptions[2243].autogenerate = true; // "ID3D11DeviceContext::PSSetConstantBuffers",
    extendedMethodDescriptions[2244].autogenerate = true; // "ID3D11DeviceContext::IASetInputLayout",
    extendedMethodDescriptions[2245].autogenerate = true; // "ID3D11DeviceContext::IASetVertexBuffers",
    extendedMethodDescriptions[2246].autogenerate = true; // "ID3D11DeviceContext::IASetIndexBuffer",
    extendedMethodDescriptions[2247].autogenerate = true; // "ID3D11DeviceContext::DrawIndexedInstanced",
    extendedMethodDescriptions[2248].autogenerate = true; // "ID3D11DeviceContext::DrawInstanced",
    extendedMethodDescriptions[2249].autogenerate = true; // "ID3D11DeviceContext::GSSetConstantBuffers",
    extendedMethodDescriptions[2250].autogenerate = true; // "ID3D11DeviceContext::GSSetShader",
    extendedMethodDescriptions[2251].autogenerate = true; // "ID3D11DeviceContext::IASetPrimitiveTopology",
    extendedMethodDescriptions[2252].autogenerate = true; // "ID3D11DeviceContext::VSSetShaderResources",
    extendedMethodDescriptions[2253].autogenerate = true; // "ID3D11DeviceContext::VSSetSamplers",
    extendedMethodDescriptions[2254].autogenerate = true; // "ID3D11DeviceContext::Begin",
    extendedMethodDescriptions[2255].autogenerate = true; // "ID3D11DeviceContext::End",
    extendedMethodDescriptions[2256].autogenerate = true; // "ID3D11DeviceContext::GetData",
    extendedMethodDescriptions[2257].autogenerate = true; // "ID3D11DeviceContext::SetPredication",
    extendedMethodDescriptions[2258].autogenerate = true; // "ID3D11DeviceContext::GSSetShaderResources",
    extendedMethodDescriptions[2259].autogenerate = true; // "ID3D11DeviceContext::GSSetSamplers",
    extendedMethodDescriptions[2260].autogenerate = true; // "ID3D11DeviceContext::OMSetRenderTargets",
    extendedMethodDescriptions[2261].autogenerate = true; // "ID3D11DeviceContext::OMSetRenderTargetsAndUnorderedAccessViews",
    extendedMethodDescriptions[2262].autogenerate = true; // "ID3D11DeviceContext::OMSetBlendState",
    extendedMethodDescriptions[2263].autogenerate = true; // "ID3D11DeviceContext::OMSetDepthStencilState",
    extendedMethodDescriptions[2264].autogenerate = true; // "ID3D11DeviceContext::SOSetTargets",
    extendedMethodDescriptions[2265].autogenerate = true; // "ID3D11DeviceContext::DrawAuto",
    extendedMethodDescriptions[2266].autogenerate = true; // "ID3D11DeviceContext::DrawIndexedInstancedIndirect",
    extendedMethodDescriptions[2267].autogenerate = true; // "ID3D11DeviceContext::DrawInstancedIndirect",
    extendedMethodDescriptions[2268].autogenerate = true; // "ID3D11DeviceContext::Dispatch",
    extendedMethodDescriptions[2269].autogenerate = true; // "ID3D11DeviceContext::DispatchIndirect",
    extendedMethodDescriptions[2270].autogenerate = true; // "ID3D11DeviceContext::RSSetState",
    extendedMethodDescriptions[2271].autogenerate = true; // "ID3D11DeviceContext::RSSetViewports",
    extendedMethodDescriptions[2272].autogenerate = true; // "ID3D11DeviceContext::RSSetScissorRects",
    extendedMethodDescriptions[2273].autogenerate = true; // "ID3D11DeviceContext::CopySubresourceRegion",
    extendedMethodDescriptions[2274].autogenerate = true; // "ID3D11DeviceContext::CopyResource",
    extendedMethodDescriptions[2275].autogenerate = true; // "ID3D11DeviceContext::UpdateSubresource",
    extendedMethodDescriptions[2276].autogenerate = true; // "ID3D11DeviceContext::CopyStructureCount",
    extendedMethodDescriptions[2277].autogenerate = true; // "ID3D11DeviceContext::ClearRenderTargetView",
    extendedMethodDescriptions[2278].autogenerate = true; // "ID3D11DeviceContext::ClearUnorderedAccessViewUint",
    extendedMethodDescriptions[2279].autogenerate = true; // "ID3D11DeviceContext::ClearUnorderedAccessViewFloat",
    extendedMethodDescriptions[2280].autogenerate = true; // "ID3D11DeviceContext::ClearDepthStencilView",
    extendedMethodDescriptions[2281].autogenerate = true; // "ID3D11DeviceContext::GenerateMips",
    extendedMethodDescriptions[2282].autogenerate = true; // "ID3D11DeviceContext::SetResourceMinLOD",
    extendedMethodDescriptions[2283].autogenerate = true; // "ID3D11DeviceContext::GetResourceMinLOD",
    extendedMethodDescriptions[2284].autogenerate = true; // "ID3D11DeviceContext::ResolveSubresource",
    extendedMethodDescriptions[2285].autogenerate = true; // "ID3D11DeviceContext::ExecuteCommandList",
    extendedMethodDescriptions[2286].autogenerate = true; // "ID3D11DeviceContext::HSSetShaderResources",
    extendedMethodDescriptions[2287].autogenerate = true; // "ID3D11DeviceContext::HSSetShader",
    extendedMethodDescriptions[2288].autogenerate = true; // "ID3D11DeviceContext::HSSetSamplers",
    extendedMethodDescriptions[2289].autogenerate = true; // "ID3D11DeviceContext::HSSetConstantBuffers",
    extendedMethodDescriptions[2290].autogenerate = true; // "ID3D11DeviceContext::DSSetShaderResources",
    extendedMethodDescriptions[2291].autogenerate = true; // "ID3D11DeviceContext::DSSetShader",
    extendedMethodDescriptions[2292].autogenerate = true; // "ID3D11DeviceContext::DSSetSamplers",
    extendedMethodDescriptions[2293].autogenerate = true; // "ID3D11DeviceContext::DSSetConstantBuffers",
    extendedMethodDescriptions[2294].autogenerate = true; // "ID3D11DeviceContext::CSSetShaderResources",
    extendedMethodDescriptions[2295].autogenerate = true; // "ID3D11DeviceContext::CSSetUnorderedAccessViews",
    extendedMethodDescriptions[2296].autogenerate = true; // "ID3D11DeviceContext::CSSetShader",
    extendedMethodDescriptions[2297].autogenerate = true; // "ID3D11DeviceContext::CSSetSamplers",
    extendedMethodDescriptions[2298].autogenerate = true; // "ID3D11DeviceContext::CSSetConstantBuffers",
    extendedMethodDescriptions[2299].autogenerate = true; // "ID3D11DeviceContext::VSGetConstantBuffers",
    extendedMethodDescriptions[2300].autogenerate = true; // "ID3D11DeviceContext::PSGetShaderResources",
    extendedMethodDescriptions[2301].autogenerate = true; // "ID3D11DeviceContext::PSGetShader",
    extendedMethodDescriptions[2302].autogenerate = true; // "ID3D11DeviceContext::PSGetSamplers",
    extendedMethodDescriptions[2303].autogenerate = true; // "ID3D11DeviceContext::VSGetShader",
    extendedMethodDescriptions[2304].autogenerate = true; // "ID3D11DeviceContext::PSGetConstantBuffers",
    extendedMethodDescriptions[2305].autogenerate = true; // "ID3D11DeviceContext::IAGetInputLayout",
    extendedMethodDescriptions[2306].autogenerate = true; // "ID3D11DeviceContext::IAGetVertexBuffers",
    extendedMethodDescriptions[2307].autogenerate = true; // "ID3D11DeviceContext::IAGetIndexBuffer",
    extendedMethodDescriptions[2308].autogenerate = true; // "ID3D11DeviceContext::GSGetConstantBuffers",
    extendedMethodDescriptions[2309].autogenerate = true; // "ID3D11DeviceContext::GSGetShader",
    extendedMethodDescriptions[2310].autogenerate = true; // "ID3D11DeviceContext::IAGetPrimitiveTopology",
    extendedMethodDescriptions[2311].autogenerate = true; // "ID3D11DeviceContext::VSGetShaderResources",
    extendedMethodDescriptions[2312].autogenerate = true; // "ID3D11DeviceContext::VSGetSamplers",
    extendedMethodDescriptions[2313].autogenerate = true; // "ID3D11DeviceContext::GetPredication",
    extendedMethodDescriptions[2314].autogenerate = true; // "ID3D11DeviceContext::GSGetShaderResources",
    extendedMethodDescriptions[2315].autogenerate = true; // "ID3D11DeviceContext::GSGetSamplers",
    extendedMethodDescriptions[2316].autogenerate = true; // "ID3D11DeviceContext::OMGetRenderTargets",
    extendedMethodDescriptions[2317].autogenerate = true; // "ID3D11DeviceContext::OMGetRenderTargetsAndUnorderedAccessViews",
    extendedMethodDescriptions[2318].autogenerate = true; // "ID3D11DeviceContext::OMGetBlendState",
    extendedMethodDescriptions[2319].autogenerate = true; // "ID3D11DeviceContext::OMGetDepthStencilState",
    extendedMethodDescriptions[2320].autogenerate = true; // "ID3D11DeviceContext::SOGetTargets",
    extendedMethodDescriptions[2321].autogenerate = true; // "ID3D11DeviceContext::RSGetState",
    extendedMethodDescriptions[2322].autogenerate = true; // "ID3D11DeviceContext::RSGetViewports",
    extendedMethodDescriptions[2323].autogenerate = true; // "ID3D11DeviceContext::RSGetScissorRects",
    extendedMethodDescriptions[2324].autogenerate = true; // "ID3D11DeviceContext::HSGetShaderResources",
    extendedMethodDescriptions[2325].autogenerate = true; // "ID3D11DeviceContext::HSGetShader",
    extendedMethodDescriptions[2326].autogenerate = true; // "ID3D11DeviceContext::HSGetSamplers",
    extendedMethodDescriptions[2327].autogenerate = true; // "ID3D11DeviceContext::HSGetConstantBuffers",
    extendedMethodDescriptions[2328].autogenerate = true; // "ID3D11DeviceContext::DSGetShaderResources",
    extendedMethodDescriptions[2329].autogenerate = true; // "ID3D11DeviceContext::DSGetShader",
    extendedMethodDescriptions[2330].autogenerate = true; // "ID3D11DeviceContext::DSGetSamplers",
    extendedMethodDescriptions[2331].autogenerate = true; // "ID3D11DeviceContext::DSGetConstantBuffers",
    extendedMethodDescriptions[2332].autogenerate = true; // "ID3D11DeviceContext::CSGetShaderResources",
    extendedMethodDescriptions[2333].autogenerate = true; // "ID3D11DeviceContext::CSGetUnorderedAccessViews",
    extendedMethodDescriptions[2334].autogenerate = true; // "ID3D11DeviceContext::CSGetShader",
    extendedMethodDescriptions[2335].autogenerate = true; // "ID3D11DeviceContext::CSGetSamplers",
    extendedMethodDescriptions[2336].autogenerate = true; // "ID3D11DeviceContext::CSGetConstantBuffers",
    extendedMethodDescriptions[2337].autogenerate = true; // "ID3D11DeviceContext::ClearState",
    extendedMethodDescriptions[2338].autogenerate = true; // "ID3D11DeviceContext::Flush",
    extendedMethodDescriptions[2339].autogenerate = true; // "ID3D11DeviceContext::GetType",
    extendedMethodDescriptions[2340].autogenerate = true; // "ID3D11DeviceContext::GetContextFlags",
    extendedMethodDescriptions[2341].autogenerate = true; // "ID3D11DeviceContext::FinishCommandList",
    extendedMethodDescriptions[2342].autogenerate = true; // "ID3D11Device::QueryInterface",
    extendedMethodDescriptions[2343].autogenerate = true; // "ID3D11Device::AddRef",
    extendedMethodDescriptions[2344].autogenerate = true; // "ID3D11Device::Release",
    extendedMethodDescriptions[2345].autogenerate = true; // "ID3D11Device::CreateBuffer",
    extendedMethodDescriptions[2346].autogenerate = true; // "ID3D11Device::CreateTexture1D",
    extendedMethodDescriptions[2347].autogenerate = true; // "ID3D11Device::CreateTexture2D",
    extendedMethodDescriptions[2348].autogenerate = true; // "ID3D11Device::CreateTexture3D",
    extendedMethodDescriptions[2349].autogenerate = true; // "ID3D11Device::CreateShaderResourceView",
    extendedMethodDescriptions[2350].autogenerate = true; // "ID3D11Device::CreateUnorderedAccessView",
    extendedMethodDescriptions[2351].autogenerate = true; // "ID3D11Device::CreateRenderTargetView",
    extendedMethodDescriptions[2352].autogenerate = true; // "ID3D11Device::CreateDepthStencilView",
    extendedMethodDescriptions[2353].autogenerate = true; // "ID3D11Device::CreateInputLayout",
    extendedMethodDescriptions[2354].autogenerate = true; // "ID3D11Device::CreateVertexShader",
    extendedMethodDescriptions[2355].autogenerate = true; // "ID3D11Device::CreateGeometryShader",
    extendedMethodDescriptions[2356].autogenerate = true; // "ID3D11Device::CreateGeometryShaderWithStreamOutput",
    extendedMethodDescriptions[2357].autogenerate = true; // "ID3D11Device::CreatePixelShader",
    extendedMethodDescriptions[2358].autogenerate = true; // "ID3D11Device::CreateHullShader",
    extendedMethodDescriptions[2359].autogenerate = true; // "ID3D11Device::CreateDomainShader",
    extendedMethodDescriptions[2360].autogenerate = true; // "ID3D11Device::CreateComputeShader",
    extendedMethodDescriptions[2361].autogenerate = true; // "ID3D11Device::CreateClassLinkage",
    extendedMethodDescriptions[2362].autogenerate = true; // "ID3D11Device::CreateBlendState",
    extendedMethodDescriptions[2363].autogenerate = true; // "ID3D11Device::CreateDepthStencilState",
    extendedMethodDescriptions[2364].autogenerate = true; // "ID3D11Device::CreateRasterizerState",
    extendedMethodDescriptions[2365].autogenerate = true; // "ID3D11Device::CreateSamplerState",
    extendedMethodDescriptions[2366].autogenerate = true; // "ID3D11Device::CreateQuery",
    extendedMethodDescriptions[2367].autogenerate = true; // "ID3D11Device::CreatePredicate",
    extendedMethodDescriptions[2368].autogenerate = true; // "ID3D11Device::CreateCounter",
    extendedMethodDescriptions[2369].autogenerate = true; // "ID3D11Device::CreateDeferredContext",
    extendedMethodDescriptions[2370].autogenerate = true; // "ID3D11Device::OpenSharedResource",
    extendedMethodDescriptions[2371].autogenerate = true; // "ID3D11Device::CheckFormatSupport",
    extendedMethodDescriptions[2372].autogenerate = true; // "ID3D11Device::CheckMultisampleQualityLevels",
    extendedMethodDescriptions[2373].autogenerate = true; // "ID3D11Device::CheckCounterInfo",
    extendedMethodDescriptions[2374].autogenerate = true; // "ID3D11Device::CheckCounter",
    extendedMethodDescriptions[2375].autogenerate = true; // "ID3D11Device::CheckFeatureSupport",
    extendedMethodDescriptions[2376].autogenerate = true; // "ID3D11Device::GetPrivateData",
    extendedMethodDescriptions[2377].autogenerate = true; // "ID3D11Device::SetPrivateData",
    extendedMethodDescriptions[2378].autogenerate = true; // "ID3D11Device::SetPrivateDataInterface",
    extendedMethodDescriptions[2379].autogenerate = true; // "ID3D11Device::GetFeatureLevel",
    extendedMethodDescriptions[2380].autogenerate = true; // "ID3D11Device::GetCreationFlags",
    extendedMethodDescriptions[2381].autogenerate = true; // "ID3D11Device::GetDeviceRemovedReason",
    extendedMethodDescriptions[2382].autogenerate = true; // "ID3D11Device::GetImmediateContext",
    extendedMethodDescriptions[2383].autogenerate = true; // "ID3D11Device::SetExceptionMode",
    extendedMethodDescriptions[2384].autogenerate = true; // "ID3D11Device::GetExceptionMode",
}
