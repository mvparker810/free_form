#pragma once


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <float.h>

#include "freeform.h"


// CONFIG

#define FREEFORM_MAXIMUM_PARAMETERS 2048
#define FREEFORM_MAXIMUM_ELEMENTS        2048
#define FREEFORM_MAXIMUM_CONSTRAINTS     2048

static inline void FREEFORM_PRINT_ERR(const char* msg) {
    fprintf(stderr, "FreeForm Error: %s\n", msg);
    assert(false);
}

//solving
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
} OperatorType;

typedef struct Expression {
    OperatorType type;
    struct Expression* a;
    struct Expression* b;
    double value; // for consts
    struct ff_parameter* param_value; // ptr to param for EXPR_PARAM
} Expression;

typedef struct ff_constraint;

typedef struct Jacobian_Matrix_Row {

    struct ff_constraint* parent_constraint_adr;

    Expression* equation;
    double error;
    Expression** derivatives;

    double* dervs_value;
} Jacobian_Matrix_Row;


void expr_free(Expression* expr);

Expression* exprInit_op(OperatorType type, Expression* a, Expression* b);
Expression* exprInit_const(double value);
Expression* exprInit_param(struct ff_parameter* param_value);


double expr_evaluate(Expression* expr);
Expression* expr_derivative(Expression* expr, struct ff_parameter* param, bool protect_params);



//Structs

typedef struct ff_vec2 {
    double x;
    double y;
} ff_vec2;


// -- POINT -- //

typedef struct ff_parameter {
    double v;
    int status;
} ff_parameter;

typedef struct ff_point {
    struct ff_parameter* x;
    struct ff_parameter* y;
} ff_point;

// -- LINE -- //

typedef struct ff_line {
    struct ff_point* p1;
    struct ff_point* p2;
} ff_line;

// -- CIRCLE -- //

typedef struct ff_circle {
    struct ff_point* center;
    struct ff_parameter* radius;
} ff_circle;

// -- ARC -- //

typedef struct ff_arc {
    struct ff_point* start;
    struct ff_point* end;
    struct ff_point* center;
};

// -- ELEMENT -- //

enum ff_element_type {
    FF_POINT,
    FF_LINE,
    FF_CIRCLE,
    FF_ARC
};


typedef struct ff_element {

    enum ff_element_type type;

    union {
        struct ff_point point;
        struct ff_line line;
        struct ff_circle circle;
        struct ff_arc arc;
    };
} ff_element;

static inline struct ff_vec2 ffPoint_getPos(struct ff_point point) {
    return (struct ff_vec2) {point.x->v, point.y->v};
}

static inline void ffParameter_set(struct ff_parameter* param, double v) {
    param->v = v;
}

static inline void ffPoint_setPos(struct ff_point* point, struct ff_vec2 position) {
    ffParameter_set(point->x, position.x);
    ffParameter_set(point->y, position.y);
}



// -- CONSTRAINT DEFINITION UTILS -- //

#define TOKEN_TO_ARG(prefix, suffix, token) prefix##token##suffix
#define PLACE_COMMA_1 ,
#define PLACE_COMMA_0
#define PLACE_COMMA(condition) PLACE_COMMA_##condition

#define EXPAND_ARGS(prefix, suffix, comma, ...) EXPAND_ARGS_HELPER(prefix, suffix, comma, __VA_ARGS__, 5, 4, 3, 2, 1)
#define EXPAND_ARGS_HELPER(prefix, suffix, comma, _1, _2, _3, _4, _5, N, ...) EXPAND_ARGS_##N(prefix, suffix, comma, _1, _2, _3, _4, _5)
#define EXPAND_ARGS_1(prefix, suffix, comma, arg1, ...) \
TOKEN_TO_ARG(prefix, suffix, arg1)

#define EXPAND_ARGS_2(prefix, suffix, comma, arg1, arg2, ...) \
TOKEN_TO_ARG(prefix, suffix, arg1) PLACE_COMMA(comma)           \
TOKEN_TO_ARG(prefix, suffix, arg2)

