# Dynamic Constraint System - Implementation Summary

## What Was Added

We've extended the freeform library with a **dynamic constraint system** that allows constraint expressions to reference entities and parameters by array index, making constraints reusable and modifiable at runtime.

## Changes to freeform.h

### 1. New Operator Types

Added to `ff_OperatorType` enum:
- `OperatorType_PARAM_IDX` - References `constraint.pars[idx]`
- `OperatorType_ENTITY_IDX` - References `constraint.ents[idx]`
- `OperatorType_POINT_X` - Gets X parameter from point at `ents[idx]`
- `OperatorType_POINT_Y` - Gets Y parameter from point at `ents[idx]`
- `OperatorType_CIRCLE_R` - Gets radius from circle at `ents[idx]`
- `OperatorType_CIRCLE_C` - Gets center entity from circle at `ents[idx]`
- `OperatorType_LINE_P1` - Gets first point from line at `ents[idx]`
- `OperatorType_LINE_P2` - Gets second point from line at `ents[idx]`

### 2. Updated ff_Expr Structure

Changed from individual fields to a union:
```c
typedef struct ff_Expr {
    ff_OperatorType op_type;
    struct ff_Expr* a;
    struct ff_Expr* b;
    union {
        ff_float value;         // For OperatorType_CONST
        ff_ParamHandle param_H; // For OperatorType_PARAM
        uint16_t param_idx;     // For OperatorType_PARAM_IDX
        uint16_t entity_idx;    // For OperatorType_ENTITY_IDX, POINT_X, etc.
    };
} ff_Expr;
```

### 3. Updated ff_ConstraintDef Structure

Added array length tracking:
```c
typedef struct ff_ConstraintDef {
    ff_Expr* eq;
    enum ff_ConstraintType type;
    ff_EntityHandle ents[FFCONS_MAXENT];
    ff_ParamHandle pars[FFCONS_MAXPAR];
    uint16_t ent_count;  // NEW: Number of entities
    uint16_t par_count;  // NEW: Number of parameters
} ff_ConstraintDef;
```

### 4. New API Functions

#### Expression Creation
```c
// Create indexed parameter reference
FF_API ff_Expr* exprInit_param_idx(uint16_t idx);

// Create indexed entity reference
FF_API ff_Expr* exprInit_entity_idx(uint16_t idx);

// Entity field accessors
FF_API ff_Expr* exprInit_point_x(uint16_t entity_idx);
FF_API ff_Expr* exprInit_point_y(uint16_t entity_idx);
FF_API ff_Expr* exprInit_circle_radius(uint16_t entity_idx);
FF_API ff_Expr* exprInit_circle_center(uint16_t entity_idx);
```

#### Expression Evaluation
```c
// Evaluate with constraint context (for indexed expressions)
FF_API ff_float expr_evaluate_constraint(
    ff_Expr* expr,
    const ff_Constraint* constraint,
    const ff_Sketch* sketch
);
```

## Implementation Requirements

To complete this system, you need to implement the following in `freeform_impl.c`:

### 1. Expression Initialization Functions

```c
ff_Expr* exprInit_param_idx(uint16_t idx) {
    ff_Expr* e = malloc(sizeof(ff_Expr));
    e->op_type = OperatorType_PARAM_IDX;
    e->param_idx = idx;
    e->a = NULL;
    e->b = NULL;
    return e;
}

ff_Expr* exprInit_entity_idx(uint16_t idx) {
    ff_Expr* e = malloc(sizeof(ff_Expr));
    e->op_type = OperatorType_ENTITY_IDX;
    e->entity_idx = idx;
    e->a = NULL;
    e->b = NULL;
    return e;
}

ff_Expr* exprInit_point_x(uint16_t entity_idx) {
    ff_Expr* e = malloc(sizeof(ff_Expr));
    e->op_type = OperatorType_POINT_X;
    e->entity_idx = entity_idx;
    e->a = NULL;
    e->b = NULL;
    return e;
}

// Similar for point_y, circle_radius, circle_center...
```

### 2. Expression Evaluation with Context

