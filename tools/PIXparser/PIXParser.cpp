
#include "PIXParser.h"
#include <cstdio>
#include <cstring>
#include <map>

using namespace std;

map<U32, AttributeDescriptor *> attributeDescriptors;
map<U32, EventDescriptor *> eventDescriptors;

main(int argc, char *argv[])
{
    FILE *f;

    if (argc < 2)
    {
        printf("Usage:\n");
        printf("  PIXParser <pixrunfile>\n");
        exit(-1);
    }

    f = fopen(argv[1], "rb");

    if (f == NULL)
    {
        printf("Error opening file : %s\n", argv[1]);
        exit(-1);
    }

    bool end = false;

    U32 readChunks = 0;

    while(!end)
    {
        Chunk *chunk = new Chunk;
        readChunk(f, chunk);

        printChunk(f, readChunks, chunk);

        switch(chunk->type)
        {
            case ATTRIBUTE_DESCRIPTOR:
                {
                    map<U32, AttributeDescriptor *>::iterator it;
                    it = attributeDescriptors.find(chunk->attribDesc.ADID);
                    if (it == attributeDescriptors.end())
                    {
                        attributeDescriptors[chunk->attribDesc.ADID] = &chunk->attribDesc;
                    }
                    else
                    {
                        printf("Attribute Descriptor %d already exists.\n");
                        exit(-1);
                    }
                }
                break;
            case EVENT_DESCRIPTOR:
                {
                    map<U32, EventDescriptor *>::iterator it;
                    it = eventDescriptors.find(chunk->eventDesc.EDID);
                    if (it == eventDescriptors.end())
                    {
                        eventDescriptors[chunk->eventDesc.EDID] = &chunk->eventDesc;
                    }
                    else
                    {
                        printf("Event Descriptor %d already exists.\n");
                        exit(-1);
                    }
                }
                break;
            default:
                deleteChunk(chunk);
                break;
        }
        
                
        readChunks++;
        end = (readChunks == 1000);
    }

    fclose(f);
}

void readChunk(FILE *f, Chunk *chunk)
{
    U32 size;
    U32 type;
    
    fread((char *) &size, sizeof(U32), 1, f);
    fread((char *) &type, sizeof(U32), 1, f);
    
    chunk->size = size;
    chunk->type = type;

    U08 *data = new U08[size - 4];
    fread((char *) data, 1, size - 4, f);
    chunk->data = data;

    switch(type)
    {
        case ATTRIBUTE_DESCRIPTOR:
            parseAttributeDescriptor(chunk, &chunk->attribDesc);
            break;
        case EVENT_DESCRIPTOR:
            parseEventDescriptor(chunk, &chunk->eventDesc);
            break;
        case EVENT:
            parseEvent(chunk, &chunk->event);
            break;
        case ASYNC_ATTRIBUTE:
            parseAsyncAttribute(chunk, &chunk->asyncAttrib);
            break;
        default:
            break;
    }
}

void parseAttributeDescriptor(Chunk *chunk, AttributeDescriptor *attrDesc)
{
    attrDesc->ADID = *((U32 *) &chunk->data[0]);
    attrDesc->type = *((U32 *) &chunk->data[4]);
    attrDesc->zero = *((U32 *) &chunk->data[8]);
    U32 size1 = copyWCharString(attrDesc->name, &chunk->data[12]);
    U32 size2 = copyWCharString(attrDesc->format, &chunk->data[12 + size1 + 4]);
}

void parseEventDescriptor(Chunk *chunk, EventDescriptor *eventDesc)
{
    eventDesc->EDID = *((U32 *) &chunk->data[0]);
    eventDesc->eventType = *((U32 *) &chunk->data[4]);
    U32 size1 = copyWCharString(eventDesc->name, &chunk->data[8]);
    eventDesc->attributeCount = *((U32 *) &chunk->data[8 + size1 + 4]);
    eventDesc->attributes = new EventAttributeDeclaration[eventDesc->attributeCount];
    U32 p = 8 + size1 + 4 + 4;
    for(U32 a = 0; a < eventDesc->attributeCount; a++)
    {
        eventDesc->attributes[a].ADID = *((U32 *) &chunk->data[p]);
        U32 size2 = copyWCharString(eventDesc->attributes[a].initialization, &chunk->data[p + 4]);
        p = p + 4 + size2 + 4;
    }
}

