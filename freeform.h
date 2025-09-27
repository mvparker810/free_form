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
        uint16_t        cap;                                                              \
        uint16_t        alive_count;                                                      \
        uint16_t        free_head;                                                        \
    } PREFIX##__table;                                                                    \
                                                                                          \
    const ff_GeneralHandle PREFIX##_INVALIDHANDLE;\

 


// -- Paramters -- //

typedef ff_GeneralHandle ff_ParamHandle;

typedef struct ff_ParameterDef {
    ff_float v;
} ff_ParameterDef;
typedef struct ff_Parameter {
    ff_ParameterDef def;
} ff_Parameter;

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

//todo can i union some of these
typedef struct ff_Expr {
    ff_OperatorType op_type;
    struct ff_Expr* a;
    struct ff_Expr* b;
    ff_float value; //for constants
    ff_ParamHandle param_H;
} ff_Expr;

// -- Constraints -- //

#define FFCONS_MAXENT 16
#define FFCONS_MAXPAR 16

enum ff_ConstraintType;

typedef ff_GeneralHandle ff_ConstraintHandle;

typedef struct ff_ConstraintDef {
    ff_Expr* eq;
    enum ff_ConstraintType type;
    ff_EntityHandle     ents[FFCONS_MAXENT];
    ff_ParamHandle      pars[FFCONS_MAXPAR];
} ff_ConstraintDef;

typedef struct ff_Constraint {
    ff_ConstraintDef def;

    struct {
        ff_float    err;
        ff_Expr**   dervs;
        ff_float*   dervs_y; 
    } JMR; //Jacobian Matrix Row
} ff_Constraint;


/*
typedef struct Jacobian_Matrix_Row {

    struct ff_constraint* parent_constraint_adr;

    Expression* equation;
    double error;
    Expression** derivatives;

    double* dervs_value;
} Jacobian_Matrix_Row;

*/


#define FF_CONSTRAINTS_CFG      \
    CONS(GENERAL)               \
    CONS(HORIZONTAL)            \

enum {
    #define CONS(n) FF_##n,
    FF_CONSTRAINTS_CFG    
    CONS(COUNT)
    #undef CONS
} ff_ConstraintType;

// -- Sketch -- //

//todo move this respective #def down here
FF_DECLARE_GENTABLE(ff_param,      ff_Parameter);
FF_DECLARE_GENTABLE(ff_entity,     ff_Entity);
FF_DECLARE_GENTABLE(ff_constraint, ff_Constraint);

typedef struct ff_Sketch {
    ff_param__table      params;
    ff_entity__table     entities;
    ff_constraint__table constraints;

    bool link_outdated;

    ff_float* normal_mtr;
    ff_float* itrm_sol;

    ff_float* cached_params; //TODO use these

    ff_Constraint**  tmp_contraints;
    ff_Parameter**   tmp_params;

} ff_Sketch;





#pragma endregion;













FF_API void ffSketch_Init(ff_Sketch* skt, uint16_t p_cap, uint16_t e_cap, uint16_t c_cap);
FF_API void ffSketch_Free(ff_Sketch* skt);

FF_API bool ffSketch_Solve(ff_Sketch* skt, double tolerance, uint32_t max_steps);

FF_API ff_ParameterDef  ff_ParameterDef_DEFAULT();
FF_API ff_EntityDef     ff_EntityDef_DEFAULT(enum ff_EntityType type);
FF_API ff_ConstraintDef ff_ConstraintDef_DEFAULT();

FF_API bool ff_ParameterDef_IsValid     (const ff_ParameterDef def);
FF_API bool ff_EntityDef_IsValid    (const ff_EntityDef def);
FF_API bool ff_ConstraintDef_IsValid(const ff_ConstraintDef def);


FF_API bool ffParam_Equals(ff_ParamHandle a, ff_ParamHandle b);
FF_API bool ffEntity_Equals(ff_EntityHandle a, ff_EntityHandle b);
FF_API bool ffConstraint_Equals(ff_ConstraintHandle a, ff_ConstraintHandle b);

// high level push n pop
FF_API ff_ParamHandle      ffSketch_AddParameter    (ff_Sketch* skt, const ff_ParameterDef  p_def);
FF_API ff_EntityHandle     ffSketch_AddEntity       (ff_Sketch* skt, const ff_EntityDef     e_def);
FF_API ff_ConstraintHandle ffSketch_AddConstraint   (ff_Sketch* skt, const ff_ConstraintDef c_def); 

FF_API bool ffSketch_DeleteParameter(ff_Sketch* skt, ff_ParamHandle h);
FF_API bool ffSketch_DeleteEntity(ff_Sketch* skt, ff_EntityHandle h);
FF_API bool ffSketch_DeleteConstraint(ff_Sketch* skt, ff_ConstraintHandle h);

// high level gettrs n setters //
FF_API ff_Parameter*       ffSketch_GetParameter(ff_Sketch* skt, ff_ParamHandle h);
FF_API const ff_Parameter* ffSketch_GetParameter_Protected(const ff_Sketch* skt, ff_ParamHandle h);

FF_API ff_Entity*          ffSketch_GetEntity(ff_Sketch* skt, ff_EntityHandle h);
FF_API const ff_Entity*    ffSketch_GetEntity_Protected(const ff_Sketch* skt, ff_EntityHandle h);

FF_API ff_Constraint*          ffSketch_GetConstraint(ff_Sketch* skt, ff_ConstraintHandle h);
FF_API const ff_Constraint*    ffSketch_GetConstraint_Protected(const ff_Sketch* skt, ff_ConstraintHandle h);






//todo prefix these with ff_

FF_API void expr_free(ff_Expr* expr);
FF_API ff_Expr* exprInit_op(ff_OperatorType type, ff_Expr* a, ff_Expr* b);
FF_API ff_Expr* exprInit_const(ff_float value);
FF_API ff_Expr* exprInit_param(ff_ParamHandle param_H);

FF_API ff_float expr_evaluate(ff_Expr* expr, const ff_param__table *t);
FF_API ff_Expr* expr_derivative(ff_Expr* expr, ff_ParamHandle indp_param, bool protect_params);
























#undef FF_CONSTRAINTS_CFG

#ifdef __cplusplus
}
#endif //extern 'C'

#endif //FF_FREEFORM_H_