```c
ff_float expr_evaluate_constraint(ff_Expr* expr,
                                  const ff_Constraint* constraint,
                                  const ff_Sketch* sketch) {
    if (!expr) return 0.0;

    switch (expr->op_type) {
        case OperatorType_CONST:
            return expr->value;

        case OperatorType_PARAM_IDX: {
            if (expr->param_idx >= constraint->def.par_count) return 0.0;
            ff_ParamHandle ph = constraint->def.pars[expr->param_idx];
            ff_Parameter* p = ffSketch_GetParameter((ff_Sketch*)sketch, ph);
            return p ? p->def.v : 0.0;
        }

        case OperatorType_POINT_X: {
            if (expr->entity_idx >= constraint->def.ent_count) return 0.0;
            ff_EntityHandle eh = constraint->def.ents[expr->entity_idx];
            ff_Entity* e = ffSketch_GetEntity((ff_Sketch*)sketch, eh);
            if (!e || e->def.type != FF_POINT) return 0.0;
            ff_Parameter* p = ffSketch_GetParameter((ff_Sketch*)sketch, e->def.data.point.x);
            return p ? p->def.v : 0.0;
        }

        case OperatorType_POINT_Y: {
            if (expr->entity_idx >= constraint->def.ent_count) return 0.0;
            ff_EntityHandle eh = constraint->def.ents[expr->entity_idx];
            ff_Entity* e = ffSketch_GetEntity((ff_Sketch*)sketch, eh);
            if (!e || e->def.type != FF_POINT) return 0.0;
            ff_Parameter* p = ffSketch_GetParameter((ff_Sketch*)sketch, e->def.data.point.y);
            return p ? p->def.v : 0.0;
        }

        case OperatorType_ADD: {
            ff_float a = expr_evaluate_constraint(expr->a, constraint, sketch);
            ff_float b = expr_evaluate_constraint(expr->b, constraint, sketch);
            return a + b;
        }

        // ... handle all other operators ...
    }
    return 0.0;
}
```

### 3. Symbolic Differentiation for Indexed Types

Update `expr_derivative()` to handle new operator types:

```c
ff_Expr* expr_derivative(ff_Expr* expr,
                        ff_ParamHandle indp_param,
                        bool protect_params) {
    if (!expr) return exprInit_const(0.0);

    switch (expr->op_type) {
        case OperatorType_PARAM_IDX:
            // Derivative of indexed param: need to check if it matches indp_param
            // This is tricky - you need the constraint context to resolve the handle
            // For now, return 0 (conservative - means no dependency)
            return exprInit_const(0.0);

        case OperatorType_POINT_X:
        case OperatorType_POINT_Y:
            // These resolve to parameters, but we don't know which at compile time
            // Return 0 for safety, or implement runtime checking
            return exprInit_const(0.0);

        // ... existing derivative rules for other operators ...
    }
}
```

**Note:** Symbolic differentiation with indexed expressions is complex because you don't know which parameter is referenced until evaluation time. Options:
1. **Conservative approach**: Return 0 derivative (assume no dependency)
2. **Runtime checking**: Check during derivative evaluation if indexed param matches
3. **Numerical derivatives**: Use finite differences instead of symbolic

## Usage Pattern

```c
// 1. Build expression tree ONCE (uses indices, not handles)
static ff_Expr* distance_expr = NULL;
if (!distance_expr) {
    distance_expr = build_distance_constraint();
}

// 2. Create constraint with specific entities/parameters
ff_ConstraintDef def = {0};
def.type = FF_GENERAL;
def.eq = distance_expr;  // Reuse same expression
def.ents[0] = point_a;
def.ents[1] = point_b;
def.ent_count = 2;
def.pars[0] = distance_param;
def.par_count = 1;

// 3. Add to sketch
ff_ConstraintHandle h = ffSketch_AddConstraint(sketch, def);

// 4. Later: modify constraint targets
ff_Constraint* c = ffSketch_GetConstraint(sketch, h);
c->def.ents[0] = new_point_a;  // Change which points are constrained
c->def.ents[1] = new_point_b;
// Expression tree unchanged - just references different entities now!

// 5. Solve
ffSketch_Solve(sketch, 0.01, 8);
```

## Benefits

1. **Reusable constraints**: Build expression once, apply to many entity pairs
2. **Dynamic retargeting**: Change constraint subjects without rebuilding expressions
3. **Cleaner code**: Constraint logic separated from specific instances
4. **Easier constraint libraries**: Store common constraint patterns as templates

## Limitations

1. **Symbolic differentiation complexity**: Indexed refs make compile-time derivatives harder
2. **Runtime overhead**: Extra indirection when resolving indices
3. **Type safety**: Need runtime checks that entity types match operator expectations
4. **Debugging**: Harder to trace which parameter an indexed ref actually resolves to

## Next Steps

To fully implement this system:
1. Implement the stub functions in `freeform_impl.c`
2. Update `expr_evaluate()` to call `expr_evaluate_constraint()` when constraint context available
3. Decide on derivative strategy (conservative, runtime, or numerical)
4. Add runtime validation (check entity types match operators)
5. Write unit tests for constraint retargeting