#define EXPAND_ARGS_3(prefix, suffix, comma, arg1, arg2, arg3, ...) \
TOKEN_TO_ARG(prefix, suffix, arg1) PLACE_COMMA(comma) \
TOKEN_TO_ARG(prefix, suffix, arg2) PLACE_COMMA(comma) \
TOKEN_TO_ARG(prefix, suffix, arg3)

#define EXPAND_ARGS_4(prefix, suffix, comma, arg1, arg2, arg3, arg4, ...) \
TOKEN_TO_ARG(prefix, suffix, arg1) PLACE_COMMA(comma) \
TOKEN_TO_ARG(prefix, suffix, arg2) PLACE_COMMA(comma) \
TOKEN_TO_ARG(prefix, suffix, arg3) PLACE_COMMA(comma) \
TOKEN_TO_ARG(prefix, suffix, arg4)

#define EXPAND_ARGS_5(prefix, suffix, comma, arg1, arg2, arg3, arg4, arg5, ...) \
TOKEN_TO_ARG(prefix, suffix, arg1) PLACE_COMMA(comma) \
TOKEN_TO_ARG(prefix, suffix, arg2) PLACE_COMMA(comma) \
TOKEN_TO_ARG(prefix, suffix, arg3) PLACE_COMMA(comma) \
TOKEN_TO_ARG(prefix, suffix, arg4) PLACE_COMMA(comma) \
TOKEN_TO_ARG(prefix, suffix, arg5)

//TODO undef all of the aboev later
//TODO is there a better way to do this?

enum ff_constraint_type;

typedef struct ff_constraint {

    enum ff_constraint_type type;

   // Expression** expressions;
    struct Jacobian_Matrix_Row* matrix_rows;
    int eq_len;

    //Point
    #define ffConsAttr_P1 struct ff_point* P1
    #define ffConsAttr_P1_ATCH cons.P1 = P1; 
    ffConsAttr_P1;

    #define ffConsAttr_P2 struct ff_point* P2
    #define ffConsAttr_P2_ATCH cons.P2 = P2; 
    ffConsAttr_P2;

    #define ffConsAttr_P3 struct ff_point* P3
    #define ffConsAttr_P3_ATCH cons.P3 = P3; 
    ffConsAttr_P3;

    //Line
    #define ffConsAttr_L1 struct ff_line* L1
    #define ffConsAttr_L1_ATCH cons.L1 = L1; 
    ffConsAttr_L1;

    #define ffConsAttr_L2 struct ff_line* L2
    #define ffConsAttr_L2_ATCH cons.L2 = L2; 
    ffConsAttr_L2;

    #define ffConsAttr_L3 struct ff_line* L3
    #define ffConsAttr_L3_ATCH cons.L3 = L3; 
    ffConsAttr_L3;

    //Circle
    #define ffConsAttr_C1 struct ff_circle* C1
    #define ffConsAttr_C1_ATCH cons.C1 = C1; 
    ffConsAttr_C1;

    #define ffConsAttr_C2 struct ff_circle* C2
    #define ffConsAttr_C2_ATCH cons.C2 = C2; 
    ffConsAttr_C2;

    #define ffConsAttr_C3 struct ff_circle* C3
    #define ffConsAttr_C3_ATCH cons.C3 = C3; 
    ffConsAttr_C3;

    //Arcs
    #define ffConsAttr_A1 struct ff_arc* A1
    #define ffConsAttr_A1_ATCH cons.A1 = A1; 
    ffConsAttr_A1;

    #define ffConsAttr_A2 struct ff_arc* A2
    #define ffConsAttr_A2_ATCH cons.A2 = A2; 
    ffConsAttr_A2;

    #define ffConsAttr_A3 struct ff_arc* A3
    #define ffConsAttr_A3_ATCH cons.A3 = A3; 
    ffConsAttr_A3;

    //Params
    #define ffConsAttr_N1 struct ff_parameter* N1
    #define ffConsAttr_N1_ATCH cons.N1 = N1; 
    ffConsAttr_N1;

    #define ffConsAttr_N2 struct ff_parameter* N2
    #define ffConsAttr_N2_ATCH cons.N2 = N2; 
    ffConsAttr_N2;

    #define ffConsAttr_N3 struct ff_parameter* N3
    #define ffConsAttr_N3_ATCH cons.N3 = N3; 
    ffConsAttr_N3;

} ff_constraint;