void parseEvent(Chunk *chunk, Event *event)
{
    chunk->event.EDID = *((U32 *) &chunk->data[0]);
    chunk->event.EID = *((U32 *) &chunk->data[4]);
    chunk->event.data = &chunk->data[8];
}

void parseAsyncAttribute(Chunk *chunk, AsyncAttribute *asyncAttrib)
{
    chunk->asyncAttrib.EID = *((U32 *) &chunk->data[0]);
    chunk->asyncAttrib.ADID = *((U32 *) &chunk->data[4]);
    chunk->asyncAttrib.data = &chunk->data[8];
}

void deleteChunk(Chunk *chunk)
{
    delete[] chunk->data;

    switch(chunk->type)
    {
        case ATTRIBUTE_DESCRIPTOR:
            delete[] chunk->attribDesc.name;
            delete[] chunk->attribDesc.format;
            break;
        case EVENT_DESCRIPTOR:
            delete chunk->eventDesc.name;
            for(U32 a = 0; a < chunk->eventDesc.attributeCount; a++)
            {
                delete[] chunk->eventDesc.attributes[a].initialization;                
            }
            delete[] chunk->eventDesc.attributes;
            break;        
    }

    delete chunk;
}

void printChunk(FILE *f, U32 chunkID, Chunk *chunk)
{
    printf("--- Chunk %08d ----\n", chunkID);
    printf("Type = ");
    switch(chunk->type)
    {
        case PIX_VERSION:
            printf("PIX_VERSION");
            break;
        case ATTRIBUTE_DESCRIPTOR:
            printf("ATTRIBUTE_DESCRIPTOR");
            break;
        case EVENT_DESCRIPTOR:
            printf("EVENT_DESCRIPTOR");
            break;
        case EVENT:
            printf("EVENT");
            break;
        case ASYNC_ATTRIBUTE:
            printf("ASYNC_ATTRIBUTE");
            break;
        case FOOTER:
            printf("FOOTER");
            break;
        default:
            printf("unknown 0x%08x", chunk->type);
            break;
    }
    printf("\n");
    printf("Size = %20d\n", chunk->size);
    printf("Data = \n");

    switch(chunk->type)
    {
        case ATTRIBUTE_DESCRIPTOR:
            printAttributeDescriptor(&chunk->attribDesc);
            break;
        case EVENT_DESCRIPTOR:
            printEventDescriptor(&chunk->eventDesc);
            break;
        case EVENT:
            printEvent(&chunk->event, chunk->size);
            break;
        case ASYNC_ATTRIBUTE:
            printAsyncAttribute(&chunk->asyncAttrib, chunk->size);
            break;
        default:
            printBuffer(chunk->data, chunk->size - 4);
            break;
    }

}

void printBuffer(U08 *data, U32 size)
{
    U32 p = 0;
    U32 b;
    for(b = 0; b < size; b++)
    {
        printf("%02x ", data[b]);
        p++;
        if (p == 16)
        {
            printf("    \"");
            for(U32 bb = 0; bb < 16; bb++)
            {
                char c = data[b - 15 + bb];
                if ((c < 0x20) || (c == 0x7F))
                    printf(" ");
                else
                    printf("%c", c);
            }
            printf("\"\n");
            p = 0;
        }
    }

    if (p > 0)
    {
        for(U32 t = p; t < 16; t++)
            printf("   ");
        printf("    \"");
        for(U32 bb = 0; bb < p; bb++)
        {
            char c = data[b - (p - 1) + bb];
            if ((c < 0x20) || (c == 0x7F))
                printf(" ");
            else
                printf("%c", c);
        }
        printf("\"\n");
    }
    printf("\n");
}

