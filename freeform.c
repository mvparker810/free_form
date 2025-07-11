#include "freeform.h"

#include <stdbool.h>

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>

void FF_SOLVING_THROW_ERROR(const char* msg) {
    fprintf(stderr, "FreeForm Solver Error: %s\n", msg);
    assert(false);
}

void expr_free(Expression* expr) { //TODO handle extr_param better.
    if (expr->type == OperatorType_EXTR_PARAM) {
        free(expr);
        return;
    }
    if (expr->a) expr_free(expr->a);
    if (expr->b) expr_free(expr->b);

    free(expr);
}


Expression* exprInit_op(OperatorType type, Expression* a, Expression* b) {
    Expression* expr = malloc(sizeof(Expression));
    expr->type = type;
    expr->a = a;
    expr->b = b;
    return expr;
}

Expression* exprInit_const(double value) {
    Expression* expr = malloc(sizeof(Expression));
    expr->type = OperatorType_CONST;
    expr->value = value;
    expr->a = expr->b = NULL;
    expr->param_value = NULL;
    return expr;
}

Expression* exprInit_param(struct ff_parameter* param_value) {
    Expression* expr = malloc(sizeof(Expression));
    expr->type = OperatorType_PARAM;
    expr->param_value = param_value;
    expr->a = expr->b = NULL;
    return expr;
}

Expression* exprInit_external_param(Expression* expr) {
    Expression* new_expr = malloc(sizeof(Expression));
    new_expr->type = OperatorType_EXTR_PARAM;
    new_expr->a = expr;
    new_expr->b = NULL;
    new_expr->param_value = NULL;
    return new_expr;
}

// Evaluate the expression
double expr_evaluate(Expression* expr) {
    switch (expr->type) {
        case OperatorType_CONST: return expr->value;
        case OperatorType_PARAM: return expr->param_value->v;
        case OperatorType_EXTR_PARAM: return expr_evaluate(expr->a);
        case OperatorType_ADD: return expr_evaluate(expr->a) + expr_evaluate(expr->b);
        case OperatorType_SUB: return expr_evaluate(expr->a) - expr_evaluate(expr->b);
        case OperatorType_MUL: return expr_evaluate(expr->a) * expr_evaluate(expr->b);
        case OperatorType_DIV: return expr_evaluate(expr->a) / expr_evaluate(expr->b);
        case OperatorType_SIN: return sin(expr_evaluate(expr->a));
        case OperatorType_COS: return cos(expr_evaluate(expr->a));
        case OperatorType_ASIN: return asin(expr_evaluate(expr->a));
        case OperatorType_ACOS: return acos(expr_evaluate(expr->a));
        case OperatorType_SQRT: return sqrt(expr_evaluate(expr->a));
        case OperatorType_SQR: {
            double val = expr_evaluate(expr->a);
            return val * val;
        }
    }
    return 0.0;
}

#define TRY_EXTR_PARAM(expr) protect_params ? exprInit_external_param(expr) : expr

