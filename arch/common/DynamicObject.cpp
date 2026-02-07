/**************************************************************************
 *
 * Dynamic Object implementation file.
 *
 */

#include "DynamicObject.h"
#include "support.h"
#include <iostream>

using namespace cg1gpu;

U32 DynamicObject::nextCookie[MAX_COOKIES]; // static implies zero initialization automatically

DynamicObject::DynamicObject() : lastCookie( 0 ), color(0)
{
    cookies[lastCookie] = nextCookie[lastCookie]++;

    //  Clear info field (zero string).  
    info[0] = 0;

    setTag("Dyn");
}

/*  Creates a dynamic object with one cookie.  The cookie is assigned from
    the internal cookie generator.  */
DynamicObject::DynamicObject( U32 aCookie ) : lastCookie( 0 ), color(0)
{
    cookies[lastCookie] = aCookie;

    //  Clear info field (zero string).  
    info[0] = 0;
}

//  Creates a dynamic object with one cookie and sets its color.  
DynamicObject::DynamicObject( U32 aCookie, U32 aColor ) : lastCookie( 0 )
{
    cookies[lastCookie] = aCookie;
    color = aColor;

    //  Clear info field (zero string).  
    info[0] = 0;
}

//  Copies cookie list from another dynamic object.  
void DynamicObject::copyParentCookies( const DynamicObject& parent )
{
    for ( lastCookie = 0; lastCookie <= parent.lastCookie; lastCookie++ )
        cookies[lastCookie] = parent.cookies[lastCookie];

    //  The for adds one false cookie.  
    lastCookie--;
}

//  Sets dynamic object last cookie.  
void DynamicObject::setCookie( U32 aCookie )
{
    cookies[lastCookie] = aCookie;
}

//  Adds a new cookie level.  
void DynamicObject::addCookie( U32 aCookie )
{
    lastCookie++;

    cookies[lastCookie] = aCookie;
}

//  Removes a cookie level for the object.  
void DynamicObject::removeCookie()
{
    if (lastCookie > 0)
        lastCookie--;
}

//  Adds a new cookie level using the internal cookie generator.  
void DynamicObject::addCookie()
{
    lastCookie++;

    cookies[lastCookie] = nextCookie[lastCookie]++; // increase cookie level generator
}

//  Sets dynamic object color.  
void DynamicObject::setColor( U32 aColor )
{
    color = aColor;
}

//  Returns a pointer the dynamic object cookies list.  
U32 *DynamicObject::getCookies( U32 &numCookies )
{
    numCookies = lastCookie + 1;

    return cookies;
}

//  Returns the object color.  
U32 DynamicObject::getColor()
{
    return color;
}

//  Sets dynamic object info field.  
//void DynamicObject::setInfo( U08* info )
//{
//    this->info = info;
//}

//  Returns dynamic object info field.  
U08* DynamicObject::getInfo()
{
    return info;
}

const U08* DynamicObject::getInfo() const
{
    return info;
}

std::string DynamicObject::toString() const
{
    return DynamicMemoryOpt::toString() + "   INFO: \"" + std::string((const char*)getInfo()) + "\"";
}

