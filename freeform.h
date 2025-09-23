//FF_FREEFORM_H_
//FF_FREEFORM_IMPL_
//FF_API

/* freeform.h — single-header library
   Usage:
     // In exactly one .c/.cpp file:
     #define FF_FREEFORM_IMPLEMENTATION
     #include "freeform.h"

     // In all other translation units:
     #include "freeform.h"
*/
#ifndef FF_FREEFORM_H_
#define FF_FREEFORM_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>  

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FF_FREEFORM_IMPL_
#define FF_API
#else
#define FF_API extern
#endif

#pragma region Types;

// -=-=-=-=- Prim Types -=-=-=-=- //

typedef double ff_float;
typedef struct ff_vec2 {
    ff_float x;
    ff_float y;
} ff_vec2;


#define FF_INVALID_INDEX 0xFFFF
typedef struct ff_GeneralHandle {
    uint16_t idx;
    uint32_t gen;
} ff_GeneralHandle;

/* ===== Generic gen+idx table: declarations ===== */
#define FF_DECLARE_GENTABLE(PREFIX, PAYLOAD_T)                                            \
    typedef struct PREFIX##__slot {                                                       \
        uint32_t gen;                                                                     \
        uint16_t next_free;                                                               \
        bool     alive;                                                                   \
        PAYLOAD_T payload;                                                                \
    } PREFIX##__slot;                                                                     \
                                                                                          \
    typedef struct PREFIX##__table {                                                      \
        PREFIX##__slot* slots;                                                            \
        uint16_t        cap;            /* total slots allocated (<= 65535) */            \
        uint16_t        alive_count;                                                      \
        uint16_t        free_head;     /* index of first free slot, or FF_INVALID_INDEX*/ \
    } PREFIX##__table;                                                                     \
                                                                                          \
    FF_API void PREFIX##_init(PREFIX##__table* t, uint16_t initial_cap);                  \
    FF_API void PREFIX##_free(PREFIX##__table* t);                                        \
    FF_API ff_GeneralHandle PREFIX##_create(PREFIX##__table* t, const PAYLOAD_T* init);   \
    FF_API bool PREFIX##_destroy(PREFIX##__table* t, ff_GeneralHandle h);                 \
    FF_API bool PREFIX##_alive(const PREFIX##__table* t, ff_GeneralHandle h);             \
    FF_API PAYLOAD_T* PREFIX##_get(PREFIX##__table* t, ff_GeneralHandle h);               \
    FF_API const PAYLOAD_T* PREFIX##_get_const(const PREFIX##__table* t, ff_GeneralHandle h);\
    FF_API const ff_GeneralHandle PREFIX##_INVALIDHANDLE;\

 

// -- Solving -- //

typedef enum {
    OperatorType_CONST,
    OperatorType_PARAM,
    OperatorType_EXTR_PARAM,
    OperatorType_ADD,
    OperatorType_SUB,
    OperatorType_MUL,
    OperatorType_DIV,
    OperatorType_SIN,
    OperatorType_COS,
    OperatorType_ASIN,
    OperatorType_ACOS,
    OperatorType_SQRT,
    OperatorType_SQR
} ff_OperatorType;

typedef struct ff_Expr {
    ff_OperatorType op_type;
    struct ff_Expr* expr_a;
    struct ff_Expr* expr_b;
    ff_float val; //for constants
    int param_valie; //idk??
} ff_Expr;




// -- Paramters -- //

typedef ff_GeneralHandle ff_ParamHandle;
typedef ff_float ff_Parameter;

// -- Entitites -- //

enum ff_EntityType {
    FF_POINT,
    FF_LINE,
    FF_CIRCLE,
    FF_ARC
};

typedef ff_GeneralHandle ff_EntityHandle;

typedef struct ff_EntityDef {
    enum ff_EntityType type;
    union {
        struct {
            ff_ParamHandle      x;
            ff_ParamHandle      y;
        } point;
        struct {
            ff_EntityHandle     p1;
            ff_EntityHandle     p2;
        } line;
        struct {
            ff_EntityHandle     c;
            ff_ParamHandle      r;
        } circle;
        struct {
            ff_EntityHandle     p1;
            ff_EntityHandle     p2;
            ff_EntityHandle     p3;
        } arc;
    } data;
} ff_EntityDef;

typedef struct ff_Entity {
    ff_EntityDef def;
} ff_Entity;

