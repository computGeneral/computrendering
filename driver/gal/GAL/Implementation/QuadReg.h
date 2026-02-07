/**************************************************************************
 *
 */

#ifndef QUADREG_H
    #define QUADREG_H    

#include <iostream>


template<class T>
class QuadReg
{

private:
    
    T values[4];
    

public:

    QuadReg()
    {
        values[0] = (T)0;
        values[1] = (T)0;
        values[2] = (T)0;
        values[3] = (T)0;
    }

    QuadReg(const T& t1, const T& t2, const T& t3, const T& t4) // : type(QR_UNKNOWN)
    {
        values[0] = t1;
        values[1] = t2;
        values[2] = t3;
        values[3] = t4;
    }

    bool operator==(const QuadReg<T>& q)
    {
        return (values[0] == q.values[0] && values[1] == q.values[1] && 
                values[2] == q.values[2] && values[3] == q.values[3]);
    }

    T& operator[](int index)
    {        
        return values[index];
    }
    
    const T& operator[](int index) const
    {
        return values[index];
    }

    T& x() { return values[0]; }
    T& y() { return values[1]; }
    T& z() { return values[2]; }
    T& w() { return values[3]; }
    
    const T& x() const { return values[0]; }
    const T& y() const { return values[1]; }
    const T& z() const { return values[2]; }
    const T& w() const { return values[3]; }

    virtual void print(std::ostream& os) const
    {
        os << "(" << values[0] << "," << values[1] << "," 
           << values[2] << "," << values[3] << ")";
    }

    // Do not work...
    friend std::ostream& operator<<(std::ostream& os, const QuadReg& q)
    {
        q.print(os);
        return os;
    }


};

#endif