// Differentiate the expression with respect to a parameter
Expression* expr_derivative(Expression* expr, struct ff_parameter* param, bool protect_params) {
    switch (expr->type) {
        case OperatorType_CONST:
            return exprInit_const(0.0);
        case OperatorType_PARAM:
            return (expr->param_value == param) ? exprInit_const(1.0) : exprInit_const(0.0);
        case OperatorType_EXTR_PARAM:
            return expr_derivative(expr->a, param, protect_params);
        case OperatorType_ADD:
            return exprInit_op(OperatorType_ADD, expr_derivative(expr->a, param, protect_params), expr_derivative(expr->b, param, protect_params));
        case OperatorType_SUB:
            return exprInit_op(OperatorType_SUB, expr_derivative(expr->a, param, protect_params), expr_derivative(expr->b, param, protect_params));
        case OperatorType_MUL:
            return exprInit_op(OperatorType_ADD,
                exprInit_op(OperatorType_MUL, expr_derivative(expr->a, param, protect_params), TRY_EXTR_PARAM(expr->b)),
                exprInit_op(OperatorType_MUL, TRY_EXTR_PARAM(expr->a), expr_derivative(expr->b, param, protect_params))
            );
        case OperatorType_DIV:
            return exprInit_op(OperatorType_DIV,
                exprInit_op(OperatorType_SUB,
                    exprInit_op(OperatorType_MUL, expr_derivative(expr->a, param, protect_params), TRY_EXTR_PARAM(expr->b)),
                    exprInit_op(OperatorType_MUL, TRY_EXTR_PARAM(expr->a), expr_derivative(expr->b, param, protect_params))
                ),
                exprInit_op(OperatorType_MUL, TRY_EXTR_PARAM(expr->b), TRY_EXTR_PARAM(expr->b))
            );
        case OperatorType_SIN:
            return exprInit_op(OperatorType_MUL, expr_derivative(expr->a, param, protect_params), exprInit_op(OperatorType_COS, expr->a, NULL));
        case OperatorType_COS:
            return exprInit_op(OperatorType_MUL,
                exprInit_op(OperatorType_MUL, exprInit_const(-1.0),
                    exprInit_op(OperatorType_SIN, expr->a, NULL)),
                expr_derivative(expr->a, param, protect_params));    
        case OperatorType_ASIN:
            return exprInit_op(OperatorType_DIV, expr_derivative(expr->a, param, protect_params), exprInit_op(OperatorType_SQRT,
                exprInit_op(OperatorType_SUB, exprInit_const(1.0), exprInit_op(OperatorType_SQR, TRY_EXTR_PARAM(expr->a), NULL)), NULL));
        case OperatorType_ACOS:
            return exprInit_op(OperatorType_DIV, exprInit_op(OperatorType_MUL, exprInit_const(-1.0), expr_derivative(expr->a, param, protect_params)), exprInit_op(OperatorType_SQRT,
                exprInit_op(OperatorType_SUB, exprInit_const(1.0), exprInit_op(OperatorType_SQR, TRY_EXTR_PARAM(expr->a), NULL)), NULL));
        case OperatorType_SQRT:
            return exprInit_op(OperatorType_DIV, expr_derivative(expr->a, param, protect_params),
                exprInit_op(OperatorType_MUL, exprInit_const(2.0), exprInit_op(OperatorType_SQRT, TRY_EXTR_PARAM(expr->a), NULL)));
        case OperatorType_SQR:
            return exprInit_op(OperatorType_MUL,
                exprInit_const(2.0),
                exprInit_op(OperatorType_MUL, TRY_EXTR_PARAM(expr->a), expr_derivative(expr->a, param, protect_params))
            );
        default:  //unhandled op.
            FF_SOLVING_THROW_ERROR("Constraint undefined"); //TODO@ make this error msg actually say something meaningful
        return NULL;
         
    }

    return NULL;
}

//Returns if system is converged or not
bool ffSketch_calcError(ff_sketch* sketch, double tolerance) {
    bool converged = true;

    for (int r = 0; r < sketch->equation_count; r++) {
        struct Jacobian_Matrix_Row* row = &sketch->jacobian_matrix[r];
        row->error = expr_evaluate(row->equation);
        if (fabs(row->error) > tolerance) converged = false;    
    }

    return converged;
}

