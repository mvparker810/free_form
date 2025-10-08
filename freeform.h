/**
 * @file freeform.h
 * @brief Minimalist parametric 2D geometric constraint solver library
 *
 * freeform is a single-header library for solving constrained parametric 2D geometric systems.
 * Supports points, lines, circles, and arcs with extensible constraint types.
 *
 * @author [Author]
 * @date [Date]
 * @version 1.0
 *
 * Usage:
 * @code
 *   // In exactly one .c/.cpp file:
 *   #define FF_FREEFORM_IMPLEMENTATION
 *   #include "freeform.h"
 *
 *   // In all other translation units:
 *   #include "freeform.h"
 * @endcode
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

/** @defgroup PrimitiveTypes Primitive Types
 *  @{
 */

/** @brief Floating point type used throughout the library */
typedef double ff_float;

/** @brief 2D vector/point */
typedef struct ff_vec2 {
    ff_float x; /**< X coordinate */
    ff_float y; /**< Y coordinate */
} ff_vec2;

/** @brief Invalid index sentinel value */
#define FF_INVALID_INDEX 0xFFFF

/**
 * @brief Generational handle for safe referencing of array elements
 *
 * Uses generation counters to detect stale references after deletions.
 */
typedef struct ff_GeneralHandle {
    uint16_t idx; /**< Index into array */
    uint32_t gen; /**< Generation counter */
} ff_GeneralHandle;

/** @} */

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

 


/** @defgroup Parameters Parameters
 *  @brief Numeric values modified by the constraint solver
 *  @{
 */

/** @brief Handle to a parameter */
typedef ff_GeneralHandle ff_ParamHandle;

/**
 * @brief Parameter definition
 */
typedef struct ff_ParameterDef {
    ff_float v; /**< Parameter value */
} ff_ParameterDef;

/**
 * @brief Parameter storage
 */
typedef struct ff_Parameter {
    ff_ParameterDef def; /**< Parameter definition */
} ff_Parameter;

/** @} */

/** @defgroup Entities Geometric Entities
 *  @brief 2D geometric primitives (points, lines, circles, arcs)
 *  @{
 */

/**
 * @brief Type of geometric entity
 */
enum ff_EntityType {
    FF_POINT,  /**< 2D point */
    FF_LINE,   /**< Line segment between two points */
    FF_CIRCLE, /**< Circle with center and radius */
    FF_ARC     /**< Circular arc through three points */
};

/** @brief Handle to an entity */
typedef ff_GeneralHandle ff_EntityHandle;

/**
 * @brief Entity definition
 *
 * Defines a geometric entity. The type field determines which union member is valid.
 */
typedef struct ff_EntityDef {
    enum ff_EntityType type; /**< Entity type */
    union {
        struct {
            ff_ParamHandle x; /**< X coordinate parameter */
            ff_ParamHandle y; /**< Y coordinate parameter */
        } point;
        struct {
            ff_EntityHandle p1; /**< First endpoint */
            ff_EntityHandle p2; /**< Second endpoint */
        } line;
        struct {
            ff_EntityHandle c; /**< Center point entity */
            ff_ParamHandle  r; /**< Radius parameter */
        } circle;
        struct {
            ff_EntityHandle p1; /**< First point on arc */
            ff_EntityHandle p2; /**< Second point on arc */
            ff_EntityHandle p3; /**< Third point on arc */
        } arc;
    } data; /**< Entity-specific data */
} ff_EntityDef;

/**
 * @brief Entity storage
 */
typedef struct ff_Entity {
    ff_EntityDef def; /**< Entity definition */
} ff_Entity;

/** @} */


/** @defgroup Expressions Expression System
 *  @brief Symbolic expression trees for constraint equations
 *  @{
 */

/**
 * @brief Expression operator type
 *
 * Defines the operation performed by an expression node.
 * Supports arithmetic, trigonometric, and mathematical operations.
 */
typedef enum {
    OperatorType_CONST,        /**< Constant value */
    OperatorType_PARAM,        /**< Direct parameter reference (by handle) */
    OperatorType_EXTR_PARAM,   /**< External parameter reference (legacy) */
    OperatorType_PARAM_IDX,    /**< Indexed parameter reference (constraint.pars[idx]) */
    OperatorType_ENTITY_IDX,   /**< Indexed entity reference (constraint.ents[idx]) */
    OperatorType_POINT_X,      /**< Point entity X parameter (ents[idx].point.x) */
    OperatorType_POINT_Y,      /**< Point entity Y parameter (ents[idx].point.y) */
    OperatorType_CIRCLE_R,     /**< Circle entity radius parameter (ents[idx].circle.r) */
    OperatorType_CIRCLE_C,     /**< Circle entity center (ents[idx].circle.c) */
    OperatorType_LINE_P1,      /**< Line entity first point (ents[idx].line.p1) */
    OperatorType_LINE_P2,      /**< Line entity second point (ents[idx].line.p2) */
    OperatorType_ADD,          /**< Addition (a + b) */
    OperatorType_SUB,          /**< Subtraction (a - b) */
    OperatorType_MUL,          /**< Multiplication (a * b) */
    OperatorType_DIV,          /**< Division (a / b) */
    OperatorType_SIN,          /**< Sine (sin(a)) */
    OperatorType_COS,          /**< Cosine (cos(a)) */
    OperatorType_ASIN,         /**< Arcsine (asin(a)) */
    OperatorType_ACOS,         /**< Arccosine (acos(a)) */
    OperatorType_SQRT,         /**< Square root (sqrt(a)) */
    OperatorType_SQR           /**< Square (a^2) */
} ff_OperatorType;

/**
 * @brief Expression tree node
 *
 * Represents a node in a symbolic expression tree.
 * The solver uses these to build constraint equations and compute derivatives.
 */
typedef struct ff_Expr {
    ff_OperatorType op_type;  /**< Operator type */
    struct ff_Expr* a;        /**< First operand (or only operand for unary ops) */
    struct ff_Expr* b;        /**< Second operand (for binary ops) */
    union {
        ff_float value;       /**< Constant value (for OperatorType_CONST) */
        ff_ParamHandle param_H; /**< Parameter handle (for OperatorType_PARAM) */
        uint16_t param_idx;   /**< Parameter array index (for OperatorType_PARAM_IDX) */
        uint16_t entity_idx;  /**< Entity array index (for OperatorType_ENTITY_IDX) */
    };
} ff_Expr;

/** @} */

/** @defgroup Constraints Constraints
 *  @brief Relationships between entities that the solver maintains
 *  @{
 */