typedef struct ff_constraint_build_data {
    ff_constraint cons;
    struct Jacobian_Matrix_Row* jacob_rows;
    int jacob_rows_len;
} ff_constraint_build_data;


static inline void init_constraint(ff_constraint_build_data* cons_data, int eq_count) {
    cons_data->jacob_rows = malloc(sizeof(struct Jacobian_Matrix_Row) * eq_count);
    cons_data->jacob_rows_len = eq_count;
    for (int eq_idx = 0; eq_idx < eq_count; eq_idx++) {
        struct Jacobian_Matrix_Row* row = &cons_data->jacob_rows[eq_idx];
        row->derivatives = NULL;
        row->dervs_value = NULL;
    }
    int i = 0;
}

#define FFCONS_BEGIN_EQUATION_DEF(NAME, EQUATION_COUNT)                                 \
    static inline void FFCONSEQUATIONS_##NAME (ff_constraint_build_data* cons_data) {   \
        init_constraint(cons_data, EQUATION_COUNT);                                     \
        struct ff_constraint* cons = &cons_data->cons;                                         \
        int i = 0;                                                                      \

        

#define FFCONS_ADD_EQUATION(eq) \
    cons_data->jacob_rows[i].equation = eq;  \
    i++;                        \
        
    
#define FFCONS_END_EQUATION_DEF                                                     \
        if (i != cons_data->jacob_rows_len) FREEFORM_PRINT_ERR("ERROR: TODO@");     \
    }                                                                               \



// -- CONSTRAINT DEFINITIONS -- //

#define FF_CONSTRAINT_LIST                      \
    X(POINT_ON_POINT, P1, P2)                   \
    X(HORIZONTAL, P1, P2)                       \
    X(VERTICAL, P1, P2)                         \
                                                \
    X(POINT_ON_LINE, P1, L1)                    \
    X(POINT_ON_CIRCLE, P1, C1)                  \
                                                \
    X(LINE_TANG_CIRCLE, L1, C1)                 \
                                                \
    X(PARALLEL, L1, L2)                         \
    X(PERPENDICULAR, L1, L2)                    \
    X(MIDPOINT, P1, P2, P3)                     \
    X(ANGLE, L1, L2, N1)                        \
                                                \
    X(POINT_TO_POINT_DIST, P1, P2, N1)          \



FFCONS_BEGIN_EQUATION_DEF(FF_CONSTRAINT_POINT_ON_POINT, 2)

        FFCONS_ADD_EQUATION(exprInit_op(
            OperatorType_SUB,
            exprInit_param(cons->P1->x),
            exprInit_param(cons->P2->x)
        ));
        FFCONS_ADD_EQUATION(exprInit_op(
            OperatorType_SUB,
            exprInit_param(cons->P1->y),
            exprInit_param(cons->P2->y)
        ));
        
        FFCONS_END_EQUATION_DEF

        //P1 /L1 
        //todo remove line noraml to circle in favor of this?
