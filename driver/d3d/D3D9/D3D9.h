/**************************************************************************
 *
 */

#ifndef D3D_DRIVER_H_INCLUDED
#define D3D_DRIVER_H_INCLUDED

class D3DTrace;

class D3D9 
{
public:

    static void initialize(D3DTrace *trace);
    static void finalize();

private:

    D3D9();
};

#endif


