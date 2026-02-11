/**************************************************************************
 *
 */

#include "TraceReader.h"
#include <sstream>
//#include <fstream> // upgraded 
//#include <cstdio>
#include "IncludeLog.h"
#include <cstring>
#include <cstdlib>

#ifndef USE_FSTREAM_H

#include <iostream>
    using namespace std;
#endif

#define IS_WHITE(c) ((c)==' ' || (c)=='\t')
#define IS_MARK(c) ((c)==',' || (c)==')')


using includelog::logfile; // Make log object visible

TraceReader::TraceReader() : mode(TraceReader::hex), line(1), directivesOn(true), batchesAsFrames(false), specialStr("")
{
}

TraceReader::TR_MODE TraceReader::getMode() const
{
    return mode;
}

void TraceReader::skipWhites()
{
    TRACING_ENTER_REGION("skipWhites", "TraceReader", "skipWhites")
    
    if ( mode == binary )
        return ;

    if ( trEOS() )
        return ;

    char c = (char)trPeek();

    while ( !trEOS() && IS_WHITE(c) )
    {
        trIgnore();
        c = (char)trPeek();
    }

    TRACING_EXIT_REGION()
}

bool TraceReader::open( const char* ProfilingFile )
{
    strcpy(filePath, ProfilingFile);
    return trOpen(ProfilingFile);
}



bool TraceReader::open( const char* ProfilingFile,
                        const char* bufferDescriptorsFile,
                        const char* memoryFile )
{
    if ( TraceReader::open(ProfilingFile ) )
    {
        bm.open(bufferDescriptorsFile, memoryFile, BufferManager::Loading);
        return true;
    }
    return false;
}


bool TraceReader::parseEnum( unsigned int& value )
{
    TRACING_ENTER_REGION("parseEnum", "TraceReader", "parseEnum")
    value = 0; // reset
    char c = trPeek();
    if ( '0' <= c && c <= '9'  )
    {
        // HEX or DEC value 
        bool success = parseNumber(value);
        
        TRACING_EXIT_REGION()
        
        return success;
    }
    else
    {
        // GL Constant 
        char temp[256];
        int i = 0;
        c = trPeek();
        while (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
               ('0' <= c && c <= '9') || (c == '_'))
        {
            trIgnore();
            temp[i++] = c;
            c = trPeek();
        }
        temp[i] = '\0';

        if ( (i = GLResolver::getConstantValue(temp)) < 0 )
        {
            char buffer[256];
            sprintf(buffer,"Unknown constant string: \"%s\" in line: %d", temp, line );
            CG_ASSERT(buffer);
        }
        value = (unsigned int)i;
        TRACING_EXIT_REGION()
        return true;
    }
}

