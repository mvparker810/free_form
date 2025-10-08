# free_form

free_form is a minimalist, bloat-free toolkit, written in C, to solve complex, constrained, parametric, 2D geometric systems. This tool supports 2D points, lines, circles, and circular arcs, along with a wide variety of constraints.

**Key Features:**
- Single-header library (STB-style)
- Written in pure C
- Minimal dependencies
- Extensible constraint system
- Symbolic differentiation for constraint solving

## Integration

```c
// In exactly one .c/.cpp file:
#define FF_FREEFORM_IMPLEMENTATION
#include "freeform.h"

// In all other translation units:
#include "freeform.h"
```

## Usage

### Basic Example

```c
#include "freeform.h"

// Create a sketch
ff_Sketch sketch;
ffSketch_Init(&sketch, 1024, 256, 128); // param capacity, entity capacity, constraint capacity

// Add two points
ff_EntityDef p1_def = { .type = FF_POINT, .data.point = {
    .x = ffSketch_AddParameter(&sketch, (ff_ParameterDef){ .v = 0.0 }),
    .y = ffSketch_AddParameter(&sketch, (ff_ParameterDef){ .v = 0.0 })
}};
ff_EntityHandle p1 = ffSketch_AddEntity(&sketch, p1_def);

ff_EntityDef p2_def = { .type = FF_POINT, .data.point = {
    .x = ffSketch_AddParameter(&sketch, (ff_ParameterDef){ .v = 10.0 }),
    .y = ffSketch_AddParameter(&sketch, (ff_ParameterDef){ .v = 0.0 })
}};
ff_EntityHandle p2 = ffSketch_AddEntity(&sketch, p2_def);

// Add a line between the points
ff_EntityDef line_def = { .type = FF_LINE, .data.line = { .p1 = p1, .p2 = p2 }};
ff_EntityHandle line = ffSketch_AddEntity(&sketch, line_def);

// Solve the sketch
ffSketch_Solve(&sketch, 0.01, 8);

// Clean up
ffSketch_Free(&sketch);
```

### 2D Entities

free_form supports four types of geometric entities:

| Entity | Description | Data |
|--------|-------------|------|
| **Point** | 2D point (x, y) | `x`, `y` (parameter handles) |
| **Line** | Line segment between two points | `p1`, `p2` (entity handles) |
| **Circle** | Circle with center point and radius | `c` (center entity), `r` (radius parameter) |
| **Arc** | Circular arc defined by three points | `p1`, `p2`, `p3` (entity handles) |

### Parameters

All numeric values (coordinates, radii, etc.) are stored as **parameters**. Parameters are:
- Stored as `ff_float` (double precision)
- Referenced by `ff_ParamHandle`
- Modified by the constraint solver

### Constraints

Constraints define relationships between entities and parameters. The solver adjusts parameter values to satisfy all constraints.

**Built-in Constraint Types:**
- `FF_GENERAL` - Custom constraint equation (using expression tree)
- `FF_HORIZONTAL` - Makes a line horizontal

### Expression System

For `FF_GENERAL` constraints, you can build custom constraint equations using the expression tree system:

**Supported Operations:**
- Constants: `OperatorType_CONST`
- Parameter references: `OperatorType_PARAM`
- Arithmetic: `ADD`, `SUB`, `MUL`, `DIV`
- Trigonometry: `SIN`, `COS`, `ASIN`, `ACOS`
- Math: `SQRT`, `SQR`

The solver automatically computes symbolic derivatives for constraint equations.

## API Reference

### Sketch Management
```c
void ffSketch_Init(ff_Sketch* skt, uint16_t p_cap, uint16_t e_cap, uint16_t c_cap);
void ffSketch_Free(ff_Sketch* skt);
bool ffSketch_Solve(ff_Sketch* skt, double tolerance, uint32_t max_steps);
```

### Adding Elements
```c
ff_ParamHandle      ffSketch_AddParameter(ff_Sketch* skt, const ff_ParameterDef p_def);
ff_EntityHandle     ffSketch_AddEntity(ff_Sketch* skt, const ff_EntityDef e_def);
ff_ConstraintHandle ffSketch_AddConstraint(ff_Sketch* skt, const ff_ConstraintDef c_def);
```

### Deleting Elements
```c
bool ffSketch_DeleteParameter(ff_Sketch* skt, ff_ParamHandle h);
bool ffSketch_DeleteEntity(ff_Sketch* skt, ff_EntityHandle h);
bool ffSketch_DeleteConstraint(ff_Sketch* skt, ff_ConstraintHandle h);
```

### Accessing Elements
```c
ff_Parameter*  ffSketch_GetParameter(ff_Sketch* skt, ff_ParamHandle h);
ff_Entity*     ffSketch_GetEntity(ff_Sketch* skt, ff_EntityHandle h);
ff_Constraint* ffSketch_GetConstraint(ff_Sketch* skt, ff_ConstraintHandle h);
```

## Creating Custom Constraints

To add a new constraint type:

**Step 1:** Add your constraint to the configuration macro in `freeform.h`:
```c
#define FF_CONSTRAINTS_CFG      \
    CONS(GENERAL)               \
    CONS(HORIZONTAL)            \
    CONS(YOUR_NEW_CONSTRAINT)   // Add here
```

**Step 2:** Implement the constraint logic in `freeform_impl.c` (look for constraint evaluation functions)

**Step 3:** Create helper functions to build the constraint expression tree

## License

todo

## Credits

todo
