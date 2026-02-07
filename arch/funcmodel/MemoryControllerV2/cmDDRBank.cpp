/**************************************************************************
 *
 */

#include "cmDDRBank.h"
#include <iostream>
#include <iomanip>

using namespace std;
using cg1gpu::memorycontroller::DDRBurst;
using cg1gpu::memorycontroller::DDRBank;

//U32 DDRBank::defRows = 4096;
//U32 DDRBank::defColumns = 512; // values of 32 bit
/*
void DDRBank::setDefaultDimensions(U32 rows, U32 columns)
{
    GPU_ASSERT
    (
        if ( rows == 0 || columns == 0 )
            CG_ASSERT("rows and columns must be defined greater than 0");
    )
    defRows = rows;
    defColumns = columns;
}

void DDRBank::getDefaultDimensions(U32& rows, U32& columns)
{
    rows = defRows;
    columns = defColumns;
}
*/
/*
DDRBank::DDRBank()
{
    nRows = defRows;
    nColumns = defColumns;
    activeRow = NoActiveRow;
    
    data = new U32*[nRows];
    for ( U32 i = 0; i < nRows; i++ )
    {
        data[i] = new U32[nColumns];
        for ( U32 j = 0; j < nColumns; j++ )
            data[i][j] = 0xDEADCAFE;
        //memset(data[i], 0xDEADCAFE, nColumns*sizeof(U32));
    }
}

*/

DDRBank::DDRBank(U32 rows, U32 cols) : 
    nRows(rows), nColumns(cols), activeRow(NoActiveRow)
{
    //cout << "ctor -> bank with " << rows << " rows and " << cols << " columns\n";
    data = new U32*[rows];
    for ( U32 i = 0; i < rows; i++ )
    {
        data[i] = new U32[cols];
        for ( U32 j = 0; j < cols; j++ )
            data[i][j] = 0xDEADCAFE; // Initialize memory
    }
}

DDRBank::DDRBank(const DDRBank& aBank)
{
    //cout << "ctor (cpy) -> bank with " << aBank.nRows << " rows and " << aBank.nColumns << " columns\n";
    nRows = aBank.nRows;
    nColumns = aBank.nColumns;
    activeRow = aBank.activeRow;

    data = new U32*[nRows];
    for ( U32 i = 0; i < nRows; i++ )
    {
        data[i] = new U32[nColumns];
        for ( U32 j = 0; j < nColumns; j++ )
            data[i][j] = aBank.data[i][j]; // Copy memory contents
    }
}


DDRBank& DDRBank::operator=(const DDRBank& aBank)
{
    //cout << "Bank Assign operator\n";
    if ( this == &aBank )
        return *this; // protect self-copying

    delete[] data; // delete previous memory array

    nRows = aBank.nRows;
    nColumns = aBank.nColumns;
    activeRow = aBank.activeRow;

    data = new U32*[nRows];
    for ( U32 i = 0; i < nRows; i++ )
    {
        data[i] = new U32[nColumns];
        for ( U32 j = 0; j < nColumns; j++ )
            data[i][j] = aBank.data[i][j]; // Copy memory contents
    }
    
    return *this;
}

U32 DDRBank::getActive() const
{
    return activeRow;
}

void DDRBank::activate(U32 row)
{
    GPU_ASSERT
    (
        if ( row >= nRows && row != NoActiveRow )
            CG_ASSERT("row out of bound");
    )
    activeRow = row;
}

void DDRBank::deactivate()
{
    activeRow = NoActiveRow;
}

DDRBurst* DDRBank::read(U32 column, U32 burstSize) const
{
    /*
    cout << "DDRBank::read(" << column << "," << burstSize << ")" << endl;
    cout << "Bank row contents: ";
    dumpRow(getActive(), column, burstSize*4);
    cout << endl;
    */

    GPU_ASSERT
    (
        if ( column >= nColumns )
            CG_ASSERT("Starting column out of bounds");
        if ( activeRow == NoActiveRow )
            CG_ASSERT("A row must be active");
    )

    DDRBurst* burst = new DDRBurst(burstSize);

    GPU_ASSERT
    (
        /*
        if ( column % burstSize != 0 )
        {
            cout << "Column = " << column << "  Current burst size = " << burstSize << "\n";
            CG_ASSERT("Starting column must be aligned to burst size");
        }
        */
        if ( burstSize + column > nColumns )
        {
cout << "DDRBank::read => column = " << column << " burstSize = " << burstSize << " nColumns = " << nColumns << endl;
            CG_ASSERT("Burst access out of row bounds");
        }
    )
    
    for ( U32 i = 0; i < burstSize; i++ )
        burst->setData(i, data[activeRow][i+column]);

    /*
    cout << "Burst contents: ";
    burst->dump();
    cout << endl;
    */

    return burst;
}