FFCONS_BEGIN_EQUATION_DEF(FF_CONSTRAINT_POINT_ON_LINE, 1)

    FFCONS_ADD_EQUATION(
        exprInit_op(OperatorType_SUB,
            exprInit_op(OperatorType_MUL, 
                exprInit_op(OperatorType_SUB,exprInit_param(cons->L1->p2->x),exprInit_param(cons->L1->p1->x)), 
                exprInit_op(OperatorType_SUB,exprInit_param(cons->P1->y),exprInit_param(cons->L1->p1->y))
            )
            , // MINUS 
            exprInit_op(OperatorType_MUL, 
                exprInit_op(OperatorType_SUB,exprInit_param(cons->L1->p2->y),exprInit_param(cons->L1->p1->y)), 
                exprInit_op(OperatorType_SUB,exprInit_param(cons->P1->x),exprInit_param(cons->L1->p1->x))
            )
        )
    );
    FFCONS_END_EQUATION_DEF


FFCONS_BEGIN_EQUATION_DEF(FF_CONSTRAINT_HORIZONTAL, 1)

        FFCONS_ADD_EQUATION(exprInit_op(
            OperatorType_SUB,
            exprInit_param(cons->P1->y),
            exprInit_param(cons->P2->y)
        ));

        FFCONS_END_EQUATION_DEF

FFCONS_BEGIN_EQUATION_DEF(FF_CONSTRAINT_VERTICAL, 1)

        FFCONS_ADD_EQUATION(exprInit_op(
            OperatorType_SUB,
            exprInit_param(cons->P1->x),
            exprInit_param(cons->P2->x)
        ));

        FFCONS_END_EQUATION_DEF

FFCONS_BEGIN_EQUATION_DEF(FF_CONSTRAINT_LINE_TANG_CIRCLE, 1)
    FFCONS_ADD_EQUATION(
        exprInit_op(OperatorType_SUB,
            exprInit_op(OperatorType_SQR,
                exprInit_op(OperatorType_SUB,
                    exprInit_op(OperatorType_MUL, 
                        exprInit_op(OperatorType_SUB,exprInit_param(cons->L1->p2->x),exprInit_param(cons->L1->p1->x)), 
                        exprInit_op(OperatorType_SUB,exprInit_param(cons->C1->center->y),exprInit_param(cons->L1->p1->y))
                    )
                    , // MINUS 
                    exprInit_op(OperatorType_MUL, 
                        exprInit_op(OperatorType_SUB,exprInit_param(cons->L1->p2->y),exprInit_param(cons->L1->p1->y)), 
                        exprInit_op(OperatorType_SUB,exprInit_param(cons->C1->center->x),exprInit_param(cons->L1->p1->x))
                    )
                ),NULL
            )
            , // MINUS
            exprInit_op(OperatorType_MUL,
                exprInit_op(OperatorType_SQR,exprInit_param(cons->C1->radius),NULL)
                ,
                exprInit_op(OperatorType_ADD,
                    exprInit_op(OperatorType_SQR,
                        exprInit_op(OperatorType_SUB,
                            exprInit_param(cons->L1->p2->x)
                            ,
                            exprInit_param(cons->L1->p1->x)
                        )
                    ,NULL)
                    , // PLUS
                    exprInit_op(OperatorType_SQR,
                        exprInit_op(OperatorType_SUB,
                            exprInit_param(cons->L1->p2->y)
                            ,
                            exprInit_param(cons->L1->p1->y)
                        )
                    ,NULL)
                )
            )
        )
    );
    FFCONS_END_EQUATION_DEF
    

    /*
FFCONS_BEGIN_EQUATION_DEF(FF_CONSTRAINT_LINE_NORM_CIRCLE, 1)

    FFCONS_ADD_EQUATION(
        exprInit_op(OperatorType_SUB,
            exprInit_op(OperatorType_MUL, 
                exprInit_op(OperatorType_SUB,exprInit_param(cons->L1->p2->x),exprInit_param(cons->L1->p1->x)), 
                exprInit_op(OperatorType_SUB,exprInit_param(cons->C1->center->y),exprInit_param(cons->L1->p1->y))
            )
            , // MINUS 
            exprInit_op(OperatorType_MUL, 
                exprInit_op(OperatorType_SUB,exprInit_param(cons->L1->p2->y),exprInit_param(cons->L1->p1->y)), 
                exprInit_op(OperatorType_SUB,exprInit_param(cons->C1->center->x),exprInit_param(cons->L1->p1->x))
            )
        )
    );
    FFCONS_END_EQUATION_DEF
    */