/**
 * @brief Constraint type configuration
 *
 * Add custom constraint types here using CONS(NAME) macro.
 */
#define FF_CONSTRAINTS_CFG      \
    CONS(GENERAL)               \
    CONS(HORIZONTAL)            \

/**
 * @brief Constraint type enumeration
 */
typedef enum ff_ConstraintType{
    #define CONS(n) FF_##n,
    FF_CONSTRAINTS_CFG
    CONS(COUNT)
    #undef CONS
} ff_ConstraintType;

/** @brief Maximum entities per constraint */
#define FFCONS_MAXENT 16
/** @brief Maximum parameters per constraint */
#define FFCONS_MAXPAR 16

/** @brief Handle to a constraint */
typedef ff_GeneralHandle ff_ConstraintHandle;

/**
 * @brief Constraint definition
 */
typedef struct ff_ConstraintDef {
    ff_Expr* eq;                         /**< Constraint equation (should equal zero) */
    enum ff_ConstraintType type;         /**< Constraint type */
    ff_EntityHandle ents[FFCONS_MAXENT]; /**< Entities involved in constraint */
    ff_ParamHandle pars[FFCONS_MAXPAR];  /**< Parameters involved in constraint */
    uint16_t ent_count;                  /**< Number of entities in ents[] array */
    uint16_t par_count;                  /**< Number of parameters in pars[] array */
} ff_ConstraintDef;

/**
 * @brief Constraint storage with solver data
 */
typedef struct ff_Constraint {
    ff_ConstraintDef def; /**< Constraint definition */

    struct {
        ff_float    err;      /**< Current constraint error */
        ff_Expr**   dervs;    /**< Symbolic derivatives */
        ff_float*   dervs_y;  /**< Evaluated derivative values */
    } JMR; /**< Jacobian matrix row data */
} ff_Constraint;

/** @} */


/*
typedef struct Jacobian_Matrix_Row {

    struct ff_constraint* parent_constraint_adr;

    Expression* equation;
    double error;
    Expression** derivatives;

    double* dervs_value;
} Jacobian_Matrix_Row;

*/




/** @defgroup Sketch Sketch
 *  @brief Main container for a parametric sketch system
 *  @{
 */

FF_DECLARE_GENTABLE(ff_param,      ff_Parameter);
FF_DECLARE_GENTABLE(ff_entity,     ff_Entity);
FF_DECLARE_GENTABLE(ff_constraint, ff_Constraint);

/**
 * @brief Parametric sketch container
 *
 * Contains all parameters, entities, and constraints for a 2D sketch.
 * The solver adjusts parameter values to satisfy all constraints.
 */
typedef struct ff_Sketch {
    ff_param__table      params;      /**< Parameter storage */
    ff_entity__table     entities;    /**< Entity storage */
    ff_constraint__table constraints; /**< Constraint storage */

    bool link_outdated; /**< Whether entity-parameter links need updating */

    ff_float* normal_mtr;    /**< Normal matrix for solving */
    ff_float* itrm_sol;      /**< Intermediate solution vector */
    ff_float* cached_params; /**< Cached parameter values */

    ff_Constraint**  tmp_contraints;
    ff_Parameter**   tmp_params;

} ff_Sketch;





#pragma endregion;









/** @defgroup API Public API
 *  @brief Core functions for sketch manipulation and solving
 *  @{
 */

/**
 * @brief Initialize a sketch
 * @param skt Sketch to initialize
 * @param p_cap Parameter capacity
 * @param e_cap Entity capacity
 * @param c_cap Constraint capacity
 */
FF_API void ffSketch_Init(ff_Sketch* skt, uint16_t p_cap, uint16_t e_cap, uint16_t c_cap);

/**
 * @brief Free sketch resources
 * @param skt Sketch to free
 */
FF_API void ffSketch_Free(ff_Sketch* skt);

/**
 * @brief Solve the constraint system
 * @param skt Sketch to solve
 * @param tolerance Convergence tolerance
 * @param max_steps Maximum solver iterations
 * @return true if converged, false otherwise
 */
FF_API bool ffSketch_Solve(ff_Sketch* skt, double tolerance, uint32_t max_steps);

/** @brief Get default parameter definition */
FF_API ff_ParameterDef ff_ParameterDef_DEFAULT();

/** @brief Get default entity definition for given type */
FF_API ff_EntityDef ff_EntityDef_DEFAULT(enum ff_EntityType type);

/** @brief Get default constraint definition */
FF_API ff_ConstraintDef ff_ConstraintDef_DEFAULT();

/** @brief Check if parameter definition is valid */
FF_API bool ff_ParameterDef_IsValid(const ff_ParameterDef def);

/** @brief Check if entity definition is valid */
FF_API bool ff_EntityDef_IsValid(const ff_EntityDef def);

/** @brief Check if constraint definition is valid */
FF_API bool ff_ConstraintDef_IsValid(const ff_ConstraintDef def);

/** @brief Compare parameter handles for equality */
FF_API bool ffParam_Equals(ff_ParamHandle a, ff_ParamHandle b);

/** @brief Compare entity handles for equality */
FF_API bool ffEntity_Equals(ff_EntityHandle a, ff_EntityHandle b);

/** @brief Compare constraint handles for equality */
FF_API bool ffConstraint_Equals(ff_ConstraintHandle a, ff_ConstraintHandle b);

/**
 * @brief Add a parameter to the sketch
 * @param skt Sketch to add to
 * @param p_def Parameter definition
 * @return Handle to the new parameter
 */
FF_API ff_ParamHandle ffSketch_AddParameter(ff_Sketch* skt, const ff_ParameterDef p_def);

/**
 * @brief Add an entity to the sketch
 * @param skt Sketch to add to
 * @param e_def Entity definition
 * @return Handle to the new entity
 */
FF_API ff_EntityHandle ffSketch_AddEntity(ff_Sketch* skt, const ff_EntityDef e_def);

/**
 * @brief Add a constraint to the sketch
 * @param skt Sketch to add to
 * @param c_def Constraint definition
 * @return Handle to the new constraint
 */
FF_API ff_ConstraintHandle ffSketch_AddConstraint(ff_Sketch* skt, const ff_ConstraintDef c_def);

/**
 * @brief Delete a parameter from the sketch
 * @param skt Sketch to delete from
 * @param h Parameter handle
 * @return true if successfully deleted
 */
FF_API bool ffSketch_DeleteParameter(ff_Sketch* skt, ff_ParamHandle h);

