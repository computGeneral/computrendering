/**************************************************************************
 *
 */

#ifndef GALx_RBANK_H
    #define GALx_RBANK_H

#include "GALTypes.h"
#include "GALxQuadReg.h"
#include <iostream>
#include <string>
#include <cstring>
#include "support.h"

namespace libGAL
{

enum GALxRType
{
    QR_UNKNOWN,
    QR_CONSTANT,
    QR_GLSTATE,
    QR_PARAM_LOCAL,
    QR_PARAM_ENV
};

/**
 * class implementing a generic GALxRBank Register of four values per register
 *
 * @note Implemented using templates
 *
 * @author Carlos González Rodríguez - cgonzale@ac.upc.es
 * @date 5/7/2004
 * @ver 1.0
 *
 */
template<class T>
class GALxRBank
{
private:

    static GALxQuadReg<T> dummy;

    std::string name; ///< GALxRBank name
    GALxQuadReg<T>* bank; ///< GALxRBank registers
    GALxRType* types; ///< GALxRBank registers type
    gal_int* indexes0; ///< GALxRBank register indexes 0
    gal_int* indexes1; ///< GALxRBank register indexes 1
    gal_bool* used; ///< bitmap of registers used
    gal_uint count; ///< number of registers within the GALxRBank


public:
/*
    GALxRBank() : count(0)
    {
        // default
        bank = NULL;
        types = NULL;
        used = NULL;
        indexes0 = NULL;
        indexes1 = NULL;
        //CG_ASSERT("Empty GALxRBank created");
        warning("GALxRBank","GALxRBank constructor","Empty GALxRBank created");
    }
*/
    /**
     * Creates a GALxRBank of a given size
     *
     * @param size Number of registers
     */
    GALxRBank(gal_uint size, const std::string& name="");

    GALxRBank(const GALxRBank<T>& rb);

    void setSize( gal_uint size );

    /**
     *
     *  GALxRBank destructor.
     *
     */

    ~GALxRBank();

    /**
     * Gets the contents of a given register in the GALxRBank
     *
     * @param pos Register position in the GALxRBank
     *
     * @return register in position pos
     */
    GALxQuadReg<T>& get(gal_uint pos) const;

    /**
     * Gets the contents of a given register in the GALxRBank
     *
     * @param pos Register position in the GALxRBank
     *
     * @return register in position pos
     *
     * @note exact behaviour than GALxRBank<T>::get()
     */
    GALxQuadReg<T>& operator[](gal_uint pos);

    /**
     * Sets a register in the GALxRBank with new value
     *
     * The register is also marked as used
     *
     * @param pos Register position in the GALxRBank
     * @param value new contents
     */
    void set(gal_uint pos, const GALxQuadReg<T>& value);

    /**
     * Mark this register as unused
     *
     * @param pos Register position in the GALxRBank

     * @note Contents are reset to zero
     */
    void clear(gal_uint pos);

    /**
     * Get the type related with the register (QR_CONSTANT,    QR_GLSTATE,    QR_PARAM_LOCAL,    QR_PARAM_ENV) and
     * related indexes.
     *
     * @param pos Register position in the GALxRBank
     * @param index0 In case of QR_PARAM_LOCAL or QR_PARAM_ENV is the index of parameter. In
     * case of QR_GLSTATE is the state identifier. Has no sense otherwise.
     * @param index1 In case of QR_GLSTATE is the row of the matrix or 0
     * @return type of register
     * @note Registers with uninitialized type returns QR_UNKNOWN. The register position has to be correct.
     */
    GALxRType getType(gal_uint pos, gal_int& index0, gal_int& index1) const;

    /**
     * Set the type related with the register and type parameters (indexes)
     *
     * @param pos Register position in the GALxRBank
     * @param type Type of register.
     * @param index0 In case of QR_PARAM_LOCAL or QR_PARAM_ENV is the index of parameter. In
     * case of QR_GLSTATE is the state identifier. Has no sense otherwise.
     * @param index1 In case of QR_GLSTATE is the row of the matrix or 0
     * @note The register position has to be correct.
     */
    void setType(gal_uint pos, GALxRType type = QR_UNKNOWN, gal_int index0 = 0, gal_int index1 = 0);

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
    gal_uint getRegister(gal_bool& found, const GALxQuadReg<T>& value, GALxRType regType = QR_UNKNOWN, 
                         gal_int index0 = 0, gal_int index1 = 0) const;

    /**
     * Find a free (unused register)
     *
     * @return Register position in the GALxRBank
     */
    gal_uint findFree() const;

    /**
     * gets GALxRBank's size
     *
     * @return GALxRBank's size
     */
    gal_uint size() const;

    /**
     * Tests if a register in the GALxRBank is being used
     *
     * @parameter pos Register position in the GALxRBank
     *
     * @return true if the register is being used, false otherwise
     */
    gal_bool usedPosition(gal_uint pos) const;

    void copyContents(const GALxRBank& rb)
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
                        CG_ASSERT("Unexpected GALxQuadReg type");
                }
                os << std::endl;
            }
        }
    }

    GALxRBank<T>& operator=(const GALxRBank<T>& rb);

    friend std::ostream& operator<<(std::ostream& os, const GALxRBank& rb)
    {
        rb.print(os);
        return os;
    }
};