//todo@ make circle radius instaed of coinicdent point

// Difference of slopes
FFCONS_BEGIN_EQUATION_DEF(FF_CONSTRAINT_PARALLEL, 1)

    
    FFCONS_ADD_EQUATION(exprInit_op(
        OperatorType_SUB,
        
        exprInit_op(
            OperatorType_MUL,
            exprInit_op(
                OperatorType_SUB,
                exprInit_param(cons->L1->p2->y),
                exprInit_param(cons->L1->p1->y)
            ),
            exprInit_op(
                OperatorType_SUB,
                exprInit_param(cons->L2->p2->x),
                exprInit_param(cons->L2->p1->x)
            )
        )
        ,

        exprInit_op(
            OperatorType_MUL,
            exprInit_op(
                OperatorType_SUB,
                exprInit_param(cons->L2->p2->y),
                exprInit_param(cons->L2->p1->y)
            ),
            exprInit_op(
                OperatorType_SUB,
                exprInit_param(cons->L1->p2->x),
                exprInit_param(cons->L1->p1->x)
            )
        )

    ));

    FFCONS_END_EQUATION_DEF

FFCONS_BEGIN_EQUATION_DEF(FF_CONSTRAINT_PERPENDICULAR, 1)

    
    FFCONS_ADD_EQUATION(exprInit_op(
        OperatorType_ADD,
        
        exprInit_op(
            OperatorType_MUL,
            exprInit_op(
                OperatorType_SUB,
                exprInit_param(cons->L1->p2->y),
                exprInit_param(cons->L1->p1->y)
            ),
            exprInit_op(
                OperatorType_SUB,
                exprInit_param(cons->L2->p2->y),
                exprInit_param(cons->L2->p1->y)
            )
        )
        ,

        exprInit_op(
            OperatorType_MUL,
            exprInit_op(
                OperatorType_SUB,
                exprInit_param(cons->L1->p2->x),
                exprInit_param(cons->L1->p1->x)
            ),
            exprInit_op(
                OperatorType_SUB,
                exprInit_param(cons->L2->p2->x),
                exprInit_param(cons->L2->p1->x)
            )
        )

    ));

    FFCONS_END_EQUATION_DEF


FFCONS_BEGIN_EQUATION_DEF(FF_CONSTRAINT_MIDPOINT, 2)


    FFCONS_ADD_EQUATION(exprInit_op(
        OperatorType_SUB,
        exprInit_param(cons->P2->x), // x_m
        exprInit_op(
            OperatorType_DIV,
            exprInit_op(
                OperatorType_ADD,
                exprInit_param(cons->P1->x), // x_a
                exprInit_param(cons->P3->x)  // x_b
            ),
            exprInit_const(2.0)
        )
    ));
    

    FFCONS_ADD_EQUATION(exprInit_op(
        OperatorType_SUB,
        exprInit_param(cons->P2->y), // x_m
        exprInit_op(
            OperatorType_DIV,
            exprInit_op(
                OperatorType_ADD,
                exprInit_param(cons->P1->y), // x_a
                exprInit_param(cons->P3->y)  // x_b
            ),
            exprInit_const(2.0)
        )
    ));

    FFCONS_END_EQUATION_DEF


FFCONS_BEGIN_EQUATION_DEF(FF_CONSTRAINT_POINT_TO_POINT_DIST, 1)

    FFCONS_ADD_EQUATION(exprInit_op(OperatorType_SUB,
        exprInit_op(OperatorType_ADD, 
            exprInit_op(OperatorType_SQR, 
                exprInit_op(OperatorType_SUB, exprInit_param(cons->P2->x), exprInit_param(cons->P1->x))
                ,NULL)
            , 
            exprInit_op(OperatorType_SQR,  
                exprInit_op(OperatorType_SUB, exprInit_param(cons->P2->y), exprInit_param(cons->P1->y))
                ,NULL)
        ), exprInit_op(OperatorType_SQR, exprInit_param(cons->N1), NULL)
    ));

    FFCONS_END_EQUATION_DEF