//actual solving. where the magic happens
int ffSketch_solve(ff_sketch* sketch, double tolerance, int maximum_steps) {

    //If we have no params or constraints, theres nothing to solve. Return early.
    if (sketch->dynamic_params_count == 0) return 1;
    if (sketch->constraint_count == 0) return 1;

    int converged = 0;

    double temp = 0.0;
    const double epsilon = 1e-10;

    int num_rows = sketch->equation_count;
    int num_cols = sketch->dynamic_params_count;

    for (int step = 0; step < maximum_steps; step++) {

        //Calculate error of system. If we are converged we are done.
        if (ffSketch_calcError(sketch, tolerance)) {
            converged = 1;
            break;
        }

        //Evaluate jacobian matrix
        for (int r = 0; r < num_rows; r++) {
            struct Jacobian_Matrix_Row* row = &sketch->jacobian_matrix[r];
            for (int c = 0; c < num_cols; c++) {
                row->dervs_value[c] = expr_evaluate(row->derivatives[c]);
            }
        }
          
        //Solve by least squares:

        //Start
        for (int r = 0; r < num_rows; r++) {
            for (int c = 0; c < num_rows; c++) {
                double sum = 0.0;
                for (int i = 0; i < num_cols; i++) {
                    double cV = sketch->jacobian_matrix[c].dervs_value[i];
                    double rV = sketch->jacobian_matrix[r].dervs_value[i];
                    if (cV == 0.0 || rV == 0.0) continue;    
                    sum += rV * cV;
                }
                sketch->normal_mat[r + c * num_rows] = sum;
            }
        }
        
        //Gaussian Solve
        for (int row = 0; row < num_rows; row++) {
            int pivot_row = row;
            double max_value = 0.0;
    
            // Find the pivot row (max absolute value in column)
            for (int candidate_row = row; candidate_row < num_rows; candidate_row++) {
                assert(candidate_row < num_rows); // Bounds check.
                if (fabs(sketch->normal_mat[candidate_row + row * num_rows]) > max_value) {
                    max_value = fabs(sketch->normal_mat[candidate_row + row * num_rows]);
                    pivot_row = candidate_row;
                }
            }
    
            if (max_value < epsilon) {
                fprintf(stderr, "Small pivot element: %f at row %d\n", max_value, row);
                continue;
            }
    
            // Swap rows in the normal matrix
            for (int col = 0; col < num_rows; col++) {
                temp = sketch->normal_mat[row + col * num_rows];
                sketch->normal_mat[row + col * num_rows] = sketch->normal_mat[pivot_row + col * num_rows];
                sketch->normal_mat[pivot_row + col * num_rows] = temp;
            }
    
            // Swap elements in the constraint error vector
            temp = sketch->jacobian_matrix[row].error;
            sketch->jacobian_matrix[row].error = sketch->jacobian_matrix[pivot_row].error;
            sketch->jacobian_matrix[pivot_row].error = temp;
    
            // Eliminate entries below the pivot
            for (int target_row = row + 1; target_row < num_rows; target_row++) {
                assert(target_row < num_rows); //Bounds check.
                if (fabs(sketch->normal_mat[row + row * num_rows]) < epsilon) {
                    fprintf(stderr, "Division by zero at row %d\n", row);
                    continue;
                }
                double coefficient = sketch->normal_mat[target_row + row * num_rows] / sketch->normal_mat[row + row * num_rows];
                for (int col = 0; col < num_rows; col++) {
                    sketch->normal_mat[target_row + col* num_rows] -= sketch->normal_mat[row + col* num_rows] * coefficient;
                }
                sketch->jacobian_matrix[target_row].error -= sketch->jacobian_matrix[row].error * coefficient;
            }
        }

   
        //Back sub
        for (int row = num_rows - 1; row >= 0; row--) {
            if (fabs(sketch->normal_mat[row + row * num_rows]) < epsilon) {
                fprintf(stderr, "Back substitution failed at row %d\n", row);
                continue;
            }
    
            double solution_value = sketch->jacobian_matrix[row].error / sketch->normal_mat[row + row * num_rows];
            for (int prev_row = num_rows - 1; prev_row > row; prev_row--) {
                solution_value -= sketch->intermediate_solution[prev_row] * sketch->normal_mat[row + prev_row * num_rows] / sketch->normal_mat[row + row * num_rows];
            }
            sketch->intermediate_solution[row] = solution_value;
        }

        //Tail
        for (int c = 0; c < num_cols; c++) {
            double sum = 0.0;
            for (int r = 0; r < num_rows; r++) {
                sum += sketch->intermediate_solution[r] * sketch->jacobian_matrix[r].dervs_value[c];
            }
            sketch->dynamic_parameter_updates[c] = sum;     

        }
 

        //Update dynamic parameters based on our calculated corrections.
        for (int i = 0; i < sketch->dynamic_params_count; i++) {
            sketch->parameters[sketch->dynamic_param_idxs[i]].v -= sketch->dynamic_parameter_updates[i];          
        }

    } //For step in maxsteps

    //End of solving process.
    //TODO@ revert params here when failed

    return converged;
}


// MATH

//todo make these MACROS?
struct ff_vec2 ffVec2_add(struct ff_vec2 a, struct ff_vec2 b) {
    return (struct ff_vec2) {a.x + b.x, a.y + b.y};
}

struct ff_vec2 ffVec2_sub(struct ff_vec2 a, struct ff_vec2 b) {
    return (struct ff_vec2) {a.x - b.x, a.y - b.y};
}

double ffVec2_distance_squared(struct ff_vec2 a, struct ff_vec2 b) {
    float h = (a.x - b.x);
    float k = (a.y - b.y);
    return (h * h) + (k * k);
}

double ffVec2_distance(struct ff_vec2 a, struct ff_vec2 b) {
    return sqrt(ffVec2_distance_squared(a,b));
}

double ffVec2_length_squared(struct ff_vec2 v) {
    return (v.x * v.x + v.y * v.y);
}

double ffVec2_length(struct ff_vec2 v) {
    return sqrt(ffVec2_length_squared(v));
}