bool TraceReader::parseNumber( unsigned int& value )
{
    TRACING_ENTER_REGION("parseNumber", "TraceReader", "parseNumber")
    // New and faster but not safer implementation...
    char c;
    value = 0;

    c = trPeek();
    if ( c == '0' )
    {
        trIgnore();
        c = trPeek();
        if ( c == 'X' || c == 'x' )
        {
            // Assume hex value 0x...
            trIgnore(); // ignore 0 and x
            f >> std::hex;
            trReadFormated(&value);
            f >> std::dec;
        }
        // else (assume 0)
        TRACING_EXIT_REGION()
        return true;
    }
    else if ( '1' <= c && c <= '9' )
    {
        trReadFormated(&value);
        TRACING_EXIT_REGION()
        return true;
    }
    else if ( c == '-' )
    {
        trIgnore(); // ignore '-'
        trReadFormated(&value);
        value = -value;
        TRACING_EXIT_REGION()
        return true;
    }
    else
    {
        stringstream ss;
        ss << "Number expected... Last APICall: " << GLResolver::getFunctionName(lastCall);
        CG_ASSERT(ss.str().c_str());
    }


    CG_ASSERT("Legacy code reached");

    // Old version - more robust

    // debug
    char myBuf[256];
    // fdebug


    value = 0;

    c = trPeek();

    if ( '0' <= c && c <= '9' )
    {
        trIgnore(); // remove caracter from input stream

        value = (int)c - 48;

        c = (char)trPeek();

        if ( c == 'x' || c == 'X' ) // it is an hex value
        {

            unsigned int partial = 0;

            trIgnore(); // remove caracter from input stream ( it is a 'x' or 'X'

            c = (char)trPeek();

            while (('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F'))
            {
                trIgnore(); // remove caracter from input stream

                if ( 'a' <= c && c <= 'f' )
                    partial = c - 'a' + 10;
                else if ( 'A' <= c && c <= 'F' )
                    partial = c - 'A' + 10;
                else
                    partial = (int)c - 48;

                value = (value << 4) + partial; // value * 16 + partial 

                c = (char)trPeek(); // next
            }
        }
        else if ( '0' <= c && c <= '9' )
        {
            value = value * 10 + ((int)c - 48);

            trIgnore();

            c = (char)trPeek();
            while ( '0' <= c && c <= '9' )
            {
                trIgnore();

                value = value * 10 + ((int)c - 48);

                c = (char)trPeek();
            }
        }

        return true; // some value has been parsed
    }

    sprintf(myBuf,"Error parsing a number in line %d. First character found before parsing: '%c'",line,c);
    CG_ASSERT(myBuf);
    return false; // not a number
}