FFCONS_BEGIN_EQUATION_DEF(FF_CONSTRAINT_ANGLE, 1)

    FFCONS_ADD_EQUATION(exprInit_op(
        OperatorType_SUB,
        exprInit_op(OperatorType_ACOS,
            exprInit_op(OperatorType_DIV,
                exprInit_op(OperatorType_ADD,
                    exprInit_op(OperatorType_MUL,
                        exprInit_op(OperatorType_SUB, exprInit_param(cons->L1->p2->x), exprInit_param(cons->L1->p1->x)),
                        exprInit_op(OperatorType_SUB, exprInit_param(cons->L2->p2->x), exprInit_param(cons->L2->p1->x))
                    ),
                    exprInit_op(OperatorType_MUL,
                        exprInit_op(OperatorType_SUB, exprInit_param(cons->L1->p2->y), exprInit_param(cons->L1->p1->y)),
                        exprInit_op(OperatorType_SUB, exprInit_param(cons->L2->p2->y), exprInit_param(cons->L2->p1->y))
                    )
                ),
                exprInit_op(OperatorType_MUL,
                    exprInit_op(OperatorType_SQRT,
                        exprInit_op(OperatorType_ADD,
                            exprInit_op(OperatorType_SQR, exprInit_op(OperatorType_SUB, exprInit_param(cons->L1->p2->x), exprInit_param(cons->L1->p1->x)), NULL),
                            exprInit_op(OperatorType_SQR, exprInit_op(OperatorType_SUB, exprInit_param(cons->L1->p2->y), exprInit_param(cons->L1->p1->y)), NULL)
                        ),
                        NULL
                    ),
                    exprInit_op(OperatorType_SQRT,
                        exprInit_op(OperatorType_ADD,
                            exprInit_op(OperatorType_SQR, exprInit_op(OperatorType_SUB, exprInit_param(cons->L2->p2->x), exprInit_param(cons->L2->p1->x)), NULL),
                            exprInit_op(OperatorType_SQR, exprInit_op(OperatorType_SUB, exprInit_param(cons->L2->p2->y), exprInit_param(cons->L2->p1->y)), NULL)
                        ),
                        NULL
                    )
                )
            )
        ,NULL),
        exprInit_param(cons->N1)
 
    ));

    FFCONS_END_EQUATION_DEF


// -- END CONSTRAINT DEFINITIONS -- //

#define X(NAME, ...) FF_CONSTRAINT_##NAME,
typedef enum {
    FF_CONSTRAINT_LIST

    FF_CONSTRAINT_TYPE_COUNT
} ff_constraint_type;

#undef X
#define X(NAME, ...)                                                                                                \
    static inline ff_constraint_build_data ffConstraint_init_##NAME(EXPAND_ARGS(ffConsAttr_, , 1, __VA_ARGS__)) {   \
                                                                                                                    \
        ff_constraint_build_data result;                                                                            \
        struct ff_constraint cons;                                                                                  \
                                                                                                                    \
        EXPAND_ARGS(ffConsAttr_, _ATCH, 0, __VA_ARGS__);                                                            \
        result.cons = cons;                                                                                         \
        result.cons.type = FF_CONSTRAINT_##NAME;                                                                    \
        FFCONSEQUATIONS_FF_CONSTRAINT_##NAME(&result);                                                              \
        return result;                                                                                              \
    }                                                                                                               \

FF_CONSTRAINT_LIST
#undef X




// -- GENERAL -- //

#define PARAMMODE_DISABLED 0
#define PARAMMODE_DYNAMIC 1
#define PARAMMODE_FIXED 2