double ffMath_line_distance(struct ff_line line, struct ff_vec2 p) {
    struct ff_vec2 a = ffPoint_getPos(*line.p1);
    struct ff_vec2 b = ffPoint_getPos(*line.p2);

    struct ff_vec2 ab = ffVec2_sub(b, a);
    struct ff_vec2 ap = ffVec2_sub(p, a);

    //TODO@ what forula is this. normalization?
    double dp = (ap.x * ab.x) + (ap.y * ab.y);

    if (dp <= 0.0) {
        return ffVec2_distance(a, p);
    }

    double length_sqrt = ffVec2_length_squared(ab);

    if (dp > length_sqrt) {
        return ffVec2_distance(b, p);
    }

    double sector = dp/length_sqrt;

    struct ff_vec2 closest_line_on_point = ffVec2_add(a, (struct ff_vec2) {sector * ab.x, sector*ab.y} );
    return ffVec2_distance(closest_line_on_point, p);
}

// FF_SKETCH

//TODO@ do i need this
void ff_initialize_infastructure() {
    
}

ff_sketch ffSketch_create() {

    ff_sketch sketch;

    for (int i = 0; i < FREEFORM_MAXIMUM_ELEMENTS; i++) {
        sketch.elements[i].type = -1;
    }
    //todo omit working_ ?
    sketch.working_elements_idxs = NULL;
    sketch.working_elements_len = 0;

    for (int i = 0; i < FREEFORM_MAXIMUM_PARAMETERS; i++) {

        struct ff_parameter* param = &sketch.parameters[i];

        param->status = PARAMMODE_DISABLED;
        param->v = 0.0;
    }
    sketch.parameters_count = 0;

    sketch.dynamic_param_idxs = NULL;
    sketch.dynamic_params_count = 0;

    sketch.constraint_count = 0;

    //Init solver data
    sketch.jacobian_matrix = NULL;
    sketch.equation_count = 0;
    sketch.normal_mat = NULL;
    sketch.intermediate_solution = NULL;
    sketch.dynamic_parameter_updates = NULL;

    return sketch;
}


void ffSketch_destroy(ff_sketch* sketch) {

    if (sketch->working_elements_idxs) {
        free(sketch->working_elements_idxs);
    }

}




//TODO@ make some sort of index map and refer to that?
struct ff_parameter* ffSketch_add_parameter(ff_sketch* sketch, double value, bool affix) {
    
    for (int i = 0; i < FREEFORM_MAXIMUM_PARAMETERS; i++) {

        struct ff_parameter* param = &sketch->parameters[i];
        
        if (!param->status) {

            param->status = affix ? PARAMMODE_FIXED : PARAMMODE_DYNAMIC;
            param->v = value;

            sketch->parameters_count++;

            if (!affix) {
                sketch->dynamic_params_count++;
                
                int dynamic_count = sketch->dynamic_params_count;

                sketch->dynamic_param_idxs = realloc(sketch->dynamic_param_idxs, sizeof(int) * dynamic_count);
                sketch->dynamic_param_idxs[dynamic_count - 1] = i;
                
                for (int j = 0; j < sketch->equation_count; j++) {
                    struct Jacobian_Matrix_Row* row = &sketch->jacobian_matrix[j];
                    row->derivatives = realloc(row->derivatives, sizeof(Expression*) * dynamic_count);
                    row->dervs_value = realloc(row->dervs_value, sizeof(double) * dynamic_count);

                    row->derivatives[dynamic_count - 1] = 
                    expr_derivative(row->equation, param, true);
                }

                sketch->dynamic_parameter_updates = realloc(sketch->dynamic_parameter_updates, sizeof(double) * dynamic_count);


            }  

            return param;
        } //If param is disbaled

    } //For all params in param map

    // SKETCH REACHED MAX COMPLEXITTTY
    //todo handle gracefully
    assert(false);
}

struct ff_element* ff_sketch_add_element(struct ff_sketch* sketch, struct ff_element element) {

    for (int i = 0; i < FREEFORM_MAXIMUM_ELEMENTS; i++) {

        if (sketch->elements[i].type < 0) {
            sketch->elements[i] = element;

            sketch->working_elements_idxs = realloc(sketch->working_elements_idxs, sizeof(int) * (sketch->working_elements_len + 1));
            sketch->working_elements_idxs[sketch->working_elements_len] = i;
            sketch->working_elements_len++;

            return &sketch->elements[i];
        }

    }

    // SKETCH REACHED MAX COMPLEXITTTY
    assert(false);
    return NULL;


}

struct ff_point* ffSketch_add_point(struct ff_sketch* sketch, struct ff_element** element_handle_out, struct ff_vec2 point) {

