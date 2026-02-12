/**************************************************************************
 *
 * Dynamic Object definitions file.
 *
 */

#ifndef _DYNAMIC_OBJECT_H_
#define _DYNAMIC_OBJECT_H_

#include "GPUType.h"
#include "DynamicMemoryOpt.h"
#include <string>

#define MAX_COOKIES 8

namespace arch
{

/**
 * Must be inherited by all traceable Objects used in Signal's traffic
 */
class DynamicObject : public DynamicMemoryOpt
{
private:
    U32 cookies[MAX_COOKIES];    // array of tracing cookies
    U32 lastCookie;              // last object cookie
    U32 color;                   // color object
    U08 info[MAX_INFO_SIZE];     // additional info ( i.e text )

    static U32 nextCookie[];     // last cookie generated ( in a level )

public:

    /**
     * Creates a new DynamicObject without any information
     */
    DynamicObject();

    /**
     * Creates a new DynamicObject initialized with one cookie ( first level cookie )
     *
     * @param aCookie identifier of the first level cookie
     */
    DynamicObject( U32 aCookie);

    /**
     * Creates a new DynamicObject initialized with one cookie ( first level cookie )
     * and a specified color
     *
     * @param aCookie identifier of the first level cookie
     * @param color color identifier
     */
    DynamicObject( U32 aCookie, U32 color);

    /**
     * Overwrites all cookies in this DynamicObject with the cookies in the parent DynamicObject
     * specified as parameter ( tipically used to simulate the inheritance of cookies )
     *
     * @param parent from where cookies are copied
     */
    void copyParentCookies( const DynamicObject& parent );

    /**
     * Modifies the cookie identifier of the current cookie level
     *
     * @param aCookie new value for the current level cookie
     */
    void setCookie( U32 aCookie );

    /**
     * Sets a new Color for this DynamicObject
     *
     * @param aColor color identifier
     */
    void setColor( U32 aColor );

    /**
     * Adds a new cookie in the next level
     *
     * @param aCookie cookie identifier
     */
    void addCookie( U32 aCookie );

    /**
     * Adds a new cookie ( selected automatically by the cookie generator )
     */
    void addCookie();

    /**
     *
     *  Removes a cookie.
     *
     */

    void removeCookie();

    /**
     * Obtains a maximum of maxCookies ( first maxCookies ) for this DynamicObject.
     *
     * @param numCookies Reference to a variable where to store
     *  the current number of cookies for the dynamic object.
     *
     * @return The pointer to the array of cookies for the
     * dynamic object.
     */
    U32 *getCookies( U32 &numCookies );

    /**
     * Gets the color for this DynamicObject
     *
     * @return the color of this DynamicObject
     */
    U32 getColor();

    /**
     * Sets a new info for this DynamicObject ( replacing the current info )
     *
     * @param info information that is going to be attached to this DynamicObject
     */
    //void setInfo( U08* info );

    /**
     * Obtains the info associated to this DynamicObject
     *
     * @return string representing the information associated to this DynamicObject
     */
    U08* getInfo();

    const U08* getInfo() const;

    virtual  std::string toString() const;

};

} // namespace arch

#endif
