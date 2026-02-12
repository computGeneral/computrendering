/**************************************************************************
 *
 */

#ifndef DDRBANK_H
    #define DDRBANK_H

#include "GPUType.h"
#include "cmDDRBurst.h"

namespace arch
{
namespace memorycontroller
{

/**
 * Class implementing a DDR Chip bank
 *
 * A DDR Bank has and array of data and what is call an active row which is the row
 * where read or write operations are applied.
 *
 * Each value of the array has 32-bit but the data is written/read using DDRBurst so
 * ordinary read/write operations imply read/write more than 1 single 32-bit values. Single
 * bytes can be written using DDRBurst masks
 */
class DDRBank
{
public:

    /**
     * Dump format
     */
    enum BankDumpFormat
    {
        txt,
        hex
    };

private:

    U32** data; ///< Array data
    U32 activeRow; ///< Current active row
    U32 nRows; ///< Number of rows in the DDR chip bank
    U32 nColumns; ///< Number of columns in each DDR chip bank row
    
    //static U32 defRows; ///< Default number of rows (initial value 4096)
    //static U32 defColumns; ///< Default number of columns per row (initial value 512)

    
    //DDRBank& operator=(const DDRBank&); ///< Forbid copy of DDRBank objects
    
public:

    /**
     * Constant used to identify that any row is active
     */
    static const U32 NoActiveRow = 0xFFFFFFFF;

    /**
     * Sets the default parameters (rows and columns per row) used to create DDRBank objects
     * with the default constructor
     *
     * @param rows number of rows used to create DDRBank objects
     * @param columns number of columns per row used to create DDRBank objects
     */
    //static void setDefaultDimensions(U32 rows, U32 columns);
    
    /**
     * Gets the default parameters used to create DDRBank objects
     *
     * @retval rows number of rows used to create DDRBanks objects 
     * @retval columns number of columns per row used to create DDRBank objects
     */
    //static void getDefaultDimensions(U32& rows, U32& columns);
    
    /**
     * Creates a DDR bank with rows and columns per row default values
     */
    //DDRBank();

    /**
     * Creates a DDR bank with a defined number of rows and colums
     *
     * The capacity of the bank is (rows * cols * 4) bytes (each col is 32-bits wide)
     */
    DDRBank(U32 rows, U32 cols);
    DDRBank(const DDRBank&);
    DDRBank& operator=(const DDRBank&);

    /**
     * Sets all bank bytes with a given value
     *
     * @param byte byte copied to all bank bytes
     */
    void setBytes(U08 byte);
    
    /**
     * Returns the current active row
     *
     * @return current active row
     */
    U32 getActive() const;
    
    /**
     * Sets the current active row
     *
     * @parameter row selected row
     */
    void activate(U32 row);

    /**
     * Sets the current active row equals to NoActiveRow
     *
     * @note This method is equivalent to DDRBank::activate(NoActiveRow)
     */
    void deactivate();
    
    /**
     * Reads a burst of data from the current active row
     *
     * @param column Starting column from where to retrieve the burst
     *
     * @warning The starting column must aligned to burst size
     */
    DDRBurst* read(U32 column, U32 burstSize) const;
    
    /**
     * Writes a burst of data to the current active row
     *
     * @param column Starting column where to write the burst
     * @param data DDRBurst to be written
     */
    void write(U32 column, const DDRBurst* data);    
    
    /**
     * Returns the number of rows
     *
     * @return number of bank rows
     */
    U32 rows() const;

    /**
     * Returns the number of columns per row
     *
     * @return number of columns per row
     */
    U32 columns() const;

    /**
     * Prints a readable information of the bank contents
     *
     * @param format txt means bytes formated as ASCII chars, hex prints bytes
     *               formated as numeric hex values
     *
     * @warning if the dimensions of the bank are too big the output presented may
     *          be very difficult to be understood
     */
    void dump(BankDumpFormat format = DDRBank::hex) const;

    /**
     * Dump a given number of bytes starting from a column
     */
    void dumpRow(U32 row, U32 startCol = 0, U32 bytes = 0) const;

    /**
     * Reads data from the bank and place it into an output stream (w/o intermediate copy)
     */
    void readData(U32 row, U32 startCol, U32 bytes, std::ostream& outStream) const;

    /**
     * Write data into the bank from an input stream (w/o intermediate copy)
     */
    void writeData(U32 row, U32 startCol, U32 bytes, std::istream& inputStream);

    
};

} // namespace arch
} // namespace memorycontroller

#endif // DDRBANK_H