void printAttributeDescriptor(AttributeDescriptor *attrDesc)
{
    printf("  ADID     = 0x%08x\n", attrDesc->ADID);
    printf("  type     = 0x%08x\n", attrDesc->type);
    printf("  zero     = 0x%08x\n", attrDesc->zero);
    printf("  name     = ");
    printWCharString((U16 *) attrDesc->name);
    printf("\n");
    printf("  format   = ");
    printWCharString((U16 *) attrDesc->format);
    printf("\n");
}

void printEventDescriptor(EventDescriptor *eventDesc)
{
    printf("  EDID           = 0x%08x\n", eventDesc->EDID);
    printf("  eventType      = 0x%08x\n", eventDesc->eventType);
    printf("  name           = ");
    printWCharString((U16 *) eventDesc->name);
    printf("\n");
    printf("  attributeCount = 0x%08x\n", eventDesc->attributeCount);
    for(U32 a = 0; a < eventDesc->attributeCount; a++)
    {
        printf("  --- Event Attribute %04d ---\n", a);
        printf("    ADID = %08x\n", eventDesc->attributes[a].ADID);
        printf("    initialization = ");
        printWCharString((U16 *) eventDesc->attributes[a].initialization);
        printf("\n");
    }
}

void printEvent(Event *event, U32 size)
{
    printf("  EDID = %08x\n", event->EDID);
    printf("  EID  = %08x\n", event->EID);

    //  Search the event descriptor
    map<U32, EventDescriptor *>::iterator it;
    it = eventDescriptors.find(event->EDID);
    if (it == eventDescriptors.end())
    {
        printf("  Data : \n");
        printBuffer(event->data, size - 12);
    }
    else
    {
        printf("  Data : \n");
        printBuffer(event->data, size - 12);

        EventDescriptor *eventDesc = it->second;
        printf("  Event Info : \n");
        printf("    Event type = %08x\n", eventDesc->eventType);
        printf("    Event name = ");
        printWCharString((U16 *) eventDesc->name);
        printf("\n");
        printf("    Attributes :\n");
        U32 p = 0;
        for(U32 a = 0; a < eventDesc->attributeCount; a++)
        {
            //  Search for the attribute descriptor.
            map<U32, AttributeDescriptor*>::iterator it;
            it = attributeDescriptors.find(eventDesc->attributes[a].ADID);
            if (it == attributeDescriptors.end())
            {
                printf("     [%04d] => Not found\n", a);
                p = 0xDEADCAFE;
            }
            else
            {
                AttributeDescriptor *attrDesc = it->second;

                printf("     [%04d] => [Info : ",a);
                printf(" | %d ", attrDesc->type);
                printf(" | ");
                printWCharString((U16 *) attrDesc->name);
                printf(" | ");
                printWCharString((U16 *) attrDesc->format);
                printf(" | ");
                printWCharString((U16 *) eventDesc->attributes[a].initialization);
                printf(" ] = ");
                U08 *init = eventDesc->attributes[a].initialization;
                if (p == 0xDEADCAFE)
                {
                    printf("Data Unsynchronized");
                }
                else if ((init[0] == 'C') && (init[2] == 'a') && (init[4] == 'l') && (init[6] == 'c') &&
                         (init[8] == 'O') && (init[10] == 'n') && (init[12] == 'L') && (init[14] == 'o') &&
                         (init[16] == 'a') && (init[18] == 'd'))
                {
                    printf("Calculated On Load");
                }
                else if ((init[0] == 'A') && (init[2] == 's') && (init[4] == 'y') && (init[6] == 'n') &&
                         (init[8] == 'c'))
                {
                    printf("Async");
                }
                else if ((init[0] == '(') && (init[2] == 'e') && (init[4] == 'd') && (init[6] == 'i') &&
                         (init[8] == 'd') && (init[10] == ')'))
                {
                    printf("%d", event->EDID);
                }
                else if ((init[0] == '(') && (init[2] == 'e') && (init[4] == 'i') && (init[6] == 'd') &&
                         (init[8] == ')'))
                {
                    printf("%d", event->EID);
                }
                else if (init[0] == '(')
                {
                    switch(attrDesc->type)
                    {
                        case AT_FLOAT:
                            printf("%f", *((F32 *) &event->data[p]));
                            p += 4;
                            break;
                        case AT_STRING:
                            {
                                U32 size = (*((U32 *) &event->data[p]) + 1) * 2;
                                printWCharString((U16 *) &event->data[p + 4]);
                                p += size + 4;
                            }
                            break;
                        case AT_HEXUINT32:
                            printf("%08x", *((U32 *) &event->data[p]));
                            p += 4;
                            break;
                        case AT_UINT32:
                            printf("%10d", *((U32 *) &event->data[p]));
                            p += 4;
                            break;
                        case AT_EVENT:
                            {
                                U32 size = (*((U32 *) &event->data[p]) + 1) * 2;
                                printWCharString((U16 *) &event->data[p + 4]);
                                p += size + 4;
                            }
                            break;
                        case AT_UINT64:
                            printf("%20lld", *((U64 *) &event->data[p]));
                            p += 8;
                            break;
                        case AT_CHILDREN:
                            printf("%08x", *((U32 *) &event->data[p]));
                            p += 4;
                            break;
                        default:
                            printf("Format unknown");
                            p = 0xDEADCAFE;
                            break;
                    }
                }
                else
                {
                    printf("unknown");
                }

                printf("\n");
            }
        }
    }
}