    struct ff_element config;
    config.type = FF_POINT;
    config.point.x = ffSketch_add_parameter(sketch, point.x, false);
    config.point.y = ffSketch_add_parameter(sketch, point.y, false);

    struct ff_element* element = ff_sketch_add_element(sketch, config);
    if (element_handle_out != NULL) *element_handle_out = element;

    return &element->point;
}

//todo make an explicit version of this, where points are provided

//TODO THIS IS IMPLICIT
struct ff_line* ffSketch_add_line(struct ff_sketch* sketch, struct ff_element** element_handle_out, struct ff_vec2 p1, struct ff_vec2 p2) {
 
    struct ff_point* p1_vert = ffSketch_add_point(sketch, NULL, p1);
    struct ff_point* p2_vert = ffSketch_add_point(sketch, NULL, p2);
    
    struct ff_element config;
    config.type = FF_LINE;
    config.line.p1 = p1_vert;
    config.line.p2 = p2_vert;

    struct ff_element* element = ff_sketch_add_element(sketch, config);
    if (element_handle_out != NULL) *element_handle_out = element;

    return &element->line;
}

struct ff_circle* ffSketch_add_circle(struct ff_sketch* sketch, struct ff_element** element_handle_out, struct ff_vec2 center, double radius) {

    struct ff_point* center_vert = ffSketch_add_point(sketch, NULL, center);
    struct ff_parameter* radius_val = ffSketch_add_parameter(sketch, radius, false);
    
    struct ff_element config;
    config.type = FF_CIRCLE;
    config.circle.center = center_vert;
    config.circle.radius = radius_val;

    struct ff_element* element = ff_sketch_add_element(sketch, config);
    if (element_handle_out != NULL) *element_handle_out = element;

    return &element->circle;

}

struct ff_arc* ffSketch_add_arc(struct ff_sketch* sketch, struct ff_element** element_handle_out, struct ff_vec2 start, struct ff_vec2 end, struct ff_vec2 center) {

    struct ff_point* start_vert = ffSketch_add_point(sketch, NULL, start);
    struct ff_point* end_vert = ffSketch_add_point(sketch, NULL, end);
    struct ff_point* center_vert = ffSketch_add_point(sketch, NULL, center);
    
    struct ff_element config;
    config.type = FF_ARC;
    config.arc.start = start_vert;
    config.arc.end = end_vert;
    config.arc.center = center_vert;

    struct ff_element* element = ff_sketch_add_element(sketch, config);
    if (element_handle_out != NULL) *element_handle_out = element;
    
    return &element->arc;
}

/*

void ffConstraint_init_jacob_rows(ff_sketch* sketch, ff_constraint* constraint) {

    int active_parameters_len = sketch->parameters_count;
    if (0 >= active_parameters_len) return;

    for (int j = 0; j < constraint->eq_len; j++) {
        struct Jacobian_Matrix_Row* row = &constraint->matrix_rows[j];
        row->derivatives = malloc(sizeof(Expression*) * active_parameters_len);
        row->dervs_value = malloc(sizeof(double) * active_parameters_len);
    }

    //constraint->matrix_rows = malloc(sizeof(Expression*) * active_parameters_len);

    int i = 0;
    for (int active_idx = 0; active_idx < sketch->parameters_count; active_idx++) {
        if (sketch->parameters[active_idx].status == PARAMMODE_DYNAMIC) {

            struct ff_parameter* wrt_param =  &sketch->parameters[active_idx];

            for (int j = 0; j < constraint->eq_len; j++) {
                struct Jacobian_Matrix_Row* row = &constraint->matrix_rows[j];
                row->derivatives[i] = expr_derivative(row->equation, wrt_param, true);
            }
            
            i++;
        }
    }

}
*/