void DDRBank::write(U32 column, const DDRBurst* burst)
{
    GPU_ASSERT
    (   
        if ( column >= nColumns )
        {
            cout << "Column = " << column << " Total columns = " << nColumns << "\n";
            CG_ASSERT("Starting column out of bounds");
        }
        if ( activeRow == NoActiveRow )
            CG_ASSERT("A row must be active");
    )
    
    U32 burstSize = burst->getSize();
    
    GPU_ASSERT
    (
        /*
        if ( column % burstSize != 0 )
        {
            cout << "Column = " << column << "  Current burst size = " << burstSize << "\n";
            CG_ASSERT("Starting column must be aligned to burst size");
        }
        */
        if ( burstSize + column > nColumns )
        {
cout << "DDRBank::write => column = " << column << " burstSize = " << burstSize << " nColumns = " << nColumns << endl;
            CG_ASSERT("Burst access out of row bounds");         
        }
    )
    
    for ( U32 i = 0; i < burstSize; i++ )
        burst->write(i, data[activeRow][i+column]); // applies mask if required
}

U32 DDRBank::rows() const
{
    return nRows;
}

U32 DDRBank::columns() const
{
    return nColumns;
}

void DDRBank::dump(BankDumpFormat format) const
{
    cout << "Rows: " << nRows << " Columns: " << nColumns << "\n";
    if ( activeRow == NoActiveRow )
        cout << "[None active row]\n";
    cout << std::hex;
    for ( U32 i = 0; i < nRows; i++ )
    {
        cout << i << ": ";
        for ( U32 j = 0; j < nColumns; j++ )
        {
            const U08* ptr = (const U08*)&data[i][j];
            if ( format == DDRBank::hex )
            {
                cout << setfill('0') << setw(2) << U32(ptr[0]) 
                     << setfill('0') << setw(2) << U32(ptr[1]) 
                     << setfill('0') << setw(2) << U32(ptr[2]) 
                     << setfill('0') << setw(2) << U32(ptr[3]);
            }
            else // format == DDRBank::txt
            {
                cout << static_cast<char>(*ptr);
                cout << static_cast<char>(*(ptr+1));
                cout << static_cast<char>(*(ptr+2));
                cout << static_cast<char>(*(ptr+3));
            }
            if ( j < nColumns - 1 )
                cout << ".";
        }
        if ( activeRow == i )
            cout << " <--";
        cout << "\n";
    }
    cout << std::dec << endl;
}

void DDRBank::setBytes(U08 byte)
{
    for ( U32 i = 0; i < nRows; i++ )
    {
        for ( U32 j = 0; j < nColumns; j++ )
        {
            U08* p = (U08 *)&data[i][j];
            *p = byte;
            *(p+1) = byte;
            *(p+2) = byte;
            *(p+3) = byte;
        }
    }
}

void DDRBank::dumpRow(U32 row, U32 startCol, U32 bytes) const
{
    if ( bytes == 0 )
        bytes = 4*(nColumns - startCol); // Whole row

    if ( row > nRows )
        CG_ASSERT("Row specified too high");
    if ( bytes % 4 != 0 )
        CG_ASSERT("bytes parameter must be multiple of 4");
    if ( startCol + bytes/4 > nColumns )
        CG_ASSERT("accessing out of row bounds");

    //cout << "row=" << row << " startCol=" << startCol << " bytes=" << bytes << " Data: ";

    const U08* r = (const U08*)data[row];
    r += (startCol*4);

    for ( U32 i = 0; i < bytes; i++ )
        cout << std::hex << (U32)(r[i]) << std::dec << ".";
}



void DDRBank::readData(U32 row, U32 startCol, U32 bytes, ostream& outStream) const
{
    GPU_ASSERT(
        if ( bytes % 4 != 0 )
            CG_ASSERT("Number of bytes to be read has to be multiple of 4 (column size)");
        if ( row > nRows )
            CG_ASSERT("Row specified too high");
        if ( bytes % 4 != 0 )
            CG_ASSERT("bytes parameter must be multiple of 4");
        if ( startCol + bytes/4 > nColumns )
            CG_ASSERT("accessing out of row bounds");
    )

    // write data from the bank into the output stream
    outStream.write((const char*)&data[row][startCol], bytes);
}

void DDRBank::writeData(U32 row, U32 startCol, U32 bytes, std::istream& inStream)
{
    GPU_ASSERT(
        if ( bytes % 4 != 0 )
            CG_ASSERT("Number of bytes to be read has to be multiple of 4 (column size)");
        if ( row > nRows )
            CG_ASSERT("Row specified too high");
        if ( bytes % 4 != 0 )
            CG_ASSERT("bytes parameter must be multiple of 4");
        if ( startCol + bytes/4 > nColumns )
            CG_ASSERT("accessing out of row bounds");
    )

    // read data from stream and write it into the bank
    inStream.read((char*)&data[row][startCol], bytes);
}