template<class T>
GALxRBank<T>::GALxRBank(const GALxRBank<T>& rb)
{
    name = rb.name;
    count = rb.count;
    bank = new GALxQuadReg<T>[count];
    types = new GALxRType[count];
    used = new gal_bool[count];
    indexes0 = new gal_int[count];
    indexes1 = new gal_int[count];

    for ( gal_uint i = 0; i < count; i++ )
    {
        bank[i] = rb.bank[i];
        used[i] = rb.used[i];
        types[i] = rb.types[i];
        indexes0[i] = rb.indexes0[i];
        indexes1[i] = rb.indexes1[i];
    }
}


template<class T>
GALxRBank<T>::GALxRBank(gal_uint size, const std::string& name) : name(name)
{
    count = size;
    bank = new GALxQuadReg<T>[count];
    types = new GALxRType[count];
    indexes0 = new gal_int[count];
    indexes1 = new gal_int[count];
    used = new gal_bool[count];
    memset(used,0,sizeof(bool)*count);
    memset(types,QR_UNKNOWN,sizeof(GALxRType)*count);
    memset(indexes0,0,sizeof(int)*count);
    memset(indexes1,0,sizeof(int)*count);

}

template<class T>
GALxRBank<T>::~GALxRBank()
{
    delete[] bank;
    delete[] types;
    delete[] indexes0;
    delete[] indexes1;
    delete[] used;
}

template<class T>
GALxRBank<T>& GALxRBank<T>::operator=(const GALxRBank<T>& rb)
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
    bank = new GALxQuadReg<T>[count];
    used = new gal_bool[count];
    types = new GALxRType[count];
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

    for ( gal_uint i = 0; i < count; i++ )
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
gal_uint GALxRBank<T>::size() const
{
    return count;
}

template<class T>
void GALxRBank<T>::setSize( gal_uint size_ )
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

    bank = new GALxQuadReg<T>[count];
    used = new gal_bool[count];
    types = new GALxRType[count];
    indexes0 = new gal_int[count];
    indexes1 = new gal_int[count];
    memset(used,0,sizeof(bool)*count);
    memset(types,QR_UNKNOWN,sizeof(GALxRType)*count);
    memset(indexes0,0,sizeof(int)*count);
    memset(indexes1,0,sizeof(int)*count);
}

template<class T>
GALxQuadReg<T>& GALxRBank<T>::get(gal_uint pos) const
{
    //  Check index to the register bank.  
    if (pos >= count)
        CG_ASSERT("Invalid position out of bounds");

    return bank[pos];
}

template<class T>
GALxQuadReg<T>& GALxRBank<T>::operator[](gal_uint pos)
{
    //  Check index to the register bank.  
    if (pos >= count)
        CG_ASSERT("Invalid position out of bounds");

    used[pos] = true;
    return bank[pos];
}


template<class T>
void GALxRBank<T>::set(gal_uint pos, const GALxQuadReg<T>& value)
{
    //  Check index to the register bank.  
    if (pos >= count)
        CG_ASSERT("Invalid position out of bounds");

    used[pos] = true;
    bank[pos] = value;
}

template<class T>
void GALxRBank<T>::clear(gal_uint pos)
{
    //  Check index to the register bank.  
    if (pos >= count)
        CG_ASSERT("Invalid position out of bounds");

    used[pos] = false;
    types[pos] = QR_UNKNOWN;
    GALxQuadReg<T> q; // init to zero 
    bank[pos] = q;
}

template<class T>
GALxRType GALxRBank<T>::getType(gal_uint pos,int& index0, int& index1) const
{
    //  Check index to the register bank.  
    if (pos >= count)
        CG_ASSERT("Invalid position out of bounds");

    index0 = indexes0[pos];
    index1 = indexes1[pos];
    return types[pos];
}

template<class T>
void GALxRBank<T>::setType(gal_uint pos, GALxRType type, int index0, int index1)
{
    //  Check index to the register bank.  
    if (pos >= count)
        CG_ASSERT("Invalid position out of bounds");

    types[pos] = type;
    indexes0[pos] = index0;
    indexes1[pos] = index1;
}

template<class T>
gal_uint GALxRBank<T>::getRegister(bool& found, const GALxQuadReg<T>& value, GALxRType regType, int index0, int index1) const
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
                CG_ASSERT("Unexpected GALxQuadReg type");
                return 0;
            }
        }

    }
    return findFree();
}


template<class T>
bool GALxRBank<T>::usedPosition(gal_uint pos) const
{
    if (pos >= count)
        CG_ASSERT("Invalid position out of bounds");

    return used[pos];
}


template<class T>
gal_uint GALxRBank<T>::findFree() const
{
    gal_uint i;
    for ( i = 0; i < count; i++ )
        if ( !used[i] )
            break;
    return i;
}



template<class T>
class GPUBank : public GALxRBank<T>
{
private:
    int readPorts;
public:

    GPUBank(gal_uint size, std::string name) : GALxRBank<T>(size,name), readPorts(0) {}

    GPUBank(gal_uint size, gal_uint readPorts = 0, std::string name="") :
        GALxRBank<T>(size,name), readPorts(readPorts)
        {}

    GPUBank(const GALxRBank<T>& rb, gal_uint readPorts = 0, std::string name="") :
        GALxRBank<T>(rb.size(),name), readPorts(readPorts)
        {
            copyContents(rb);
        }

    int getReadPorts() const { return readPorts; }

    void setReadPorts(int nReads) { readPorts = nReads; }

    void print(std::ostream& os) const
    {
        GALxRBank<T>::print(os);
        os << "Read ports: " << readPorts << std::endl;
    }
};


} // namespace libGAL

#endif // GALx_RBANK_H

