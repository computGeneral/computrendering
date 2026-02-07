/**************************************************************************
 *
 */

#ifndef STATE_ITEM
    #define STATE_ITEM

#include "GALTypes.h"

namespace libGAL
{

template<class T>
class StateItem
{
public:

    inline StateItem();

    inline StateItem(T initialValue);

    inline void restart();

    inline gal_bool changed() const;

    inline operator const T&() const;

    inline StateItem<T>& operator=(const T& value);

    inline const T& initial() const;

private:

    T _initialValue;
    T _value;

};

} // namespace galib

template<class T>
libGAL::StateItem<T>::StateItem() {}

template<class T>
libGAL::StateItem<T>::StateItem(T initialValue) : _initialValue(initialValue), _value(_initialValue) {}

template<class T>
void libGAL::StateItem<T>::restart() { _initialValue = _value; }

template<class T>
libGAL::gal_bool libGAL::StateItem<T>::changed()  const { return (!(_initialValue == _value)); }

template<class T>
libGAL::StateItem<T>::operator const T&() const { return _value; }

template<class T>
libGAL::StateItem<T> & libGAL::StateItem<T>::operator=(const T& value) { _value = value; return *this; }

template<class T>
const T& libGAL::StateItem<T>::initial() const { return _initialValue; }

#endif // STATE_ITEM
