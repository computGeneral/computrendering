#ifndef _SUPPORTMACRO_H__ 
#define _SUPPORTMACRO_H__ 

#define CG_INFOEN            1 
#define CG_WARNEN            1
#define CG_DEBGEN            1


#define CG_INFO(Msg, ...) \
        do \
        { \
            if(CG_INFOEN) \
            { \
                ModelPrint("INFO", " ", __FILE__, __func__, __LINE__, Msg, ##__VA_ARGS__); \
            } \
        } \
        while (0)

#define CG_INFO_COND(Exp, Msg, ...) \
        do \
        { \
            if(CG_INFOEN) \
            { \
                if(Exp) \
                { \
                    ModelPrint("INFO", " ", __FILE__, __func__, __LINE__, Msg, ##__VA_ARGS__); \
                } \
            } \
        } \
        while(0)

#define CG_WARN(Msg, ...) \
        do \
        { \
            if(CG_WARNEN) \
            { \
                ModelPrint("WARN", " ", __FILE__, __func__, __LINE__, Msg, ##__VA_ARGS__); \
            } \
        } \
        while (0)

#define CG_WARN_COND(Exp, Msg, ...) \
        do \
        { \
            if(CG_WARNEN) \
            { \
                if(Exp) \
                { \
                    ModelPrint("WARN", " ", __FILE__, __func__, __LINE__, Msg, ##__VA_ARGS__); \
                } \
            } \
        } \
        while(0)

#define CG_ABORT(Msg, ...) \
        do \
        { \
            ModelPrint("ERRO", " ", __FILE__, __func__, __LINE__, Msg, ##__VA_ARGS__); \
            throw false; \
        } \
        while (0)

#define CG_ABORT_COND(Exp, Msg, ...) \
        do \
        { \
            if (Exp) \
            { \
                ModelPrint("ERRO", " ", __FILE__, __func__, __LINE__, Msg, ##__VA_ARGS__); \
                throw false; \
            } \
        } \
        while (0)

#define CG_ASSERT(Msg, ...) \
        do \
        { \
            ModelPrint("ASRT", " ", __FILE__, __func__, __LINE__, Msg, ##__VA_ARGS__); \
            throw false; \
        } \
        while (0)
        //    throw false; \

#define CG_ASSERT_COND(Exp, Msg, ...) \
        do \
        { \
            if (!(Exp)) \
            { \
                ModelPrint("ASRT", " ", __FILE__, __func__, __LINE__, Msg, ##__VA_ARGS__); \
                throw false; \
            } \
        } \
        while (0)

#define CG_DEBUG(Msg, ...) \
        do \
        { \
            if(CG_DEBGEN && debugMode) \
            { \
                ModelPrint("DEBG", " ", __FILE__, __func__, __LINE__, Msg, ##__VA_ARGS__); \
            } \
        } \
        while (0)
//#endif /*CG_ARCH_MDL*/
#endif /*_SUPPORTMACRO_H__ */