/**
 * @brief Delete an entity from the sketch
 * @param skt Sketch to delete from
 * @param h Entity handle
 * @return true if successfully deleted
 */
FF_API bool ffSketch_DeleteEntity(ff_Sketch* skt, ff_EntityHandle h);

/**
 * @brief Delete a constraint from the sketch
 * @param skt Sketch to delete from
 * @param h Constraint handle
 * @return true if successfully deleted
 */
FF_API bool ffSketch_DeleteConstraint(ff_Sketch* skt, ff_ConstraintHandle h);

/**
 * @brief Get mutable parameter by handle
 * @param skt Sketch
 * @param h Parameter handle
 * @return Pointer to parameter, or NULL if invalid
 */
FF_API ff_Parameter* ffSketch_GetParameter(ff_Sketch* skt, ff_ParamHandle h);

/**
 * @brief Get const parameter by handle
 * @param skt Sketch
 * @param h Parameter handle
 * @return Const pointer to parameter, or NULL if invalid
 */
FF_API const ff_Parameter* ffSketch_GetParameter_Protected(const ff_Sketch* skt, ff_ParamHandle h);

/**
 * @brief Get mutable entity by handle
 * @param skt Sketch
 * @param h Entity handle
 * @return Pointer to entity, or NULL if invalid
 */
FF_API ff_Entity* ffSketch_GetEntity(ff_Sketch* skt, ff_EntityHandle h);

/**
 * @brief Get const entity by handle
 * @param skt Sketch
 * @param h Entity handle
 * @return Const pointer to entity, or NULL if invalid
 */
FF_API const ff_Entity* ffSketch_GetEntity_Protected(const ff_Sketch* skt, ff_EntityHandle h);

/**
 * @brief Get mutable constraint by handle
 * @param skt Sketch
 * @param h Constraint handle
 * @return Pointer to constraint, or NULL if invalid
 */
FF_API ff_Constraint* ffSketch_GetConstraint(ff_Sketch* skt, ff_ConstraintHandle h);

/**
 * @brief Get const constraint by handle
 * @param skt Sketch
 * @param h Constraint handle
 * @return Const pointer to constraint, or NULL if invalid
 */
FF_API const ff_Constraint* ffSketch_GetConstraint_Protected(const ff_Sketch* skt, ff_ConstraintHandle h);


/**
 * @brief Free an expression tree
 * @param expr Expression to free
 */
FF_API void expr_free(ff_Expr* expr);

/**
 * @brief Create expression with binary/unary operator
 * @param type Operator type
 * @param a First operand (or only operand for unary)
 * @param b Second operand (NULL for unary ops)
 * @return New expression node
 */
FF_API ff_Expr* exprInit_op(ff_OperatorType type, ff_Expr* a, ff_Expr* b);

/**
 * @brief Create constant expression
 * @param value Constant value
 * @return New constant expression
 */
FF_API ff_Expr* exprInit_const(ff_float value);

/**
 * @brief Create parameter reference expression
 * @param param_H Parameter handle
 * @return New parameter expression
 */
FF_API ff_Expr* exprInit_param(ff_ParamHandle param_H);

/**
 * @brief Create indexed parameter reference expression
 * @param idx Index into constraint.pars[] array
 * @return New indexed parameter expression
 */
FF_API ff_Expr* exprInit_param_idx(uint16_t idx);

/**
 * @brief Create indexed entity reference expression
 * @param idx Index into constraint.ents[] array
 * @return New indexed entity expression
 */
FF_API ff_Expr* exprInit_entity_idx(uint16_t idx);

/**
 * @brief Create expression for point entity's X parameter
 * @param entity_idx Index into constraint.ents[] array
 * @return Expression that resolves to ents[entity_idx].point.x parameter
 * @note Only valid if ents[entity_idx] is a POINT entity
 */
FF_API ff_Expr* exprInit_point_x(uint16_t entity_idx);

/**
 * @brief Create expression for point entity's Y parameter
 * @param entity_idx Index into constraint.ents[] array
 * @return Expression that resolves to ents[entity_idx].point.y parameter
 * @note Only valid if ents[entity_idx] is a POINT entity
 */
FF_API ff_Expr* exprInit_point_y(uint16_t entity_idx);

/**
 * @brief Create expression for circle entity's radius parameter
 * @param entity_idx Index into constraint.ents[] array
 * @return Expression that resolves to ents[entity_idx].circle.r parameter
 * @note Only valid if ents[entity_idx] is a CIRCLE entity
 */
FF_API ff_Expr* exprInit_circle_radius(uint16_t entity_idx);

/**
 * @brief Create expression for circle entity's center point index
 * @param entity_idx Index into constraint.ents[] array
 * @return Expression that resolves to ents[entity_idx].circle.c entity
 * @note Only valid if ents[entity_idx] is a CIRCLE entity
 */
FF_API ff_Expr* exprInit_circle_center(uint16_t entity_idx);

/**
 * @brief Evaluate an expression within a constraint context
 * @param expr Expression to evaluate
 * @param constraint Constraint providing entity and parameter arrays
 * @param sketch Sketch containing all entities and parameters
 * @return Evaluated result
 */
FF_API ff_float expr_evaluate_constraint(ff_Expr* expr, const ff_Constraint* constraint, const ff_Sketch* sketch);

/**
 * @brief Evaluate an expression (legacy, direct parameter table access)
 * @param expr Expression to evaluate
 * @param t Parameter table for looking up parameter values
 * @return Evaluated result
 */
FF_API ff_float expr_evaluate(ff_Expr* expr, const ff_param__table *t);

/**
 * @brief Compute symbolic derivative of expression
 * @param expr Expression to differentiate
 * @param indp_param Parameter to differentiate with respect to
 * @param protect_params Whether to protect parameters from modification
 * @return New expression representing the derivative
 */
FF_API ff_Expr* expr_derivative(ff_Expr* expr, ff_ParamHandle indp_param, bool protect_params);

/** @} */














#ifdef FF_FREEFORM_IMPL_

#pragma region General
static int ff_ERROR(const char* msg) {
    printf("FreeForm Critical Error: \'%s\'\n", msg);
    exit(1);
}
#pragma endregion


#pragma region Handle Management

/* ===== Generic gen+idx table: definitions ===== */
#define FF__CLAMP_CAP_16(x) ((uint16_t)((x) > 0xFFFF ? 0xFFFF : (x)))