APICall TraceReader::parseApiCall()
{
    TRACING_ENTER_REGION("parseApiCall", "TraceReader", "parseApiCall")
    using namespace std;

    if ( trEOS() )
    {
        TRACING_EXIT_REGION()
        return APICall_UNDECLARED;
    }
    
    if ( mode == binary )
    {
        unsigned short apiID;
        trRead(&apiID,2);
        TRACING_EXIT_REGION()
        return (APICall)apiID;
    }

    char c;

    while ( !trEOS() )
    {
        skipWhites();

        c = trPeek();

        if ( c != '#' && c != '\n' )
            break;

        if ( c == '\n' )
        {
            line++;
            trIgnore();
        }
        else // must be '#'
        {
            // check if it is a command
            trIgnore(); // skip #
            c = trPeek();
            if ( c != '!' )
            {
                // It's not a comand (#!), just a comment (#)
                skipLine();
                continue;
            }

            // It is command (Process command)
            trIgnore(); // Ignore '!'
            skipWhites();
            char command[1024];
            int i = 0;
            c = trPeek();
            while ( c != ' ' && c != '\n' && c != '\t' )
            {
                command[i] = c;
                trIgnore();
                c = trPeek();
                i++;
            }
            command[i] = 0;
            if ( strcmp(command, "PRINT") == 0 )
            {
                skipWhites();
                trGetline(command, sizeof(command));
                cout << "output:> " << command << endl;
                popup("Output PRINT command", command);
            }
            else if ( strcmp(command, "END") == 0 && directivesOn )
            {
                cout << "output:> Finishing trace (END command found)" << endl;
                skipLine();
                TRACING_EXIT_REGION()
                return APICall_UNDECLARED; // Force finishing trace
            }
            else if ( strcmp(command, "SWAPBUFFERS") == 0 && directivesOn )
            {
                cout << "output:> sending SWAPBUFFERS command" << endl;
                skipLine();
                TRACING_EXIT_REGION()
                return APICall_wglSwapBuffers;
            }
            else if ( strcmp(command, "BATCHES_AS_FRAMES") == 0 )
            {
                skipWhites();
                trGetline(command, sizeof(command));
                if ( strncmp(command, "ON",2) == 0 )
                {
                    cout << "output:> BATCHES_AS_FRAMES enabled (ON)" << endl;
                    batchesAsFrames = true;
                }
                else
                {
                    cout << "output:> BATCHES_AS_FRAMES disabled (OFF)" << endl;
                    batchesAsFrames = false;
                }
            }
            else if ( strcmp(command, "LABEL") == 0 && directivesOn )
            {
                cout << "output:> LABEL command not supported yet" << endl;
                skipLine();
            }
            else if ( strcmp(command, "LOOP") == 0 && directivesOn )
            {
                cout << "output:> LOOP command not supported yet" << endl;
                skipLine();
            }
            else if ( strcmp(command, "DUMP_CTX") == 0 )
            {
                cout << "output:> sending DUMP_CTX command..." << endl;
                specialStr  = "DUMP_CTX";
                skipLine();
                TRACING_EXIT_REGION()
                return APICall_SPECIAL; // send a SPECIAL message
            }
            else if ( strcmp(command, "DUMP_STENCIL") == 0 )
            {
                cout << "output:> sending DUMP_STENCIL command..." << endl;
                specialStr  = "DUMP_STENCIL";
                skipLine();
                TRACING_EXIT_REGION()
                return APICall_SPECIAL; // send a SPECIAL message
            }
            else
                skipLine(); // ignore unknown command
        }
    }


    if ( trEOS() )
    {
        TRACING_EXIT_REGION()
        return APICall_UNDECLARED;
    }

    char buffer[256];


    // Improved version
    trGetline(buffer, sizeof(buffer),'(');

    GPU_TRACE_LOG(
        stringstream ss;
        ss << "Parsing call " << buffer << " at line " << line << "\n";
        logfile().pushInfo(__FILE__,__FUNCTION__);
        logfile().write(includelog::Debug, ss.str());
        logfile().popInfo();
    )

    TRACING_ENTER_REGION("APICallMap", "TraceReader", "parseAPICall")
    map<string,APICall>::iterator it = translatedCalls.find(buffer);
    TRACING_EXIT_REGION()
    if ( it != translatedCalls.end() )
    {
        //popup("Processing call", buffer);
        TRACING_EXIT_REGION()
        return (lastCall = it->second);
    }

    TRACING_ENTER_REGION("resolveAPICall", "TraceReader", "parseAPICall")
    lastCall = GLResolver::getFunctionID(buffer);
    translatedCalls.insert(make_pair(string(buffer), lastCall));;
    TRACING_EXIT_REGION()
    if ( lastCall == APICall_UNDECLARED )
    {
        char temp[512];
        sprintf(temp, "APICall unknown: '%s' in line %d", buffer, line);
        CG_ASSERT(temp);
    }
    //popup("Processing call", buffer);
    TRACING_EXIT_REGION()
    return lastCall;

    // Old version...

    CG_ASSERT("Legacy code reached.");
    
    int i = 0;

    while ( !trEOS() && i < (sizeof(buffer)-1) && (c = trPeek()) != '(' )
    {
        if (('0'<=c && c<='9') || ('a'<=c && c<='z') || ('A'<=c && c <='Z'))
            buffer[i++] = c;
        else
        {
            return APICall_UNDECLARED; // Hacking for PFC 
            sprintf( buffer, "char %c cannot be part of an API call (line %d)", c, line);
            CG_ASSERT(buffer);
        }
        trIgnore();
    }

    if ( c != '(' )
    {
        buffer[i] = 0;
        char temp[2*sizeof(buffer)];
        sprintf(temp, "APICall name too long: '%s' in line", buffer, line);
        CG_ASSERT( temp);
    }

    trIgnore(); // skip '(' 
    buffer[i] = 0;

    lastCall = GLResolver::getFunctionID(buffer);

    if ( lastCall == APICall_UNDECLARED )
    {
        char temp[512];
        sprintf(temp,  "APICall unknown: '%s' in line %d", buffer, line);
        CG_ASSERT(temp);
    }
    return lastCall;
}


