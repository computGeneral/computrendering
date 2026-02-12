/**************************************************************************
 *
 */

#include <iostream>
#include <sstream>
#include <cstring>
#include "cmDDRBurst.h"

using namespace std;
using arch::memorycontroller::DDRBurst;

U32 DDRBurst::instances = 0;

DDRBurst::DDRBurst(U32 size) : size(size)
{
    ++instances;

    GPU_ASSERT
    (
        if ( size > MAX_BURST_SIZE )
        {
            cout << "Burst length = " << size << " Max. Burst length accepted = " 
                 << MAX_BURST_SIZE << "\n";
            CG_ASSERT("Max. burst length exceed");
        }
    )

    for ( U32 i = 0; i < size; i++ )
        masks[i] = 0xF;
}

DDRBurst::~DDRBurst()
{
    --instances;
}

U32 DDRBurst::countInstances()
{
    return instances;
}

void DDRBurst::setData(U32 position, U32 datum)
{
    GPU_ASSERT
    (
        if ( position >= size )
            CG_ASSERT("Position selected in the burst out of bounds");
    )
    values[position] = datum;
    masks[position] = 0xF; // no mask
}

void DDRBurst::setData(U32 position, U32 datum, U08 mask)
{
    GPU_ASSERT
    (
        if ( position >= size )
            CG_ASSERT("Position selected in the burst out of bounds");
        if ( mask > 0xF )
            CG_ASSERT("Mask out of bounds, must be in the range [0..15]");
    )
    values[position] = datum;
    masks[position] = mask;
}

void DDRBurst::setData(const U08* dataBytes, U32 dataSize)
{
    if ( dataSize > 4 * size )
        CG_ASSERT("Too many data supplied");

    if ( dataSize == 0 )
        CG_ASSERT("Amount of data supplied must be at least 1 byte");

    memcpy(values, dataBytes, dataSize); // copy data

    if ( dataSize < 4 * size) // partial burst ( 4*size > dataSize )
    {
        U32 i = dataSize / 4; // Compute the first 32-bit burst item to be masked
        if ( dataSize % 4 == 0 )
            ; // frequent case (do nothing)
        else
        {
            //if ( dataSize % 4 == 1 )
            //    masks[i] = 0x8;
            //else if ( dataSize % 4 == 2 )
            //    masks[i] = 0xC;
            //else // ( dataSize % 4 == 3 )
            //    masks[i] = 0xE;
            if ( dataSize % 4 == 1 )
                masks[i] = 0x1;
            else if ( dataSize % 4 == 2 )
                masks[i] = 0x3;
            else // ( dataSize % 4 == 3 )
                masks[i] = 0x7;
            i++; // next 32-bit item to be masked
        }
        // mask not set bytes
        for ( ; i < size; i++ )
            masks[i] = 0x0; // mask the four bytes of the 32-bit datum
    }
}

void DDRBurst::setMask(const U32* writeMask, U32 maskSize)
{
    CG_ASSERT_COND(!( maskSize > size ), "Mask size cannot be greater than burst length");    if ( maskSize == 0 ) 
        maskSize = size;
    
    // Mask conversion
    /*
    for ( U32 i = 0; i < maskSize; i++ )
    {
        masks[i] = 0x0;
        if ( writeMask[i] & 0xFF000000 == 0xFF000000)
            masks[i] = 0x1;
        if ( writeMask[i] & 0x00FF0000 == 0x00FF0000)
            masks[i] |= 0x2;
        if ( writeMask[i] & 0x0000FF00 == 0x0000FF00)
            masks[i] |= 0x4;
        if ( writeMask[i] & 0x000000FF == 0x000000FF)
            masks[i] |= 0x8;
    }
    */

    for ( U32 i = 0; i < maskSize; i++ )
    {
        masks[i] = 0x0;
        if ( (writeMask[i] & 0x000000FF) == 0x000000FF)
            masks[i] = 0x1;
        if ( (writeMask[i] & 0x0000FF00) == 0x0000FF00)
            masks[i] |= 0x2;
        if ( (writeMask[i] & 0x00FF0000) == 0x00FF0000)
            masks[i] |= 0x4;
        if ( (writeMask[i] & 0xFF000000) == 0xFF000000)
            masks[i] |= 0x8;
    }
}

void DDRBurst::write(U32 position, U32& destination) const
{
    GPU_ASSERT
    (
        if ( position >= size )
            CG_ASSERT("Position selected in the burst out of bounds");
    )

    U08 mask = masks[position]; // mask for this 32-bit value

    if ( mask == 0xF ) // optimize frequent case (write without mask)
    {
        destination = values[position];
        return ;
    }
    else if ( mask == 0x0 ) // all bytes masked
        return ;

    // Obtain byte address pointers to source and destination
    const U08* source = (const U08 *)&values[position];
    U08* dest = (U08 *)&destination;

    //if ( mask & 0x8 ) *dest = *source;
    //if ( mask & 0x4 ) *(dest+1) = *(source+1);
    //if ( mask & 0x2 ) *(dest+2) = *(source+2);
    //if ( mask & 0x1 ) *(dest+3) = *(source+3);
    if ( mask & 0x1 ) *dest = *source;
    if ( mask & 0x2 ) *(dest+1) = *(source+1);
    if ( mask & 0x4 ) *(dest+2) = *(source+2);
    if ( mask & 0x8 ) *(dest+3) = *(source+3);
}

void DDRBurst::dump() const
{
    cout << "Burst size: " << size
         << " Data: ";

    const U08* val = getBytes();
    cout << hex;
    for ( U32 i = 0; i < size*4; i++ )
        cout << U32(val[i]) << ".";

    cout << " Mask: ";
    for ( U32 i = 0; i < size; i++ )
    {
        U08 m = masks[i];
        if ( m & 0x8 ) cout << "1."; else cout << "0.";
        if ( m & 0x4 ) cout << "1."; else cout << "0.";
        if ( m & 0x2 ) cout << "1."; else cout << "0.";
        if ( m & 0x1 ) cout << "1."; else cout << "0.";
    }
    cout << dec;
}

const U08* DDRBurst::getBytes() const
{
    return (const U08*)values;
}

U32 DDRBurst::getSize() const
{
    return size;
}