// -- Constraints -- //

#define FFCONS_MAXENT 16
#define FFCONS_MAXPAR 16

enum ff_ConstraintType;

typedef ff_GeneralHandle ff_ConstraintHandle;
typedef struct ff_Constraint {
    enum ff_ConstraintType type;

    ff_EntityHandle     ents[FFCONS_MAXENT];
    ff_ParamHandle      pars[FFCONS_MAXPAR];
} ff_Constraint;



#define FF_CONSTRAINTS_CFG      \
    CONS(HORIZONTAL)            \

enum {
    #define CONS(n) FF_##n,
    FF_CONSTRAINTS_CFG    
    CONS(COUNT)
    #undef CONS
} ff_ConstraintType;

// -- Sketch -- //

FF_DECLARE_GENTABLE(ff_param,      ff_Parameter);
FF_DECLARE_GENTABLE(ff_entity,     ff_Entity);
FF_DECLARE_GENTABLE(ff_constraint, ff_Constraint);

typedef struct ff_Sketch {
    ff_param__table      params;
    ff_entity__table     entities;
    ff_constraint__table constraints;
} ff_Sketch;





#pragma endregion;













FF_API void ff_SketchInit(ff_Sketch* sk, uint16_t p_cap, uint16_t e_cap, uint16_t c_cap);
FF_API void ff_SketchFree(ff_Sketch* sk);


FF_API ff_EntityDef ff_EntityDef_DEFAULT(enum ff_EntityType type);
FF_API bool ff_EntityDef_IsValid(ff_EntityDef def);

/* Example high-level helpers (nice to have) */
FF_API ff_ParamHandle      ff_CreateParam(ff_Sketch* sk, ff_Parameter value);
FF_API ff_EntityHandle     ff_CreateEntity(ff_Sketch* sk, const ff_EntityDef e_def);
FF_API ff_ConstraintHandle ff_CreateConstraint(ff_Sketch* sk, const ff_Constraint* c);

FF_API bool ff_DestroyParam(ff_Sketch* sk, ff_ParamHandle h);
FF_API bool ff_DestroyEntity(ff_Sketch* sk, ff_EntityHandle h);
FF_API bool ff_DestroyConstraint(ff_Sketch* sk, ff_ConstraintHandle h);

// high level gettrs n setters //
FF_API ff_Parameter*       ff_GetParam(ff_Sketch* sk, ff_ParamHandle h);
FF_API const ff_Parameter* ff_GetParamConst(const ff_Sketch* sk, ff_ParamHandle h);

FF_API ff_Entity*          ff_GetEntity(ff_Sketch* sk, ff_EntityHandle h);
FF_API const ff_Entity*    ff_GetEntityConst(const ff_Sketch* sk, ff_EntityHandle h);

FF_API ff_Constraint*          ff_GetConstraint(ff_Sketch* sk, ff_ConstraintHandle h);
FF_API const ff_Constraint*    ff_GetConstraintConst(const ff_Sketch* sk, ff_ConstraintHandle h);



#ifdef FF_FREEFORM_IMPL_




static int ff_ERROR(const char* msg) {
    exit(1);
}


/* ===== Generic gen+idx table: definitions ===== */
#define FF__CLAMP_CAP_16(x) ((uint16_t)((x) > 0xFFFF ? 0xFFFF : (x)))