#define FF_DEFINE_GENTABLE(PREFIX, PAYLOAD_T)                                             \
    static void PREFIX##TBL__grow(PREFIX##__table* t, uint16_t add) {                        \
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
            new_slots[i].payload = PAYLOAD_T##_DEFAULT();                                 \
        }                                                                                 \
        new_slots[new_cap - 1].next_free = t->free_head;                                  \
        t->free_head = t->cap;                                                            \
        t->slots    = new_slots;                                                          \
        t->cap      = new_cap;                                                            \
    }                                                                                     \
                                                                                          \
    void PREFIX##TBL_init(PREFIX##__table* t, uint16_t initial_cap) {                        \
        t->slots       = NULL;                                                            \
        t->cap         = 0;                                                               \
        t->alive_count = 0;                                                               \
        t->free_head   = FF_INVALID_INDEX;                                                \
        if (initial_cap) PREFIX##TBL__grow(t, initial_cap);                                  \
    }                                                                                     \
                                                                                          \
    void PREFIX##TBL_free(PREFIX##__table* t) {                                              \
        free(t->slots);                                                                   \
        t->slots = NULL;                                                                  \
        t->cap = t->alive_count = 0;                                                      \
        t->free_head = FF_INVALID_INDEX;                                                  \
    }                                                                                     \
                                                                                          \
    ff_GeneralHandle PREFIX##TBL_create(PREFIX##__table* t, const PAYLOAD_T* init) {         \
        if (t->free_head == FF_INVALID_INDEX) {                                           \
            /* grow (geometric) */                                                        \
            uint16_t add = (t->cap < 64) ? 64 : (t->cap >> 1);                            \
            if (add == 0) add = 64;                                                       \
            PREFIX##TBL__grow(t, add);                                                       \
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
    static inline bool PREFIX##TBL__valid(const PREFIX##__table* t, ff_GeneralHandle h) {    \
        return (h.idx != FF_INVALID_INDEX) && (h.idx < t->cap);                           \
    }                                                                                     \
                                                                                          \
    bool PREFIX##TBL_alive(const PREFIX##__table* t, ff_GeneralHandle h) {                   \
        if (!PREFIX##TBL__valid(t, h)) return false;                                         \
        const PREFIX##__slot* s = &t->slots[h.idx];                                       \
        return s->alive && (s->gen == h.gen);                                             \
    }                                                                                     \
                                                                                          \
    PAYLOAD_T* PREFIX##TBL_get(PREFIX##__table* t, ff_GeneralHandle h) {                     \
        if (!PREFIX##TBL_alive(t, h)) return NULL;                                           \
        return &t->slots[h.idx].payload;                                                  \
    }                                                                                     \
                                                                                          \
    const PAYLOAD_T* PREFIX##TBL_get_const(const PREFIX##__table* t, ff_GeneralHandle h) {   \
        if (!PREFIX##TBL_alive(t, h)) return NULL;                                           \
        return &t->slots[h.idx].payload;                                                  \
    }                                                                                     \
                                                                                          \
    bool PREFIX##TBL_destroy(PREFIX##__table* t, ff_GeneralHandle h) {                       \
        if (!PREFIX##TBL_alive(t, h)) return false;                                          \
        PREFIX##__slot* s = &t->slots[h.idx];                                             \
        s->alive = false;                                                                 \
        s->gen  += 1u; /* bump generation to invalidate stale handles */                  \
        s->next_free = t->free_head;                                                      \
        t->free_head = h.idx;                                                             \
        if (t->alive_count) t->alive_count--;                                             \
        return true;                                                                      \
    }                                                                                     \
                                                                                          \
    const ff_GeneralHandle PREFIX##_INVALIDHANDLE = { FF_INVALID_INDEX, 0u };             \
  
    static ff_Parameter ff_Parameter_DEFAULT() {
        ff_Parameter obj = (ff_Parameter) {0};
        return obj;
    }

    static ff_Entity ff_Entity_DEFAULT() {
        ff_Entity obj = (ff_Entity) {0};
        return obj;
    }

    static ff_Constraint ff_Constraint_DEFAULT() {
        ff_Constraint obj = (ff_Constraint) {0};

        obj.JMR.err = 0.0;
        obj.JMR.dervs = NULL;
        obj.JMR.dervs_y = NULL;

        return obj;
    }


    FF_DEFINE_GENTABLE(ff_param,      ff_Parameter)
    FF_DEFINE_GENTABLE(ff_entity,     ff_Entity)
    FF_DEFINE_GENTABLE(ff_constraint, ff_Constraint)
    static inline bool ffGenHandle_Equals(ff_GeneralHandle a, ff_GeneralHandle b) {
        return ((a.gen == b.gen) && (a.idx == b.idx));
    }


#pragma endregion //Handling Handles 



#pragma region Type Defaults/Validity