bool TraceReader::readOring( unsigned int* value)
{
    TRACING_ENTER_REGION("readOring", "TraceReader", "readerOring")
    if ( mode == binary )//|| mode == hex )
    {
        bool success = readEnum(value);
        TRACING_EXIT_REGION()
        return success;
    }
    
    skipWhites();
    char c;

    // text mode 
    *value = 0;
    unsigned int temp = 0;
    while  ( true )
    {
        parseEnum(temp);
        skipWhites();
        c = (char)trPeek();
        if ( c == ')' || c == ',' )
        {
            // end of list of bitfields 
            *value |= temp;
            trIgnore(); // skip mark 
            TRACING_EXIT_REGION()
            return true;
        }
        else if ( c == '|' )
        {
            *value |= temp;
            trIgnore();
            skipWhites();
        }
        else
        {
            char aux[256];
            sprintf(aux, "Unexpected mark '%c' found in line %d", c, line);
            CG_ASSERT(aux);
        }
    }
    TRACING_EXIT_REGION()
}

bool TraceReader::readEnum( unsigned int* value)
{
    TRACING_ENTER_REGION("readEnum", "TraceReader", "readEnum")

    if ( mode == binary )
    {
        trRead(value,4);
        TRACING_EXIT_REGION()
        return true;
    }

    skipWhites();

    if ( mode == text || mode == hex )
    {
        parseEnum(*value); // supports both HEX & DEC modes 
        skipWhites();
        char c;
        trGet(c);
        if ( c == ',' || c == '}' || c == ')')
        {
            TRACING_EXIT_REGION()
            return true;
        }
        char myBuf[256];
        sprintf(myBuf,"Error reading value in line %d. Expected mark not found", line);
        CG_ASSERT(myBuf);
    }

    TRACING_EXIT_REGION()
    return false;

    // hex
    //return TraceReader::read(value);
}

/*
bool TraceReader::readPFD(void **ptr, char* data, int dataSize, ArrayTextType att )
{
    return readArray(ptr, data, dataSize, att);
}
*/

