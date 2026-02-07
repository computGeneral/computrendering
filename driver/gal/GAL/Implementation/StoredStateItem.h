/**************************************************************************
 *
 */

#ifndef STORED_STATE_ITEM_H
    #define STORED_STATE_ITEM_H

namespace libGAL
{

class StoredStateItem
{
public:
    /**
     * @note This pure virtual destructor with a defined body allows
     * deleting StoredStateItem objects from the "StoredStateItem"
     * pointer level, being sure that the subclasses destructors will
     * be called properly from the lowest to the highest level in the
     * hierarchy.
     */
    virtual ~StoredStateItem() = 0;
};


} // namespace libGAL

#endif // STORED_STATE_ITEM_H
