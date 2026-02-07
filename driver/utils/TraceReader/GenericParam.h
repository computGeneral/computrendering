/**************************************************************************
 *
 */

#ifndef GENERICPARAM_H
    #define GENERICPARAM_H

/**
 * Possible parameter types accepted
 */
union Param
{
    unsigned char boolean;
    signed char v8bit;
    unsigned char uv8bit;
    short v16bit;
    unsigned short uv16bit;
    int v32bit; 
    unsigned int uv32bit;
    float fv32bit;
    double fv64bit;
    void* ptr; // generic pointer ( cast must be done )
};


#endif // GENERICPARAM_H