// 'ptr' will point data returned ( to buffer index or to data array )
// 'data' used for store text array or inlined array contents
// if it is a buffer index data do ot contain anything
// Remember: u must use ptr for extract data
bool TraceReader::readArray( void** ptr, char* data, int dataSize, ArrayTextType att )
{
    TRACING_ENTER_REGION("readArray", "TraceReader", "readArray")
    BufferDescriptor* buf = NULL;
    unsigned int bufId = 1;

    /***************
     * Binary mode *
     ***************/

    if ( mode == binary )
        CG_ASSERT("Binary mode not supported yet");

    /******************
     * READABLE MODES *
     ******************/

    int i;
    char c;

    skipWhites();

    c = (char)trPeek();

    switch ( c )
    {
        case '{': // inlined array ( vector )
            trIgnore();

            if ( mode == hex )
            {
                for ( i = 0; ; i++ )
                {
                    if ( !parseNumber(*(((unsigned int*)data)+i)) )
                    {
                        CG_ASSERT("Error parsing elem in inlined array");
                    }
                    else
                    {
                        c = (char)trPeek();
                        if ( c == '.' ) // it is a double number
                        {
                            trIgnore();
                            i++;
                            if ( !parseNumber(*(((unsigned int*)data)+i)) )
                            {
                                CG_ASSERT("Error parsing elem (2nd part) in inlined array");
                            }
                        }
                    }

                    skipWhites();
                    trGet(c);
                    if ( c == ',' )
                        skipWhites(); // go to next array item 
                    else if ( c == '}' ) // End of array 
                    {
                        trGet(c);
                        if ( c == ',' ) // array is not the last parameter 
                        {
                            skipWhites();
                            break; // end of parsing. go out from loop 
                        }
                        else if ( c == ')' ) // array is the last parameter 
                            break; // end of parsing. go out from loop 
                        else
                        {
                            char temp[256];
                            sprintf(temp, "Error parsing array. Expected mark not found. Mark found: %c", c);
                            CG_ASSERT(temp);
                        }
                    }
                } // end for 
            }
            else if ( mode == text )
            {
                for ( i = 0; ; i++ )
                {
                    skipWhites();
                    char cc = (char)trPeek();
                    if ( cc == ',' )
                        CG_ASSERT("Unexpected ',', a value was expected");
                    else if ( cc == '}' )
                        break; // end of parsing (empty array...)
                    else if ( cc != '-' && cc != '+' && ('0' > cc || cc > '9') )
                    {
                        char msg[256];
                        sprintf(msg, "Unexpected character: '%c', a value was expected", cc);
                        CG_ASSERT(msg);
                    }

                    switch ( att )
                    {
                        case TEXT_BYTE:
                            trReadFormated(data+i);
                            break;
                        case TEXT_SHORT:
                            trReadFormated((((unsigned short *)data)+i));
                            break;
                        case TEXT_INT:
                            trReadFormated((((unsigned int *)data)+i));
                            break;
                        case TEXT_FLOAT:
                            trReadFormated((((float *)data)+i));
                            break;
                        case TEXT_DOUBLE:
                            trReadFormated((((double *)data)+i));
                            break;
                        default:
                            CG_ASSERT("TEXT_TYPE unknown");
                    }

                    skipWhites();

                    trGet(c);

                    if ( c == ',' )
                        skipWhites(); // go to next array item 
                    else if ( c == '}' ) // End of array 
                    {
                        trGet(c);
                        if ( c == ',' ) // array is not the last parameter 
                        {
                            skipWhites();
                            break; // end of parsing. go out from loop 
                        }
                        else if ( c == ')' ) // array is the last parameter 
                            break; // end of parsing. go out from loop 
                        else
                        {
                            CG_ASSERT("Error parsing array. Expected mark not found");
                        }
                    }
                    else
                    {
                        char msg[256];
                        sprintf(msg, "Unexpected character: '%c'. simbols ',' or '}' expected", c);
                        CG_ASSERT(msg);
                    }
                }
            }
            *ptr = (void *)data; // return pointer 
            TRACING_EXIT_REGION()
            return true;
        case '*': // buffer index
            trIgnore();
            parseNumber(bufId);

            buf = bm.find(bufId);
            if ( buf == 0 )
            {
                stringstream ss;
                ss << "Buffer descriptor: " << bufId << " not found";
                CG_ASSERT(ss.str().c_str());
            }

            skipWhites();
            trGet(c);
            if ( c != ',' && c != ')' )
            {
                char temp[256];
                sprintf(temp, "Expected mark not found after reading buffer identifier. Read: '%c'", c);
                CG_ASSERT( temp);
            }

            *ptr = (void*)buf->getData();

            /*
            if ( *ptr == 0 )
            {
                stringstream ss;
                ss << "Buffer descriptor: " << buf->getID() << " returning NULL with getData()";
                popup("TraceReader::readArray", ss.str().c_str());
                //CG_ASSERT("Memory data not found!");
            }
            */

            TRACING_EXIT_REGION()
            return true;
        case '"': // text array
            trIgnore();
            trGetline(data,dataSize,'"');
            {
                //stringstream ss;
                //ss << "Data to parse: '" << data << "' linesBefore = " << line;
                {
                    for ( int i = 0; data[i]; i++ )
                    {
                        if ( data[i] == '\n' ) line++;
                    }
                }

                //ss << " linesAfter = " << line << "\n";
                //logfile().write(includelog::Debug, ss.str());
            }
            skipWhites();
            trGet(c);
            if ( c != ',' && c != ')' )
            {
                char temp[256];
                sprintf(temp, "Expected mark not found after reading a text array. Read: '%c'", c);
                CG_ASSERT( temp);
            }
            *ptr = (void *)data;
            TRACING_EXIT_REGION()
            return true;
        default:
            if ( '0' <= c && c <= '9' )
            {
                // using a pointer as an offset or maybe a null pointer 
                parseNumber(*((unsigned int *)ptr));
                skipWhites();
                trGet(c);
                if ( c != ',' && c != ')' )
                    CG_ASSERT("Expected mark not found after reading an offset (or null pointer)");
                TRACING_EXIT_REGION()
                return true;

            }
            else
            {
                stringstream ss;
                ss << "Unknown start buffer. Token: \"" << c << "\" in line: " << line
                    << " Last GL Call: " << GLResolver::getFunctionName(lastCall);
                CG_ASSERT(ss.str().c_str()); // should not happen ever 
            }
    }
    TRACING_EXIT_REGION()
    return false;
}