#define FF_DEFINE_GENTABLE(PREFIX, PAYLOAD_T)                                             \
    static void PREFIX##__grow(PREFIX##__table* t, uint16_t add) {                        \
        if (add == 0) return;                                                             \
        uint32_t new_cap32 = (uint32_t)t->cap + (uint32_t)add;                            \
        uint16_t new_cap   = FF__CLAMP_CAP_16(new_cap32);                                 \
        if (new_cap <= t->cap) return; /* hit 65535 */                                    \
        PREFIX##__slot* new_slots = (PREFIX##__slot*)realloc(                             \
            t->slots, sizeof(PREFIX##__slot) * new_cap);                                  \
        if (!new_slots) return; /* OOM: leave table unchanged */                          \
        /* initialize newly added range and link them into free list */                   \
        for (uint16_t i = t->cap; i < new_cap; ++i) {                                     \
            new_slots[i].gen       = 1;                                                   \
            new_slots[i].alive     = false;                                               \
            new_slots[i].next_free = (uint16_t)(i + 1);                                   \
            memset(&new_slots[i].payload, 0, sizeof(PAYLOAD_T));                          \
        }                                                                                 \
        new_slots[new_cap - 1].next_free = t->free_head;                                  \
        t->free_head = t->cap;                                                            \
        t->slots    = new_slots;                                                          \
        t->cap      = new_cap;                                                            \
    }                                                                                     \
                                                                                          \
    void PREFIX##_init(PREFIX##__table* t, uint16_t initial_cap) {                        \
        t->slots       = NULL;                                                            \
        t->cap         = 0;                                                               \
        t->alive_count = 0;                                                               \
        t->free_head   = FF_INVALID_INDEX;                                                \
        if (initial_cap) PREFIX##__grow(t, initial_cap);                                  \
    }                                                                                     \
                                                                                          \
    void PREFIX##_free(PREFIX##__table* t) {                                              \
        free(t->slots);                                                                   \
        t->slots = NULL;                                                                  \
        t->cap = t->alive_count = 0;                                                      \
        t->free_head = FF_INVALID_INDEX;                                                  \
    }                                                                                     \
                                                                                          \
    ff_GeneralHandle PREFIX##_create(PREFIX##__table* t, const PAYLOAD_T* init) {         \
        if (t->free_head == FF_INVALID_INDEX) {                                           \
            /* grow (geometric) */                                                        \
            uint16_t add = (t->cap < 64) ? 64 : (t->cap >> 1);                            \
            if (add == 0) add = 64;                                                       \
            PREFIX##__grow(t, add);                                                       \
            if (t->free_head == FF_INVALID_INDEX) {                                       \
                return (ff_GeneralHandle){ FF_INVALID_INDEX, 0 };                         \
            }                                                                             \
        }                                                                                 \
        uint16_t idx = t->free_head;                                                      \
        PREFIX##__slot* s = &t->slots[idx];                                               \
        t->free_head = s->next_free;                                                      \
        s->alive = true;                                                                  \
        if (init) memcpy(&s->payload, init, sizeof(PAYLOAD_T));                           \
        else      memset(&s->payload, 0,    sizeof(PAYLOAD_T));                           \
        t->alive_count++;                                                                 \
        return (ff_GeneralHandle){ idx, s->gen };                                         \
    }                                                                                     \
                                                                                          \
    static inline bool PREFIX##__valid(const PREFIX##__table* t, ff_GeneralHandle h) {    \
        return (h.idx != FF_INVALID_INDEX) && (h.idx < t->cap);                           \
    }                                                                                     \
                                                                                          \
    bool PREFIX##_alive(const PREFIX##__table* t, ff_GeneralHandle h) {                   \
        if (!PREFIX##__valid(t, h)) return false;                                         \
        const PREFIX##__slot* s = &t->slots[h.idx];                                       \
        return s->alive && (s->gen == h.gen);                                             \
    }                                                                                     \
                                                                                          \
    PAYLOAD_T* PREFIX##_get(PREFIX##__table* t, ff_GeneralHandle h) {                     \
        if (!PREFIX##_alive(t, h)) return NULL;                                           \
        return &t->slots[h.idx].payload;                                                  \
    }                                                                                     \
                                                                                          \
    const PAYLOAD_T* PREFIX##_get_const(const PREFIX##__table* t, ff_GeneralHandle h) {   \
        if (!PREFIX##_alive(t, h)) return NULL;                                           \
        return &t->slots[h.idx].payload;                                                  \
    }                                                                                     \
                                                                                          \
    bool PREFIX##_destroy(PREFIX##__table* t, ff_GeneralHandle h) {                       \
        if (!PREFIX##_alive(t, h)) return false;                                          \
        PREFIX##__slot* s = &t->slots[h.idx];                                             \
        s->alive = false;                                                                 \
        s->gen  += 1u; /* bump generation to invalidate stale handles */                  \
        s->next_free = t->free_head;                                                      \
        t->free_head = h.idx;                                                             \
        if (t->alive_count) t->alive_count--;                                             \
        return true;                                                                      \
    }\
    \
    const ff_GeneralHandle PREFIX##_INVALIDHANDLE = { FF_INVALID_INDEX, 0u };\


FF_DEFINE_GENTABLE(ff_param,      ff_Parameter)
FF_DEFINE_GENTABLE(ff_entity,     ff_Entity)
FF_DEFINE_GENTABLE(ff_constraint, ff_Constraint)

