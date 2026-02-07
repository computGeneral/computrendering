/**************************************************************************
 *
 */

#ifndef RBANK_H
    #define RBANK_H

#include "GALTypes.h"
#include "QuadReg.h"
#include <iostream>
#include <string>
#include "support.h"

namespace libGAL
{

enum RType
{
    QR_UNKNOWN,
    QR_CONSTANT,
    QR_GLSTATE,
    QR_PARAM_LOCAL,
    QR_PARAM_ENV
};

/**
 * class implementing a generic RBank Register of four values per register
 *
 * @note Implemented using templates
 *
 * @author Carlos González Rodríguez - cgonzale@ac.upc.es
 * @date 5/7/2004
 * @ver 1.0
 *
 */
template<class T>
class RBank
{
private:

    static QuadReg<T> dummy;

    std::string name; ///< RBank name
    QuadReg<T>* bank; ///< RBank registers
    RType* types; ///< RBank registers type
    gal_int* indexes0; ///< RBank register indexes 0
    gal_int* indexes1; ///< RBank register indexes 1
    gal_bool* used; ///< bitmap of registers used
    gal_uint count; ///< number of registers within the RBank


public:
/*
    RBank() : count(0)
    {
        // default
        bank = NULL;
        types = NULL;
        used = NULL;
        indexes0 = NULL;
        indexes1 = NULL;
        //CG_ASSERT("Empty RBank created");
        warning("RBank","RBank constructor","Empty RBank created");
    }
*/
    /**
     * Creates a RBank of a given size
     *
     * @param size Number of registers
     */
    RBank(gal_uint size, const std::string& name="");

    RBank(const RBank<T>& rb);

    void setSize( gal_uint size );

    /**
     *
     *  RBank destructor.
     *
     */

    ~RBank();

    /**
     * Gets the contents of a given register in the RBank
     *
     * @param pos Register position in the RBank
     *
     * @return register in position pos
     */
    QuadReg<T>& get(gal_uint pos) const;

    /**
     * Gets the contents of a given register in the RBank
     *
     * @param pos Register position in the RBank
     *
     * @return register in position pos
     *
     * @note exact behaviour than RBank<T>::get()
     */
    QuadReg<T>& operator[](gal_uint pos);

    /**
     * Sets a register in the RBank with new value
     *
     * The register is also marked as used
     *
     * @param pos Register position in the RBank
     * @param value new contents
     */
    void set(gal_uint pos, const QuadReg<T>& value);

    /**
     * Mark this register as unused
     *
     * @param pos Register position in the RBank

     * @note Contents are reset to zero
     */
    void clear(gal_uint pos);

    /**
     * Get the type related with the register (QR_CONSTANT,    QR_GLSTATE,    QR_PARAM_LOCAL,    QR_PARAM_ENV) and
     * related indexes.
     *
     * @param pos Register position in the RBank
     * @param index0 In case of QR_PARAM_LOCAL or QR_PARAM_ENV is the index of parameter. In
     * case of QR_GLSTATE is the state identifier. Has no sense otherwise.
     * @param index1 In case of QR_GLSTATE is the row of the matrix or 0
     * @return type of register
     * @note Registers with uninitialized type returns QR_UNKNOWN. The register position has to be correct.
     */
    RType getType(gal_uint pos, gal_int& index0, gal_int& index1) const;

    /**
     * Set the type related with the register and type parameters (indexes)
     *
     * @param pos Register position in the RBank
     * @param type Type of register.
     * @param index0 In case of QR_PARAM_LOCAL or QR_PARAM_ENV is the index of parameter. In
     * case of QR_GLSTATE is the state identifier. Has no sense otherwise.
     * @param index1 In case of QR_GLSTATE is the row of the matrix or 0
     * @note The register position has to be correct.
     */
    void setType(gal_uint pos, RType type = QR_UNKNOWN, gal_int index0 = 0, gal_int index1 = 0);

    /**
     * Get the first register containing a specified value
     *
     * If a register with the specified value is not found then a free register is returned
     *
     * @param value Register value searched
     * @found out parameter indicating if a register with the specified value was found
     *
     * @retval found true if the register was found, false otherwise
     * @return position of a register found or position ofa new register or equals to size()
     * if there are not free registers
     */
    gal_uint getRegister(gal_bool& found, const QuadReg<T>& value, RType regType = QR_UNKNOWN, 
                         gal_int index0 = 0, gal_int index1 = 0) const;

    /**
     * Find a free (unused register)
     *
     * @return Register position in the RBank
     */
    gal_uint findFree() const;