bool TraceReader::skipLine()
{
    TRACING_ENTER_REGION("skipLine", "TraceReader", "skipLine")
    
    if ( mode == binary )
    {
        TRACING_EXIT_REGION()
        return true;
    }
    
    char c;
    if ( trEOS() )
    {
        TRACING_EXIT_REGION()
        return true;
    }
    
    trGet(c);
    while ( !trEOS() && c != '\n' )
        trGet(c);

    line++;

    TRACING_EXIT_REGION()
    return true;
}

bool TraceReader::readBoolean( unsigned char* value)
{
    TRACING_ENTER_REGION("readBoolean", "TraceReader", "readBoolean")
    
    if ( mode == binary )
        CG_ASSERT("Binary reads not implemented yet");

    unsigned char b;

    skipWhites();
    trReadFormated(&b);
    skipWhites();

    char c;
    trGet(c); // skip mark char

    if ( c == ',' || c == '}' || c == ')' )
        ;
    else
    {
        char buffer[256];
        sprintf(buffer,"Error reading value, current char is: '%c'",c);
        CG_ASSERT(buffer);
        TRACING_EXIT_REGION()
        return false;
    }


    if ( b == '0' )
        *value = 0;
    else if (b == '1')
        *value = 1;
    else
        *value = b;
    TRACING_EXIT_REGION()
    return true;
}


// double representation in a tracefile 0xAAAAAAAA.0xBBBBBBBBB
bool TraceReader::readDouble( double *value )
{
    TRACING_ENTER_REGION("readDouble", "TraceReader", "readDouble")
    char buffer[256];

    unsigned int a;
    unsigned int b;
    unsigned int *ptrU;
    char c;

    if ( mode == binary )
    {
        // 8 raw bytes
        trRead(value,sizeof(double));
        TRACING_EXIT_REGION()
        return true;
    }

    if ( mode == text )
    {
        trReadFormated(value);
        skipWhites();
        trGet(c);
        if ( c == ',' || c == '}' || c == ')' )
        {
            TRACING_EXIT_REGION()
            return true;
        }            
        sprintf(buffer,"Error reading value in line %d, current char is: '%c'",line,c);
        CG_ASSERT(buffer);
        return false;
    }


    *value = 0; // reset ( for debug purpouse )

    skipWhites(); // skip previous whites

    if ( mode == hex )
    {
        if ( !parseNumber(a) ) {
            CG_ASSERT("Error parsing 1st HEX");
            return false;
        }
        trGet(c);
        if ( c != '.' ) {
            CG_ASSERT("Error parsing '.' character");
            return false;
        }
        if ( !parseNumber(b) )
        {
            CG_ASSERT("Error parsing 2nd HEX");
            return false;
        }

        ptrU = (unsigned int *)value;
        *ptrU = a;
        *(ptrU+1) = b;

        skipWhites();

        trGet(c); // skip mark char

        if ( c == ',' || c == '}' || c == ')' )
        {
            TRACING_EXIT_REGION()
            return true;
        }
        sprintf(buffer,"Error reading value, current char is: '%c'",c);
        CG_ASSERT(buffer);
        return false;

    }
    CG_ASSERT("Error reading value");
    return false; // text no supported
}


unsigned int TraceReader::currentLine() const
{
    return line;
}

#ifdef WIN32
    #define MSC8_LOCALE_FIX
#endif

