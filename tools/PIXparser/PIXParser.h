
#include <cstdio>
#include "GPUType.h"

#ifndef _PIXParser_
#define _PIXParser_


// PIXRun specific structures and id's , see
// "PixRunFileFormat.pdf" in doc
struct AttributeDescriptor
{
    U32 ADID;
    U32 type;
    U32 zero;
    U08 *name;
    U08 *format;
};

struct EventAttributeDeclaration
{
    U32 ADID;
    U08 *initialization;
};

struct EventDescriptor
{
    U32 EDID;
    U32 eventType;
    U08 *name;
    U32 attributeCount;
    EventAttributeDeclaration *attributes;
};

struct Event
{
    U32 EDID;
    U32 EID;
    U08 *data;
};


struct AsyncAttribute
{
    U32 EID;
    U32 ADID;
    U08 *data;
};

struct PackedCallPackage
{
    U32 size;
    U32 CID;
    U32 one;
};

// Type identifiers
enum ChunkType
{
    PIX_VERSION          = 0x03E8,
    ATTRIBUTE_DESCRIPTOR = 0x03E9,
    EVENT_DESCRIPTOR     = 0x03EA,
    EVENT                = 0x03EB,
    ASYNC_ATTRIBUTE      = 0x03EC,
    FOOTER               = 0x03ED
};

enum AttributeType
{
    AT_FLOAT = 0,
    AT_STRING = 1,
    AT_HEXUINT32 = 2,
    AT_UINT32 = 3,
    AT_EVENT = 4,
    AT_UINT64 = 5,
    AT_CHILDREN = 6,
    AT_PACKEDCALLPACKAGE = 7
};

struct Chunk
{
    U32 size;
    U32 type;
    U08 *data;

    union
    {
        AttributeDescriptor attribDesc;
        EventDescriptor eventDesc;
        Event event;
        AsyncAttribute asyncAttrib;
    };
};

void readChunk(FILE *f, Chunk *chunk);

void parseAttributeDescriptor(Chunk *chunk, AttributeDescriptor *attrDesc);

void parseEventDescriptor(Chunk *chunk, EventDescriptor *eventDesc);

void parseEvent(Chunk *chunk, Event *event);

void parseAsyncAttribute(Chunk *chunk, AsyncAttribute *asyncAttr);

void deleteChunk(Chunk *chunk);

void printChunk(FILE *f, U32 chunkID, Chunk *chunk);

void printBuffer(U08 *data, U32 size);

void printAttributeDescriptor(AttributeDescriptor *attrDesc);

void printEventDescriptor(EventDescriptor *eventDesc);

void printEvent(Event *event, U32 size);

void printAsyncAttribute(AsyncAttribute *asyncAttrib, U32 size);

void printWCharString(U16 *str);

U32 copyWCharString(U08 *&dest, U08 *src);

//const U32 ASYNC_ATTRIBUTE = 0x3EC;
//const U32 PACKED_CALL_PACKAGE = 19;

#endif