ff_ParameterDef ff_ParameterDef_DEFAULT() {
    ff_ParameterDef def;
    def.v = 0.0f;
    return def;
}
bool ff_ParameterDef_IsValid(const ff_ParameterDef def) {
    (void)def;
    return true;
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


bool ff_EntityDef_IsValid (const ff_EntityDef def) {
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

ff_ConstraintDef ff_ConstraintDef_DEFAULT() {
    ff_ConstraintDef def;
    def.eq = NULL;
    def.type = FF_GENERAL;
    return def;
}

bool ff_ConstraintDef_IsValid(const ff_ConstraintDef def) {
    if (def.eq == NULL) return false;
    if (def.type >= FF_COUNT) return false;
    return true;
}




#pragma endregion

#pragma region High Level Handles

ff_ParamHandle      ffSketch_AddParameter    (ff_Sketch* skt, const ff_ParameterDef  p_def) {
    if (!ff_ParameterDef_IsValid(p_def)) return ff_param_INVALIDHANDLE;
    ff_Parameter param = (ff_Parameter) { .def = p_def };
    return ff_paramTBL_create(&skt->params, &param);
}
ff_EntityHandle     ffSketch_AddEntity       (ff_Sketch* skt, const ff_EntityDef     e_def) {
    if (!ff_EntityDef_IsValid(e_def)) return ff_entity_INVALIDHANDLE;
    ff_Entity ent = (ff_Entity) { .def = e_def };
    return ff_entityTBL_create(&skt->entities, &ent);
}
ff_ConstraintHandle ffSketch_AddConstraint   (ff_Sketch* skt, const ff_ConstraintDef c_def) {
    if (!ff_ConstraintDef_IsValid(c_def)) return ff_constraint_INVALIDHANDLE;
    ff_Constraint cons = (ff_Constraint) { .def = c_def, .JMR = {0} };
    return ff_constraintTBL_create(&skt->constraints, &cons);
}

bool ffSketch_DeleteParameter(ff_Sketch* skt, ff_ParamHandle h) {
    return ff_paramTBL_destroy(&skt->params, h);
}
bool ffSketch_DeleteEntity(ff_Sketch* skt, ff_EntityHandle h) {
    return ff_entityTBL_destroy(&skt->entities, h);
}
bool ffSketch_DeleteConstraint(ff_Sketch* skt, ff_ConstraintHandle h) {
    return ff_constraintTBL_destroy(&skt->constraints, h);
}

ff_Parameter* ffSketch_GetParameter(ff_Sketch* skt, ff_ParamHandle h) {
    return ff_paramTBL_get(&skt->params, h);
}
const ff_Parameter* ffSketch_GetParameter_Protected(const ff_Sketch* skt, ff_ParamHandle h) {
    return ff_paramTBL_get_const(&skt->params, h);
}
bool ffParam_Equals(ff_ParamHandle a, ff_ParamHandle b) { return ffGenHandle_Equals(a,b); }

//Entities
ff_Entity* ffSketch_GetEntity(ff_Sketch* skt, ff_EntityHandle h) {
    return ff_entityTBL_get(&skt->entities, h);
}
const ff_Entity* ffSketch_GetEntity_Protected(const ff_Sketch* skt, ff_EntityHandle h) {
    return ff_entityTBL_get_const(&skt->entities, h);
}
bool ffEntity_Equals(ff_EntityHandle a, ff_EntityHandle b) { return ffGenHandle_Equals(a,b); }

//Constraint
ff_Constraint* ffSketch_GetConstraint(ff_Sketch* skt, ff_ConstraintHandle h) {
    return ff_constraintTBL_get(&skt->constraints, h);
}
const ff_Constraint* ffSketch_GetConstraint_Protected(const ff_Sketch* skt, ff_ConstraintHandle h) {
    return ff_constraintTBL_get_const(&skt->constraints, h);
}
bool ffConstraint_Equals(ff_ConstraintHandle a, ff_ConstraintHandle b) { return ffGenHandle_Equals(a,b); }
















#pragma endregion



#pragma region Equations



void expr_free(ff_Expr* expr) { //TODO handle extr_param better.
    if (expr->op_type == OperatorType_EXTR_PARAM) {
        free(expr);
        return;
    }
    if (expr->a) expr_free(expr->a);
    if (expr->b) expr_free(expr->b);

    free(expr);
}


ff_Expr* exprInit_op(ff_OperatorType op_type, ff_Expr* a, ff_Expr* b) {
    ff_Expr* expr = malloc(sizeof(ff_Expr));
    expr->op_type = op_type;
    expr->a = a;
    expr->b = b;
    return expr;
}

ff_Expr* exprInit_const(ff_float value) {
    ff_Expr* expr = malloc(sizeof(ff_Expr));
    expr->op_type = OperatorType_CONST;
    expr->value = value;
    expr->a = expr->b = NULL;
    expr->param_H = ff_param_INVALIDHANDLE;
    return expr;
}

ff_Expr* exprInit_param(ff_ParamHandle param_H) {
    ff_Expr* expr = malloc(sizeof(ff_Expr));
    expr->op_type = OperatorType_PARAM;
    expr->param_H = param_H;
    expr->a = expr->b = NULL;
    return expr;
}

ff_Expr* exprInit_param_idx(uint16_t idx) {
    ff_Expr* expr = malloc(sizeof(ff_Expr));
    expr->op_type = OperatorType_PARAM_IDX;
    expr->param_idx = idx;
    expr->a = expr->b = NULL;
    return expr;
}

ff_Expr* exprInit_entity_idx(uint16_t idx) {
    ff_Expr* expr = malloc(sizeof(ff_Expr));
    expr->op_type = OperatorType_ENTITY_IDX;
    expr->entity_idx = idx;
    expr->a = expr->b = NULL;
    return expr;
}

ff_Expr* exprInit_point_x(uint16_t entity_idx) {
    ff_Expr* expr = malloc(sizeof(ff_Expr));
    expr->op_type = OperatorType_POINT_X;
    expr->entity_idx = entity_idx;
    expr->a = expr->b = NULL;
    return expr;
}

ff_Expr* exprInit_point_y(uint16_t entity_idx) {
    ff_Expr* expr = malloc(sizeof(ff_Expr));
    expr->op_type = OperatorType_POINT_Y;
    expr->entity_idx = entity_idx;
    expr->a = expr->b = NULL;
    return expr;
}

ff_Expr* exprInit_circle_radius(uint16_t entity_idx) {
    ff_Expr* expr = malloc(sizeof(ff_Expr));
    expr->op_type = OperatorType_CIRCLE_R;
    expr->entity_idx = entity_idx;
    expr->a = expr->b = NULL;
    return expr;
}

ff_Expr* exprInit_circle_center(uint16_t entity_idx) {
    ff_Expr* expr = malloc(sizeof(ff_Expr));
    expr->op_type = OperatorType_CIRCLE_C;
    expr->entity_idx = entity_idx;
    expr->a = expr->b = NULL;
    return expr;
}

static ff_Expr* exprInit_external_param(ff_Expr* expr) {
    ff_Expr* new_expr = malloc(sizeof(ff_Expr));
    new_expr->op_type = OperatorType_EXTR_PARAM;
    new_expr->a = expr;
    new_expr->b = NULL;
    new_expr->param_H = ff_param_INVALIDHANDLE;
    return new_expr;
}

// Evaluate the expression
ff_float expr_evaluate(ff_Expr* expr, const ff_param__table *t) {

    

    switch (expr->op_type) {
        case OperatorType_CONST: return expr->value;
        case OperatorType_PARAM: {
            const ff_Parameter * p = ff_paramTBL_get_const(t, expr->param_H);
            return p->def.v;
        }
        case OperatorType_EXTR_PARAM: return expr_evaluate(expr->a,t);
        case OperatorType_ADD: return expr_evaluate(expr->a,t) + expr_evaluate(expr->b,t);
        case OperatorType_SUB: return expr_evaluate(expr->a,t) - expr_evaluate(expr->b,t);
        case OperatorType_MUL: return expr_evaluate(expr->a,t) * expr_evaluate(expr->b,t);
        case OperatorType_DIV: return expr_evaluate(expr->a,t) / expr_evaluate(expr->b,t);
        case OperatorType_SIN: return sin(expr_evaluate(expr->a,t));
        case OperatorType_COS: return cos(expr_evaluate(expr->a,t));
        case OperatorType_ASIN: return asin(expr_evaluate(expr->a,t));
        case OperatorType_ACOS: return acos(expr_evaluate(expr->a,t));
        case OperatorType_SQRT: return sqrt(expr_evaluate(expr->a,t));
        case OperatorType_SQR: { ff_float val = expr_evaluate(expr->a,t); return val * val; }
    }
    return 0.0;
}

// Evaluate expression within a constraint context (supports indexed expressions)
ff_float expr_evaluate_constraint(ff_Expr* expr, const ff_Constraint* constraint, const ff_Sketch* sketch) {
    if (!expr) return 0.0;

    switch (expr->op_type) {
        case OperatorType_CONST:
            return expr->value;

        case OperatorType_PARAM: {
            const ff_Parameter* p = ff_paramTBL_get_const(&sketch->params, expr->param_H);
            return p ? p->def.v : 0.0;
        }

        case OperatorType_PARAM_IDX: {
            if (expr->param_idx >= constraint->def.par_count) return 0.0;
            ff_ParamHandle ph = constraint->def.pars[expr->param_idx];
            const ff_Parameter* p = ff_paramTBL_get_const(&sketch->params, ph);
            return p ? p->def.v : 0.0;
        }

        case OperatorType_POINT_X: {
            if (expr->entity_idx >= constraint->def.ent_count) return 0.0;
            ff_EntityHandle eh = constraint->def.ents[expr->entity_idx];
            const ff_Entity* e = ff_entityTBL_get_const(&sketch->entities, eh);
            if (!e || e->def.type != FF_POINT) return 0.0;
            const ff_Parameter* p = ff_paramTBL_get_const(&sketch->params, e->def.data.point.x);
            return p ? p->def.v : 0.0;
        }

        case OperatorType_POINT_Y: {
            if (expr->entity_idx >= constraint->def.ent_count) return 0.0;
            ff_EntityHandle eh = constraint->def.ents[expr->entity_idx];
            const ff_Entity* e = ff_entityTBL_get_const(&sketch->entities, eh);
            if (!e || e->def.type != FF_POINT) return 0.0;
            const ff_Parameter* p = ff_paramTBL_get_const(&sketch->params, e->def.data.point.y);
            return p ? p->def.v : 0.0;
        }

        case OperatorType_CIRCLE_R: {
            if (expr->entity_idx >= constraint->def.ent_count) return 0.0;
            ff_EntityHandle eh = constraint->def.ents[expr->entity_idx];
            const ff_Entity* e = ff_entityTBL_get_const(&sketch->entities, eh);
            if (!e || e->def.type != FF_CIRCLE) return 0.0;
            const ff_Parameter* p = ff_paramTBL_get_const(&sketch->params, e->def.data.circle.r);
            return p ? p->def.v : 0.0;
        }

        case OperatorType_EXTR_PARAM:
            return expr_evaluate_constraint(expr->a, constraint, sketch);

        case OperatorType_ADD:
            return expr_evaluate_constraint(expr->a, constraint, sketch) +
                   expr_evaluate_constraint(expr->b, constraint, sketch);

        case OperatorType_SUB:
            return expr_evaluate_constraint(expr->a, constraint, sketch) -
                   expr_evaluate_constraint(expr->b, constraint, sketch);

        case OperatorType_MUL:
            return expr_evaluate_constraint(expr->a, constraint, sketch) *
                   expr_evaluate_constraint(expr->b, constraint, sketch);

        case OperatorType_DIV:
            return expr_evaluate_constraint(expr->a, constraint, sketch) /
                   expr_evaluate_constraint(expr->b, constraint, sketch);

        case OperatorType_SIN:
            return sin(expr_evaluate_constraint(expr->a, constraint, sketch));

        case OperatorType_COS:
            return cos(expr_evaluate_constraint(expr->a, constraint, sketch));

        case OperatorType_ASIN:
            return asin(expr_evaluate_constraint(expr->a, constraint, sketch));

        case OperatorType_ACOS:
            return acos(expr_evaluate_constraint(expr->a, constraint, sketch));

        case OperatorType_SQRT:
            return sqrt(expr_evaluate_constraint(expr->a, constraint, sketch));

        case OperatorType_SQR: {
            ff_float val = expr_evaluate_constraint(expr->a, constraint, sketch);
            return val * val;
        }
    }
    return 0.0;
}

#define TRY_EXTR_PARAM(expr) protect_params ? exprInit_external_param(expr) : expr

// Differentiate the expression with respect to a parameter
ff_Expr* expr_derivative(ff_Expr* expr, ff_ParamHandle indp_param, bool protect_params) {
    switch (expr->op_type) {
        case OperatorType_CONST:
            return exprInit_const(0.0);
        case OperatorType_PARAM:
            return ffParam_Equals(expr->param_H, indp_param) ? 
            exprInit_const(1.0) : exprInit_const(0.0);
        case OperatorType_EXTR_PARAM:
            return expr_derivative(expr->a, indp_param, protect_params);
        case OperatorType_ADD:
            return exprInit_op(OperatorType_ADD, expr_derivative(expr->a, indp_param, protect_params), expr_derivative(expr->b, indp_param, protect_params));
        case OperatorType_SUB:
            return exprInit_op(OperatorType_SUB, expr_derivative(expr->a, indp_param, protect_params), expr_derivative(expr->b, indp_param, protect_params));
        case OperatorType_MUL:
            return exprInit_op(OperatorType_ADD,
                exprInit_op(OperatorType_MUL, expr_derivative(expr->a, indp_param, protect_params), TRY_EXTR_PARAM(expr->b)),
                exprInit_op(OperatorType_MUL, TRY_EXTR_PARAM(expr->a), expr_derivative(expr->b, indp_param, protect_params))
            );
        case OperatorType_DIV:
            return exprInit_op(OperatorType_DIV,
                exprInit_op(OperatorType_SUB,
                    exprInit_op(OperatorType_MUL, expr_derivative(expr->a, indp_param, protect_params), TRY_EXTR_PARAM(expr->b)),
                    exprInit_op(OperatorType_MUL, TRY_EXTR_PARAM(expr->a), expr_derivative(expr->b, indp_param, protect_params))
                ),
                exprInit_op(OperatorType_MUL, TRY_EXTR_PARAM(expr->b), TRY_EXTR_PARAM(expr->b))
            );
        case OperatorType_SIN:
            return exprInit_op(OperatorType_MUL, expr_derivative(expr->a, indp_param, protect_params), exprInit_op(OperatorType_COS, expr->a, NULL));
        case OperatorType_COS:
            return exprInit_op(OperatorType_MUL,
                exprInit_op(OperatorType_MUL, exprInit_const(-1.0),
                    exprInit_op(OperatorType_SIN, expr->a, NULL)),
                expr_derivative(expr->a, indp_param, protect_params));    
        case OperatorType_ASIN:
            return exprInit_op(OperatorType_DIV, expr_derivative(expr->a, indp_param, protect_params), exprInit_op(OperatorType_SQRT,
                exprInit_op(OperatorType_SUB, exprInit_const(1.0), exprInit_op(OperatorType_SQR, TRY_EXTR_PARAM(expr->a), NULL)), NULL));
        case OperatorType_ACOS:
            return exprInit_op(OperatorType_DIV, exprInit_op(OperatorType_MUL, exprInit_const(-1.0), expr_derivative(expr->a, indp_param, protect_params)), exprInit_op(OperatorType_SQRT,
                exprInit_op(OperatorType_SUB, exprInit_const(1.0), exprInit_op(OperatorType_SQR, TRY_EXTR_PARAM(expr->a), NULL)), NULL));
        case OperatorType_SQRT:
            return exprInit_op(OperatorType_DIV, expr_derivative(expr->a, indp_param, protect_params),
                exprInit_op(OperatorType_MUL, exprInit_const(2.0), exprInit_op(OperatorType_SQRT, TRY_EXTR_PARAM(expr->a), NULL)));
        case OperatorType_SQR:
            return exprInit_op(OperatorType_MUL,
                exprInit_const(2.0),
                exprInit_op(OperatorType_MUL, TRY_EXTR_PARAM(expr->a), expr_derivative(expr->a, indp_param, protect_params))
            );
        default:  //unhandled op.
            ff_ERROR("Constraint undefined"); //TODO@ make this error msg actually say something meaningful
        return NULL;
         
    }

    return NULL;
}

#pragma endregion




/* ===== Sketch helpers ===== */
void ffSketch_Init(ff_Sketch* skt, uint16_t p_cap, uint16_t e_cap, uint16_t c_cap) {
    ff_paramTBL_init(&skt->params,      p_cap);
    ff_entityTBL_init(&skt->entities,   e_cap);
    ff_constraintTBL_init(&skt->constraints, c_cap);

    /*
    for (uint16_t i = 0; i < skt->params.cap; i++) {
        ff_param__slot* slot = &skt->params.slots[i];
        ff_Parameter* param = &slot->payload;
    }

    for (uint16_t i = 0; i < skt->entities.cap; i++) {
        ff_entity__slot* slot = &skt->entities.slots[i];
        ff_Entity* ent = &slot->payload;
    }

    for (uint16_t i = 0; i < skt->constraints.cap; i++) {
        ff_constraint__slot* slot = &skt->constraints.slots[i];
        ff_Constraint* cons = &slot->payload;
    }
    */

    skt->link_outdated = true;

    skt->normal_mtr     = NULL;
    skt->itrm_sol       = NULL;
    skt->cached_params  = NULL;

    skt->tmp_contraints = NULL;
    skt->tmp_params     = NULL;
}


static inline void ffSketch_FreeToBaseState(ff_Sketch* skt) {

    uint16_t  rows = skt->constraints.alive_count;
    uint16_t  cols = skt->params.alive_count;

    for (uint16_t i = 0; i < skt->constraints.cap; i++) {
        if (skt->constraints.slots[i].alive) {
            ff_Constraint* cons = &skt->constraints.slots[i].payload;

            if (cons->JMR.dervs) {
                for (uint16_t i = 0; i < skt->params.alive_count; i++) {
                    expr_free(cons->JMR.dervs[i]);
                }
                free(cons->JMR.dervs);
            }
            if (cons->JMR.dervs_y) free(cons->JMR.dervs_y);

        }
    }

    for (uint16_t i = 0; i < skt->params.cap; i++) {
        if (skt->params.slots[i].alive) {
            ff_Parameter* param = &skt->params.slots[i].payload;
            

        }
    }

    //normal mat, intermdeiat esol
    if (skt->normal_mtr) free(skt->normal_mtr);
    if (skt->itrm_sol) free(skt->itrm_sol);
    if (skt->cached_params) free(skt->cached_params);
    
    if (skt->tmp_contraints) free(skt->tmp_contraints);
    if (skt->tmp_params) free(skt->tmp_params);

    skt->normal_mtr = NULL;
    skt->itrm_sol = NULL;
    skt->cached_params = NULL;

    skt->tmp_contraints = NULL;
    skt->tmp_params = NULL;
}





void ffSketch_Free(ff_Sketch* skt) {

    ffSketch_FreeToBaseState(skt);

    ff_paramTBL_free(&skt->params);
    ff_entityTBL_free(&skt->entities);
    ff_constraintTBL_free(&skt->constraints);
}

//todo add this to ff api?
static void ffSketch_tryRelink(ff_Sketch* skt) {
    if (!skt->link_outdated) return;

    ffSketch_FreeToBaseState(skt);

    uint16_t  eq_cnt = skt->constraints.alive_count;
    uint16_t  par_cnt = skt->params.alive_count;

    skt->tmp_contraints = malloc(sizeof(ff_Constraint*)     * eq_cnt);
    skt->tmp_params     = malloc(sizeof(ff_ParamHandle*)    * par_cnt);
    
    uint16_t _c = 0;
    for (uint16_t consIdx = 0; consIdx < skt->constraints.cap; consIdx++) {
        if (skt->constraints.slots[consIdx].alive) {
            ff_Constraint* cons = &skt->constraints.slots[consIdx].payload;
            skt->tmp_contraints[_c++] = cons;

            cons->JMR.dervs = malloc(sizeof(ff_Expr*) * par_cnt);
            cons->JMR.dervs_y = malloc(sizeof(double) * par_cnt);

            uint16_t _p = 0;
            for (uint16_t paramIdx = 0; paramIdx < skt->params.cap; paramIdx++) {
                if (skt->params.slots[paramIdx].alive) {
                    ff_ParamHandle pH = (ff_ParamHandle){ .idx = paramIdx, .gen = skt->params.slots[paramIdx].gen };

                    cons->JMR.dervs[_p] = expr_derivative(cons->def.eq, pH, true);
                    cons->JMR.dervs_y[_p] = 0.0;

                    _p++;
                    if (_p >= par_cnt) break;
                }
            }

        }
    }

    uint16_t _p = 0; //todo find a better way to do dis
    for (uint16_t paramIdx = 0; paramIdx < skt->params.cap; paramIdx++) {
        if (skt->params.slots[paramIdx].alive) {
            ff_Parameter* param = &skt->params.slots[paramIdx].payload;
            skt->tmp_params[_p++] = param;
            if (_p >= par_cnt) break;
        }
    }


    skt->normal_mtr = malloc(sizeof(ff_float) * eq_cnt * eq_cnt);
    skt->itrm_sol   = malloc(sizeof(ff_float) * eq_cnt);
    skt->cached_params = malloc(sizeof(ff_float) * par_cnt);

    skt->link_outdated = false;
}




// -=-=-=-=- High level push/pop -=-=-=-=- //



// -=-=-=-=- High level getters / equality -=-=-=-=- //


//Params






#pragma region SOLVING



static inline bool ffSketch_calcError(ff_Sketch* skt, double tolerance) {
    bool converged = true;

    for (uint16_t i = 0; i < skt->constraints.alive_count; i++) {
        ff_Constraint* cons = skt->tmp_contraints[i];
        cons->JMR.err = expr_evaluate(cons->def.eq, &skt->params);
        if (fabs(cons->JMR.err) > tolerance) converged = false;           
        printf("Constraint %d error: %f\n", i, cons->JMR.err);
    }

    return converged;
}

bool ffSketch_Solve(ff_Sketch* skt, double tolerance, uint32_t max_steps) {

    ffSketch_tryRelink(skt);

    uint16_t  rows = skt->constraints.alive_count;
    uint16_t  cols = skt->params.alive_count;
    
    if (!(rows && cols)) return true;

    double temp = 0.0;
    const double epsilon = 1e-10;

    bool converged = false;




    for (uint32_t step = 0; step < max_steps; step++) {

        printf("* Iteration (%d/%d)\n", step + 1, max_steps);
      

        //Calculate error of system. If we are converged we are done.
        if (ffSketch_calcError(skt, tolerance)) {
            converged = true;
            break;
        }

        for (uint16_t i = 0; i < skt->constraints.alive_count; i++) {
            ff_Constraint* cons = skt->tmp_contraints[i];
            for (uint16_t p = 0; p < skt->params.alive_count; p++) {
                cons->JMR.dervs_y[p] = expr_evaluate(cons->JMR.dervs[p], &skt->params);
                printf("D= %f\n", cons->JMR.dervs_y[p]);
            }
        }
  
        //Solve by least squares:

        //Start
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < rows; c++) {
                double sum = 0.0;
                for (int i = 0; i < cols; i++) {
                    double cV = skt->tmp_contraints[c]->JMR.dervs_y[i];
                    double rV = skt->tmp_contraints[r]->JMR.dervs_y[i];
                    if (cV == 0.0 || rV == 0.0) continue;    
                    sum += rV * cV;
                }
                skt->normal_mtr[r + c * rows] = sum;
            }
        }
        
          
        //Gaussian Solve
        for (int row = 0; row < rows; row++) {
            int pivot_row = row;
            double max_value = 0.0;
    
            // Find the pivot row (max absolute value in column)
            for (int candidate_row = row; candidate_row < rows; candidate_row++) {
                if (candidate_row >= rows) ff_ERROR("Bounds error");  //todo will these bounds errors ever fire?
                if (fabs(skt->normal_mtr[candidate_row + row * rows]) > max_value) {
                    max_value = fabs(skt->normal_mtr[candidate_row + row * rows]);
                    pivot_row = candidate_row;
                }
            }
    
            if (max_value < epsilon) {
                printf("Small pivot element: %f at row %d\n", max_value, row);
                continue;
            }
    
            // Swap rows in the normal matrix
            for (int col = 0; col < rows; col++) {
                temp = skt->normal_mtr[row + col * rows];
                skt->normal_mtr[row + col * rows] = skt->normal_mtr[pivot_row + col * rows];
                skt->normal_mtr[pivot_row + col * rows] = temp;
            }
    
            // Swap elements in the constraint error vector
            temp = skt->tmp_contraints[row]->JMR.err;
            skt->tmp_contraints[row      ]->JMR.err = skt->tmp_contraints[pivot_row]->JMR.err;
            skt->tmp_contraints[pivot_row]->JMR.err = temp;
    
            // Eliminate entries below the pivot
            for (int target_row = row + 1; target_row < rows; target_row++) {
                if(target_row >= rows) ff_ERROR("Bounds error"); //todo will these bounds errors ever fire?
                if (fabs(skt->normal_mtr[row + row * rows]) < epsilon) {
                    printf("Division by zero at row %d\n", row);
                    continue;
                }
                double coefficient = skt->normal_mtr[target_row + row * rows] / skt->normal_mtr[row + row * rows];
                for (int col = 0; col < rows; col++) {
                    skt->normal_mtr[target_row + col* rows] -= skt->normal_mtr[row + col* rows] * coefficient;
                }
                skt->tmp_contraints[target_row]->JMR.err -= skt->tmp_contraints[row]->JMR.err * coefficient;
            }
        }

              
        //Back sub
        for (int row = rows - 1; row >= 0; row--) {
            if (fabs(skt->normal_mtr[row + row * rows]) < epsilon) {
                printf("Back substitution failed at row %d\n", row);
                continue;
            }
    
            ff_float solution_value = skt->tmp_contraints[row]->JMR.err / skt->normal_mtr[row + row * rows];
            for (int prev_row = rows - 1; prev_row > row; prev_row--) {
                solution_value -= skt->itrm_sol[prev_row] * skt->normal_mtr[row + prev_row * rows] / skt->normal_mtr[row + row * rows];
            }
            skt->itrm_sol[row] = solution_value;
        }

        //Update parameters based on this steps corrections
        for (int c = 0; c < cols; c++) {
            ff_float corr = 0.0;
            for (int r = 0; r < rows; r++) {
                corr += skt->itrm_sol[r] * skt->tmp_contraints[r]->JMR.dervs_y[c];
            }
            printf("Correction is %f\n", corr);
            skt->tmp_params[c]->def.v -= corr;
        }

       printf("--==--==--==--\n\n\n");

    } //For step in maxsteps

    
    

    //End of solving process.
    //TODO@ revert params here when failed
    return converged;
}






#pragma endregion




#endif //FF_FREEFORM_IMPL_










#undef FF_CONSTRAINTS_CFG

#ifdef __cplusplus
}
#endif //extern 'C'

#endif //FF_FREEFORM_H_
