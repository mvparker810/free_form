#define FF_FREEFORM_IMPL_
#include "freeform.h"



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