    /**
     * gets RBank's size
     *
     * @return RBank's size
     */
    gal_uint size() const;

    /**
     * Tests if a register in the RBank is being used
     *
     * @parameter pos Register position in the RBank
     *
     * @return true if the register is being used, false otherwise
     */
    gal_bool usedPosition(gal_uint pos) const;

    void copyContents(const RBank& rb)
    {
        if (count != rb.count)
            CG_ASSERT("Target bank has not the same size");

        for ( int i = 0; i < rb.count && i < count; i++ )
        {
            bank[i] = rb.bank[i];
            used[i] = rb.used[i];
            types[i] = rb.types[i];
            indexes0[i] = rb.indexes0[i];
            indexes1[i] = rb.indexes1[i];

        }
        //count = rb.count;
    }

    virtual void print(std::ostream& os) const
    {
        os << "Register bank name: " << name.c_str() << std::endl;
        for (gal_uint i = 0; i < count; i++)
        {
            if ( used[i] )
            {
                os << i << ": " << bank[i];
                switch ( types[i] )
                {
                    case QR_UNKNOWN:
                        os << 'U'; break;
                    case QR_CONSTANT:
                        os << 'C'; break;
                    case QR_PARAM_LOCAL:
                        os << " L: " << indexes0[i]; break;
                    case QR_PARAM_ENV:
                        os << " E: " << indexes0[i]; break;
                    case QR_GLSTATE:
                        os << " S: " << indexes0[i] << " row: " << indexes1[i]; break;
                    default:
                        CG_ASSERT("Unexpected QuadReg type");
                }
                os << std::endl;
            }
        }
    }

    RBank<T>& operator=(const RBank<T>& rb);