int ffSketch_add_constraint(ff_sketch* sketch, ff_constraint_build_data constraint_data) {
    if (sketch->constraint_count >= FREEFORM_MAXIMUM_ELEMENTS) {
        return 0; // Error: Max constraints reached
        //todo handle me gracefully
    }

    //Look for the thingy being a duplicate
    //for (int i = 0; )

    ff_constraint* constraint_adr = &sketch->constraints[sketch->constraint_count++];

    *constraint_adr = constraint_data.cons;
    sketch->jacobian_matrix = realloc(sketch->jacobian_matrix,
    sizeof(struct Jacobian_Matrix_Row) * (sketch->equation_count + constraint_data.jacob_rows_len));
    
    for (int i = 0; i < constraint_data.jacob_rows_len; i++) {
        struct Jacobian_Matrix_Row new_row = constraint_data.jacob_rows[i];
        
        new_row.parent_constraint_adr = constraint_adr;

        new_row.derivatives = malloc(sizeof(Expression*) * sketch->dynamic_params_count);
        new_row.dervs_value = malloc(sizeof(double) * sketch->dynamic_params_count);

        for (int j = 0; j < sketch->dynamic_params_count; j++) {
            struct ff_parameter* wrt_param =  &sketch->parameters[sketch->dynamic_param_idxs[j]];
            new_row.derivatives[j] = expr_derivative(new_row.equation, wrt_param, true);
        }
   
        sketch->jacobian_matrix[sketch->equation_count + i] = new_row;
    }

    sketch->equation_count += constraint_data.jacob_rows_len;

    sketch->normal_mat = realloc(sketch->normal_mat, sizeof(double) * sketch->equation_count * sketch->equation_count);
    sketch->intermediate_solution = realloc(sketch->intermediate_solution, sizeof(double) * sketch->equation_count);
   
    free(constraint_data.jacob_rows);
    return 1;
}


// - EDITOR UTILS - //

double ffElement_distance_to(struct ff_element element, struct ff_vec2 point) {

    switch (element.type) {
        case FF_POINT:
            return ffVec2_distance(ffPoint_getPos(element.point), point);
        
        case FF_LINE:
            return ffMath_line_distance(element.line, point);
        break;
        case FF_CIRCLE:
            struct ff_vec2 center = ffPoint_getPos(*element.circle.center);
            double radius = element.circle.radius->v;
            double dist = (double)fabs(ffVec2_distance(point,center) - radius);

            return dist;
        break;
        case FF_ARC:
            
            break;
    }

    return -1;

}



struct ff_element* ffSketch_closest_element(struct ff_sketch* sketch, struct ff_vec2 point, double point_override_radius, double* distance_out) {

    struct ff_element* result = NULL;
    double toBeat = FF_DBL_MAX;

    for (int i = 0; i < sketch->working_elements_len; i++) {
        struct ff_element* element = &sketch->elements[sketch->working_elements_idxs[i]];

        double dist = ffElement_distance_to(*element, point);

        if (element->type != FF_POINT) {
            //continue;
            dist += point_override_radius;
        }
        
        if (dist < toBeat) {   
            toBeat = dist;
            result = element;
        }

    }
    *distance_out = toBeat;
    return result;
}


int ffElement_getSharedElement(struct ff_element* parent, struct ff_element* target) {
    
    if (parent == target) return 1;

    switch (parent->type) {
        case FF_POINT:
            if (parent == target) return 1;
        break;
        case FF_LINE:
            if (parent->line.p1 == &target->point) return 1;
            if (parent->line.p2 == &target->point) return 1;
        break;
        case FF_CIRCLE:
            if (parent->circle.center == &target->point) return 1;
        break;
        case FF_ARC:
            if (parent->arc.start == &target->point) return 1;
            if (parent->arc.end == &target->point) return 1;
            if (parent->arc.center == &target->point) return 1;
        break;
    }

    return 0;
}

struct ff_element* ffSketch_closest_element_exc(struct ff_sketch* sketch, struct ff_vec2 point, double point_override_radius, double* distance_out, struct ff_element* to_exclude) {

    struct ff_element* result = NULL;
    double toBeat = FF_DBL_MAX;

    for (int i = 0; i < sketch->working_elements_len; i++) {
        struct ff_element* element = &sketch->elements[sketch->working_elements_idxs[i]];

        if (ffElement_getSharedElement(element, to_exclude)) continue;

        double dist = ffElement_distance_to(*element, point);

        if (element->type != FF_POINT) {
            //continue;
            dist += point_override_radius;
        }
        
        if (dist < toBeat) {   
            toBeat = dist;
            result = element;
        }

    }
    *distance_out = toBeat;
    return result;
}




































void ffSketch_draw(ff_sketch* sketch, struct ff_sketch_drawing_config config) {

    for (int i = 0; i < sketch->working_elements_len; i++) {
        struct ff_element* element = &sketch->elements[sketch->working_elements_idxs[i]];

        switch (element->type) {
            case FF_POINT:
                config.draw_point(element);
            break;
            case FF_LINE:
                config.draw_line(element);
            break;
            case FF_CIRCLE:
                config.draw_circle(element);
            break;
            case FF_ARC:
                config.draw_arc(element);
            break;
        }
    }
  
}