void printAsyncAttribute(AsyncAttribute *asyncAttrib, U32 size)
{
    printf("  EID  = %08x\n", asyncAttrib->EID);
    printf("  ADID = %08x\n", asyncAttrib->ADID);

    //  Search for the attribute descriptor.
    map<U32, AttributeDescriptor*>::iterator it;
    it = attributeDescriptors.find(asyncAttrib->ADID);
    if (it == attributeDescriptors.end())
    {
        printf("  Data : \n");
        printBuffer(asyncAttrib->data, size - 12);
    }
    else
    {
        printf("  Data : \n");
        printBuffer(asyncAttrib->data, size - 12);

        AttributeDescriptor *attrDesc = it->second;

        printf("  Attribute Info => %d ", attrDesc->type);
        printf(" | ");
        printWCharString((U16 *) attrDesc->name);
        printf(" | ");
        printWCharString((U16 *) attrDesc->format);
        /*printf(" = ");
        switch(attrDesc->type)
        {
            case AT_FLOAT:
                printf("%f", *((F32 *) &event->data[p]));
                p += 4;
                break;
            case AT_STRING:
                {
                    U32 size = (*((U32 *) &event->data[p]) + 1) * 2;
                    printWCharString((U16 *) &event->data[p + 4]);
                    p += size + 4;
                }
                break;
            case AT_HEXUINT32:
                printf("%08x", *((U32 *) &event->data[p]));
                p += 4;
                break;
            case AT_UINT32:
                printf("%010d", *((U32 *) &event->data[p]));
                p += 4;
                break;
            case AT_EVENT:
                {
                    U32 size = (*((U32 *) &event->data[p]) + 1) * 2;
                    printWCharString((U16 *) &event->data[p + 4]);
                    p += size + 4;
                }
                break;
            case AT_UINT64:
                printf("%020lld", *((U64 *) &event->data[p]));
                p += 8;
                break;
            case AT_CHILDREN:
                printf("%08x", *((U32 *) &event->data[p]));
                p += 4;
                break;
            default:
                printf("Format unknown");
                p = 0xDEADCAFE;
                break;
        }*/
        printf("\n");
    }
}

void printWCharString(U16 *str)
{
    for(U32 p = 0; (str[p] != 0); p++)
    {
        U08 c = U08(str[p] & 0xFF);
        if ((c < 0x20) || (c == 0x7F))
            printf(" ");
        else
            printf("%c", c);
    }
}

U32 copyWCharString(U08 *&dest, U08 *src)
{
    U32 size = (*((U32 *) src) + 1) * 2;

    dest = new U08[size];

    memcpy(dest, &src[4], size);

    return size;
}