    friend std::ostream& operator<<(std::ostream& os, const RBank& rb)
    {
        rb.print(os);
        return os;
    }
};

template<class T>
RBank<T>::RBank(const RBank<T>& rb)
{
    name = rb.name;
    count = rb.count;
    bank = new QuadReg<T>[count];
    types = new RType[count];
    used = new gal_bool[count];
    indexes0 = new gal_int[count];
    indexes1 = new gal_int[count];

    for ( U32 i = 0; i < count; i++ )
    {
        bank[i] = rb.bank[i];
        used[i] = rb.used[i];
        types[i] = rb.types[i];
        indexes0[i] = rb.indexes0[i];
        indexes1[i] = rb.indexes1[i];
    }
}


template<class T>
RBank<T>::RBank(gal_uint size, const std::string& name) : name(name)
{
    count = size;
    bank = new QuadReg<T>[count];
    types = new RType[count];
    indexes0 = new gal_int[count];
    indexes1 = new gal_int[count];
    used = new gal_bool[count];
    memset(used,0,sizeof(bool)*count);
    memset(types,QR_UNKNOWN,sizeof(RType)*count);
    memset(indexes0,0,sizeof(int)*count);
    memset(indexes1,0,sizeof(int)*count);

}

template<class T>
RBank<T>::~RBank()
{
    delete[] bank;
    delete[] types;
    delete[] indexes0;
    delete[] indexes1;
    delete[] used;
}

template<class T>
RBank<T>& RBank<T>::operator=(const RBank<T>& rb)
{
    name = rb.name;
    if ( count != 0 )
    {
        delete[] bank;
        delete[] used;
        delete[] types;
        delete[] indexes0;
        delete[] indexes1;
    }

    //  Check number of registers in the source register bank.  
    CG_ASSERT_COND(!(rb.count == 0), "No registers in the source register bank.");
    count = rb.count;
    bank = new QuadReg<T>[count];
    used = new gal_bool[count];
    types = new RType[count];
    indexes0 = new gal_int[count];
    indexes1 = new gal_int[count];

    //  Check allocation.  
    GPU_ASSERT(
        if (bank == NULL)
            CG_ASSERT("Error allocating bank array.");
        if (used == NULL)
            CG_ASSERT("Error allocating used array.");
        if (types == NULL)
            CG_ASSERT("Error allocating types array.");
        if (indexes0 == NULL)
            CG_ASSERT("Error allocating indexes0 array.");
        if (indexes1 == NULL)
            CG_ASSERT("Error allocating indexes1 array.");
    )

    for ( U32 i = 0; i < count; i++ )
    {
        bank[i] = rb.bank[i];
        used[i] = rb.used[i];
        types[i] = rb.types[i];
        indexes0[i] = rb.indexes0[i];
        indexes1[i] = rb.indexes1[i];
    }
    return *this;
}


template<class T>
gal_uint RBank<T>::size() const
{
    return count;
}

template<class T>
void RBank<T>::setSize( gal_uint size_ )
{
    if ( count != 0 )
    {
        delete[] bank;
        delete[] used;
        delete[] types;
        delete[] indexes0;
        delete[] indexes1;
    }

    count = size_;

    bank = new QuadReg<T>[count];
    used = new gal_bool[count];
    types = new RType[count];
    indexes0 = new gal_int[count];
    indexes1 = new gal_int[count];
    memset(used,0,sizeof(bool)*count);
    memset(types,QR_UNKNOWN,sizeof(RType)*count);
    memset(indexes0,0,sizeof(int)*count);
    memset(indexes1,0,sizeof(int)*count);
}

template<class T>
QuadReg<T>& RBank<T>::get(gal_uint pos) const
{
    //  Check index to the register bank.  
    if (pos >= count)
        CG_ASSERT("Invalid position out of bounds");

    return bank[pos];
}

template<class T>
QuadReg<T>& RBank<T>::operator[](gal_uint pos)
{
    //  Check index to the register bank.  
    if (pos >= count)
        CG_ASSERT("Invalid position out of bounds");

    used[pos] = true;
    return bank[pos];
}


template<class T>
void RBank<T>::set(gal_uint pos, const QuadReg<T>& value)
{
    //  Check index to the register bank.  
    if (pos >= count)
        CG_ASSERT("Invalid position out of bounds");

    used[pos] = true;
    bank[pos] = value;
}

template<class T>
void RBank<T>::clear(gal_uint pos)
{
    //  Check index to the register bank.  
    if (pos >= count)
        CG_ASSERT("Invalid position out of bounds");

    used[pos] = false;
    types[pos] = QR_UNKNOWN;
    QuadReg<T> q; // init to zero 
    bank[pos] = q;
}

template<class T>
RType RBank<T>::getType(gal_uint pos,int& index0, int& index1) const
{
    //  Check index to the register bank.  
    if (pos >= count)
        CG_ASSERT("Invalid position out of bounds");

    index0 = indexes0[pos];
    index1 = indexes1[pos];
    return types[pos];
}

template<class T>
void RBank<T>::setType(gal_uint pos, RType type, int index0, int index1)
{
    //  Check index to the register bank.  
    if (pos >= count)
        CG_ASSERT("Invalid position out of bounds");

    types[pos] = type;
    indexes0[pos] = index0;
    indexes1[pos] = index1;
}

template<class T>
gal_uint RBank<T>::getRegister(bool& found, const QuadReg<T>& value, RType regType, int index0, int index1) const
{
    gal_uint i;
    found = false;
    for ( i = 0; i < count; i++ )
    {
        if ( types[i] == regType )
        {
            if ( types[i] == QR_CONSTANT || types[i] == QR_UNKNOWN)
            {
                if ( bank[i] == value )
                {
                    found =  true;
                    return i;
                }
            }
            else if ( (types[i] == QR_PARAM_LOCAL) || (types[i] == QR_PARAM_ENV) )
            {
                if (indexes0[i] == index0)
                {
                   found =  true;
                   return i;
                }
            }
            else if ( types[i] == QR_GLSTATE )
            {
                if ( indexes0[i] == index0 && indexes1[i] == index1 )
                {
                   found = true;
                   return i;
                }
            }
            else
            {
                CG_ASSERT("Unexpected QuadReg type");
                return 0;
            }
        }

    }
    return findFree();
}


template<class T>
bool RBank<T>::usedPosition(gal_uint pos) const
{
    if (pos >= count)
        CG_ASSERT("Invalid position out of bounds");

    return used[pos];
}


template<class T>
gal_uint RBank<T>::findFree() const
{
    U32 i;
    for ( i = 0; i < count; i++ )
        if ( !used[i] )
            break;
    return i;
}



template<class T>
class GPUBank : public RBank<T>
{
private:
    int readPorts;
public:

    GPUBank(gal_uint size, std::string name) : RBank<T>(size,name), readPorts(0) {}

    GPUBank(gal_uint size, gal_uint readPorts = 0, std::string name="") :
        RBank<T>(size,name), readPorts(readPorts)
        {}

    GPUBank(const RBank<T>& rb, gal_uint readPorts = 0, std::string name="") :
        RBank<T>(rb.size(),name), readPorts(readPorts)
        {
            copyContents(rb);
        }

    int getReadPorts() const { return readPorts; }

    void setReadPorts(int nReads) { readPorts = nReads; }

    void print(std::ostream& os) const
    {
        RBank<T>::print(os);
        os << "Read ports: " << readPorts << std::endl;
    }
};


} // namespace libGAL

#endif // RBANK_H

