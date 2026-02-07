/**************************************************************************
 *
 */

#ifndef GPUMEMORY_H
    #define GPUMEMORY_H

#include "GPUType.h"
#include "BaseObject.h"
#include <map>
#include <vector>

class HAL; // HAL is defined (for now) in the global scope

namespace libgl
{

/**
 * GPU Card memory abstraction for copy and synchronize OpenGL BaseObjects into GPU local memory
 */
class GPUMemory
{        
private:

    struct BaseObjectInfo
    {
        std::vector<GLuint> size;
        std::vector<U32> md;
    };
    
    std::map<BaseObject*, BaseObjectInfo> maps;

    HAL* driver;
        
    void _update(BaseObject* bo, BaseObjectInfo* boi);
    void _dealloc(BaseObject* bo, BaseObjectInfo* boi);
    void _alloc(BaseObject* bo);
        
    BaseObjectInfo* _find(BaseObject* bo);
        
public:
    
    GPUMemory(HAL* driver);
    
    bool allocate(BaseObject& bo);
    
    bool deallocate(BaseObject& bo);
    
    bool isPresent(BaseObject& bo);
            
    U32 md(BaseObject& bo, GLuint portion = 0);
    
    void dump() const;
};

} // namespace libgl

#endif // GPUMEMORY_H