/* ===== Sketch helpers ===== */
void ff_SketchInit(ff_Sketch* sk, uint16_t p_cap, uint16_t e_cap, uint16_t c_cap) {
    ff_param_init(&sk->params,      p_cap);
    ff_entity_init(&sk->entities,   e_cap);
    ff_constraint_init(&sk->constraints, c_cap);
}
void ff_SketchFree(ff_Sketch* sk) {
    ff_param_free(&sk->params);
    ff_entity_free(&sk->entities);
    ff_constraint_free(&sk->constraints);
}




ff_EntityDef ff_EntityDef_DEFAULT(enum ff_EntityType type) {
    ff_EntityDef def;

    def.type = type;

    def.data.point.x     = ff_param_INVALIDHANDLE;
    def.data.point.y     = ff_param_INVALIDHANDLE;

    def.data.line.p1     = ff_entity_INVALIDHANDLE;
    def.data.line.p2     = ff_entity_INVALIDHANDLE;

    def.data.circle.c    = ff_entity_INVALIDHANDLE;
    def.data.circle.r    = ff_param_INVALIDHANDLE;

    def.data.arc.p1      = ff_entity_INVALIDHANDLE;
    def.data.arc.p2      = ff_entity_INVALIDHANDLE;
    def.data.arc.p3      = ff_entity_INVALIDHANDLE;

    return def;
}

bool ff_EntityDef_IsValid (ff_EntityDef def) {
    const ff_Parameter*     param;
    const ff_Entity*        ent;

    switch (def.type) {
        case (FF_POINT):
            if (0) { ff_ERROR("error\n"); return false; }
        break;
        case (FF_LINE):

        break;
        case (FF_CIRCLE):

        break;
        case (FF_ARC):

        break;
        default:
        ff_ERROR("Invalid Type for Entity Def");
        break;
    }

    return true;
}

ff_ParamHandle ff_CreateParam(ff_Sketch* sk, ff_Parameter value) {
    return ff_param_create(&sk->params, &value);
}
ff_EntityHandle ff_CreateEntity(ff_Sketch* sk, const ff_EntityDef e_def) {
    if (!ff_EntityDef_IsValid(e_def)) return ff_entity_INVALIDHANDLE;
    ff_Entity ent = (ff_Entity) { .def = e_def };
    return ff_entity_create(&sk->entities, &ent);
}
ff_ConstraintHandle ff_CreateConstraint(ff_Sketch* sk, const ff_Constraint* c) {
    return ff_constraint_create(&sk->constraints, c);
}

bool ff_DestroyParam(ff_Sketch* sk, ff_ParamHandle h) {
    return ff_param_destroy(&sk->params, h);
}
bool ff_DestroyEntity(ff_Sketch* sk, ff_EntityHandle h) {
    return ff_entity_destroy(&sk->entities, h);
}
bool ff_DestroyConstraint(ff_Sketch* sk, ff_ConstraintHandle h) {
    return ff_constraint_destroy(&sk->constraints, h);
}





/* === High-level getters/setters (definitions) === */
ff_Parameter* ff_GetParam(ff_Sketch* sk, ff_ParamHandle h) {
    return ff_param_get(&sk->params, h);
}
const ff_Parameter* ff_GetParamConst(const ff_Sketch* sk, ff_ParamHandle h) {
    return ff_param_get_const(&sk->params, h);
}


ff_Entity* ff_GetEntity(ff_Sketch* sk, ff_EntityHandle h) {
    return ff_entity_get(&sk->entities, h);
}
const ff_Entity* ff_GetEntityConst(const ff_Sketch* sk, ff_EntityHandle h) {
    return ff_entity_get_const(&sk->entities, h);
}


ff_Constraint* ff_GetConstraint(ff_Sketch* sk, ff_ConstraintHandle h) {
    return ff_constraint_get(&sk->constraints, h);
}
const ff_Constraint* ff_GetConstraintConst(const ff_Sketch* sk, ff_ConstraintHandle h) {
    return ff_constraint_get_const(&sk->constraints, h);
}




#endif //FF_FREEFORM_IMPL_








#undef FF_CONSTRAINTS_CFG






#ifdef __cplusplus
}
#endif //extern 'C'

#endif //FF_FREEFORM_H_