typedef struct ff_sketch {

    struct ff_element elements[FREEFORM_MAXIMUM_ELEMENTS];
    int* working_elements_idxs;
    int working_elements_len;
    
    struct ff_parameter parameters[FREEFORM_MAXIMUM_PARAMETERS];
    int parameters_count;
    
    int* dynamic_param_idxs;
    int dynamic_params_count;

    ff_constraint constraints[FREEFORM_MAXIMUM_CONSTRAINTS];
    int constraint_count;  

    //Solving data
    struct Jacobian_Matrix_Row* jacobian_matrix;
    int equation_count;

    double* normal_mat; //Normal Mat: 2D matrix of eq_len x eq_len
    double* intermediate_solution; //1D matrix of eq_len
    double* dynamic_parameter_updates; //1D matrix of param_len

} ff_sketch;

static inline int ffSketch_getAttr_element_count(ff_sketch* sketch) {
    return sketch->working_elements_len;
}

static inline int ffSketch_getAttr_constraint_count(ff_sketch* sketch) {
    return sketch->constraint_count;
}

static inline int ffSketch_getAttr_equation_count(ff_sketch* sketch) {
    return sketch->equation_count;
}


typedef struct ff_sketch_drawing_config {
    void (*draw_point)  (struct ff_element* element);
    void (*draw_line)   (struct ff_element* element);
    void (*draw_circle) (struct ff_element* element);
    void (*draw_arc)    (struct ff_element* element);
} ff_sketch_drawing_config;


//GLOBALS

#define FF_PI 3.14159265358979323846
#define FF_TAU (2.0 * FF_PI)
#define FF_HALF_PI (FF_PI / 2.0)

#define FF_DBL_MAX DBL_MAX


// - MATH - //

struct ff_vec2 ffVec2_add(struct ff_vec2 a, struct ff_vec2 b);
struct ff_vec2 ffVec2_sub(struct ff_vec2 a, struct ff_vec2 b);
double ffVec2_distance_squared(struct ff_vec2 a, struct ff_vec2 b);
double ffVec2_distance(struct ff_vec2 a, struct ff_vec2 b);
double ffVec2_length_squared(struct ff_vec2 v);
double ffVec2_length(struct ff_vec2 v);
double ffMath_line_distance(struct ff_line line, struct ff_vec2 p);


//FUNCS
void ff_initialize_infastructure();

ff_sketch ffSketch_create();
void ffSketch_destroy(ff_sketch* sketch);

struct ff_parameter* ffSketch_add_parameter(ff_sketch* sketch, double value, bool affix);

struct ff_point* ffSketch_add_point(struct ff_sketch* sketch, struct ff_element** element_handle_out, struct ff_vec2 point);
struct ff_line* ffSketch_add_line(struct ff_sketch* sketch, struct ff_element** element_handle_out, struct ff_vec2 p1, struct ff_vec2 p2);
struct ff_circle* ffSketch_add_circle(struct ff_sketch* sketch, struct ff_element** element_handle_out, struct ff_vec2 center, double radius);
struct ff_arc* ffSketch_add_arc(struct ff_sketch* sketch, struct ff_element** element_handle_out, struct ff_vec2 start, struct ff_vec2 end, struct ff_vec2 center);

int ffSketch_add_constraint(ff_sketch* sketch, ff_constraint_build_data constraint_data);
int ffSketch_solve(ff_sketch* sketch, double tolerance, int maximum_steps);

//Editor utils

//todo@
double ffElement_distance_to(struct ff_element element, struct ff_vec2 point);
struct ff_element* ffSketch_closest_element(struct ff_sketch* sketch, struct ff_vec2 point, double point_override_radius, double* distance_out);
struct ff_element* ffSketch_closest_element_exc(struct ff_sketch* sketch, struct ff_vec2 point, double point_override_radius, double* distance_out, struct ff_element* to_exclude);

void ffSketch_draw(ff_sketch* sketch, struct ff_sketch_drawing_config config);

void SOLVE_TEST();