#ifdef MSC8_LOCALE_FIX

class MSC8LocaleFix : public std::numpunct_byname<char>
{
    public:
        MSC8LocaleFix (const char *name)
            : std::numpunct_byname<char>(name)
        {
        }
    protected:
        virtual char_type do_thousand_sep() const
        {
            return '\0';
        }

        virtual string do_grouping() const
        {
            return "\0";
        }
};

#endif

bool TraceReader::trOpen( const char* file )
{
    logfile().pushInfo(__FILE__,__FUNCTION__);
    f.open(file, ios::binary | ios::in); // check if the file exists...
    if ( !f )
    {
        logfile().write(includelog::Init, "Input trace file not found");
        CG_ASSERT("Input trace file not found");
    }

    mode = readTraceFormat();

    f.close();

    if ( mode == binary )
        f.open(file, std::ios::binary | ios::in );
    else
        f.open(file, ios::binary | ios::in); // text 

    if ( !f )
    {
        logfile().write(includelog::Init, "Input trace file not found (2)");
        CG_ASSERT("Input trace file not found (2)");
    }

    f.ignore(7); // skip app string 

#ifdef MSC8_LOCALE_FIX
    std::locale loc2 (std::locale("C"), new MSC8LocaleFix("C"));
    f.imbue(loc2);
    locale loc = f.getloc();
#endif

    logfile().popInfo();
    return true;
}

int TraceReader::trPeek()
{
    TRACING_ENTER_REGION("trPeek", "TraceReader", "trPeek")
    int p = f.peek();
    TRACING_EXIT_REGION()
    return p;
}

void TraceReader::trIgnore( int count )
{
    TRACING_ENTER_REGION("trIgnore", "TraceReader", "trIgnore")
    f.ignore(count);
    TRACING_EXIT_REGION()
}


void TraceReader::trGet( char& c )
{
    TRACING_ENTER_REGION("trGet", "TraceReader", "trGet")
    f.get(c);
    TRACING_EXIT_REGION()
}

void TraceReader::trGetline( char* buffer, int bufSize, char eol )
{
    TRACING_ENTER_REGION("trGetLine", "TraceReader", "trGetLine")
    f.getline(buffer,bufSize,eol);
    TRACING_EXIT_REGION()
}

int TraceReader::trEOS()
{
    TRACING_ENTER_REGION("trEOS", "TraceReader", "trEOS")
    bool endOfFile = f.eof();
    TRACING_EXIT_REGION()
    return endOfFile;
}

void TraceReader::trRead(void* data, int size)
{
    TRACING_ENTER_REGION("trRead", "TraceReader", "trRead")
    f.read((char *)data,size);
    TRACING_EXIT_REGION()
}


long TraceReader::trGetPos()
{
    TRACING_ENTER_REGION("trGetPos", "TraceReader", "trGetPos")
    long pos = f.tellg();
    TRACING_EXIT_REGION()
    return pos;
}


void TraceReader::trSetPos(long pos)
{
    TRACING_ENTER_REGION("trSetPos", "TraceReader", "trSetPos")
    f.seekg(pos, ios::beg); // Absolute positioning
    TRACING_EXIT_REGION()
}



void TraceReader::readResolution(unsigned int& width, unsigned int& height)
{
    char line[256];
    unsigned int prevPos = f.tellg();
    //f.seekg(posRes);
    f.getline(line, sizeof(line));

    if ( strncmp("#! RES", line, 6) != 0 )
    {
        // There is not defined a resolution in the tracefile
        width = 0;
        height = 0;
        return ;
    }

    width = atoi(&line[6]);

    int i = 2;
    while ( line[i] != 0 && line[i] != 'x')
        i++;
    if ( line[i] = 0 )
        CG_ASSERT("Height not found");

    height = atoi(&line[i+1]);

    f.seekg(prevPos, ios::beg);

    /*
    char msg[256];
    sprintf(msg, "hres = %d  vres = %d", width, height);
    MessageBox(NULL, msg, "Message", MB_OK);
    */
}

