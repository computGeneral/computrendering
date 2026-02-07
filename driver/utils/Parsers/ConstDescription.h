/**************************************************************************
 *
 */

#ifndef CONSTDESCRIPTION_H
    #define CONSTDESCRIPTION_H



class ConstDescription
{
private:
    
    char* name;
    int value;

    ConstDescription();
    ConstDescription( const ConstDescription& );

    static int parseHex( const char* hexStr );

    static char toStringBuffer[];

public:

    static const ConstDescription* parseConstant( const char* constantDef );

    int getValue() const;

    const char* getString() const;

    const char* toString() const;

    ~ConstDescription();
};


#endif