TraceReader::TR_MODE TraceReader::readTraceFormat()
{    
    char line[256];
    f.getline(line,sizeof(line));

    posRes = f.tellg(); // store resolution directive position

    //if (( strlen(line) == 6 )
    //{
    //    CG_ASSERT(line);
    //}
    if ( strncmp("GLI0.",line,5) != 0 )
    {
        CG_ASSERT(line);
    }

    this->line++; // go to line 2 

    if ( line[5] == 'b' )
    {
        //popup("TraceReader::readTraceFormat()","Trace file in BINARY mode");
        return binary;
    }
    else if ( line[5] == 'h' )
    {
        //popup("TraceReader::readTraceFormat()","Trace file in HEX mode");
        return hex;
    }
    else if ( line[5] == 't' )
    {
        //popup("TraceReader::readTraceFormat()","Trace file in TEXT mode");
        return text;
    }

    CG_ASSERT(line);
    return text; // dummy 
}


bool TraceReader::skipUnknown()
{
    TRACING_ENTER_REGION("skipUnknown", "TraceReader", "skipUnknown")
    
    if ( binary ) // in binary unknown type occupies 0 bytes
    {
        TRACING_EXIT_REGION()
        return true;
    }
    else
    {
        // unknown format "U0xAAAAAA"
        // HEX or TEXT
        skipWhites();
        char c = trPeek();
        if ( c == 'U' || c == 'u' )
            trIgnore();
        else
        {
            //CG_ASSERT("Incorrect format for unknown value");

        }

        while ( !trEOS() && c != ',' && c != ')' && c != '}' )
            trGet(c); // skip unknown
        if ( trEOS() )
        {
            CG_ASSERT("Incorrect format for unknown value");
        }

        TRACING_EXIT_REGION()
        return true;
    }
}

bool TraceReader::skipResult()
{
    TRACING_ENTER_REGION("skipResult", "TraceReader", "skipResult")
    if ( mode == binary )
    {
        TRACING_EXIT_REGION()
        return true;
    }
    
    // text or hex
    while ( trPeek() != '\n' ) // find return (eol)
        trIgnore();

    TRACING_EXIT_REGION()
    return true;
}

bool TraceReader::skipCall()
{
    TRACING_ENTER_REGION("skipCall", "TraceReader", "skipCall")
    if ( mode == text || mode == hex )
    {
        char c;
        if ( trEOS() )
        {
            CG_ASSERT("End of stream was reached");
        }
        trGet(c);
        while ( !trEOS() && c != ')' )
        {
            if ( c == '\n' )
                line++;
            trGet(c);
        }
        // found ')' and skipped 
        skipWhites();

        if ( trEOS() )
        {
            TRACING_EXIT_REGION()
            return true;
        }
        
        if ( (char)trPeek() == '\n' )
        {
            line++;
            trIgnore(); // call skipping finished 
            TRACING_EXIT_REGION()
            return true;
        }
        else if ( (char)trPeek() == '=' )
        {
            // return value found. Must be skipped 
            trGet(c);
            while ( !trEOS() && c != '\n' )
                trGet(c);
            line++;
            TRACING_EXIT_REGION()
            return true;
        }
        else
        {
            if ( trPeek() == 13 ) // Skip 13 (NL)
            {
                trIgnore();
                TRACING_EXIT_REGION()
                return true;
            }
            char msg[256];
            sprintf(msg ,"Unexpected char: '%c' in tracefile line %d", (char)trPeek(), line);
            CG_ASSERT(msg);
        }
        TRACING_EXIT_REGION()
        return true;
    }
    CG_ASSERT("Binary mode still not supported");
    return true; // dummy. Avoid compiler warnings 
}


long TraceReader::getCurrentTracePos()
{
    return trGetPos();
}

void TraceReader::setCurrentTracePos(long pos)
{
    trSetPos(pos);
}


