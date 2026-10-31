#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>

#define EPSILON 1e-9

typedef enum {
    STATUS_OK = 0,
    STATUS_ARG_ERROR = 1,
    STATUS_FILE_ERROR = 2,
    STATUS_MEMORY_ERROR = 3,
    STATUS_DIMENSION_ERROR = 4,
    STATUS_COMPUTE_ERROR = 5
} Status;

typedef struct {
    double *coords;
    int dimension;
} Vector;

typedef struct {
    Vector *vectors;
    int count;
} NormResult;

typedef struct {
    NormResult *results;
    int norm_count;
} AllNormResults;

typedef struct {
    const double *matrix;
    int matrix_dim;
} MatrixContext;

typedef double (*NormFunc)(const Vector *vec, const void *context);

double l_infinity_norm(const Vector *vec, const void *context) {
    (void)context;
    if (!vec || vec->dimension <= 0) return -1.0;
    
    double max_val = 0.0;
    for (int i = 0; i < vec->dimension; i++) {
        double abs_val = fabs(vec->coords[i]);
        if (abs_val > max_val) {
            max_val = abs_val;
        }
    }
    return max_val;
}

double lp_norm(const Vector *vec, const void *context) {
    if (!vec || vec->dimension <= 0 || !context) return -1.0;
    
    const double p = *(const double *)context;
    if (p <= 0.0) return -1.0;
    
    double sum = 0.0;
    for (int i = 0; i < vec->dimension; i++) {
        sum += pow(fabs(vec->coords[i]), p);
    }
    return pow(sum, 1.0 / p);
}

double matrix_norm(const Vector *vec, const void *context) {
    if (!vec || vec->dimension <= 0 || !context) return -1.0;
    
    const MatrixContext *const mc = (const MatrixContext *)context;
    if (!mc->matrix || mc->matrix_dim != vec->dimension) return -1.0;
    
    double ax_sum = 0.0;
    for (int i = 0; i < vec->dimension; i++) {
        double ax_i = 0.0;
        for (int j = 0; j < vec->dimension; j++) {
            ax_i += mc->matrix[i * vec->dimension + j] * vec->coords[j];
        }
        ax_sum += ax_i * vec->coords[i];
    }
    
    if (ax_sum < -EPSILON) return -1.0;
    return sqrt(ax_sum < 0.0 ? 0.0 : ax_sum);
}

AllNormResults find_longest_vectors(const int dimension, const int norm_count,
                                     const Vector *const all_vectors, const int vector_count,
                                     ...) {
    AllNormResults all_results = {NULL, 0};
    
    if (dimension <= 0 || norm_count <= 0 || vector_count <= 0 || !all_vectors) {
        return all_results;
    }
    
    all_results.results = (NormResult *)malloc(norm_count * sizeof(NormResult));
    if (!all_results.results) {
        return all_results;
    }
    
    for (int i = 0; i < norm_count; i++) {
        all_results.results[i].vectors = NULL;
        all_results.results[i].count = 0;
    }
    all_results.norm_count = norm_count;
    
    va_list args;
    va_start(args, vector_count);
    
    for (int n = 0; n < norm_count; n++) {
        const NormFunc norm_func = va_arg(args, NormFunc);
        const void *const context = va_arg(args, const void *);
        
        if (!norm_func) {
            continue;
        }
        
        double max_norm = -1.0;
        int max_count = 0;
        Vector *max_vectors = NULL;
        
        for (int v = 0; v < vector_count; v++) {
            const double norm_val = norm_func(&all_vectors[v], context);
            
            if (norm_val < 0.0) {
                continue;
            }
            
            if (max_norm < 0.0 || norm_val > max_norm + EPSILON) {
                if (max_vectors) free(max_vectors);
                max_norm = norm_val;
                max_count = 1;
                max_vectors = (Vector *)malloc(sizeof(Vector));
                if (!max_vectors) {
                    va_end(args);
                    for (int j = 0; j < norm_count; j++) {
                        free(all_results.results[j].vectors);
                    }
                    free(all_results.results);
                    all_results.results = NULL;
                    all_results.norm_count = 0;
                    return all_results;
                }
                max_vectors[0].coords = all_vectors[v].coords;
                max_vectors[0].dimension = all_vectors[v].dimension;
            } else if (fabs(norm_val - max_norm) < EPSILON) {
                Vector *const temp = (Vector *)realloc(max_vectors,
                                                       (max_count + 1) * sizeof(Vector));
                if (!temp) {
                    free(max_vectors);
                    va_end(args);
                    for (int j = 0; j < norm_count; j++) {
                        free(all_results.results[j].vectors);
                    }
                    free(all_results.results);
                    all_results.results = NULL;
                    all_results.norm_count = 0;
                    return all_results;
                }
                max_vectors = temp;
                max_vectors[max_count].coords = all_vectors[v].coords;
                max_vectors[max_count].dimension = all_vectors[v].dimension;
                max_count++;
            }
        }
        
        if (max_count > 0) {
            all_results.results[n].vectors = max_vectors;
            all_results.results[n].count = max_count;
        } else {
            free(max_vectors);
            all_results.results[n].vectors = NULL;
            all_results.results[n].count = 0;
        }
    }
    
    va_end(args);
    return all_results;
}

Status read_vectors_from_file(const char *const filename, Vector **const vectors,
                              int *const dimension, int *const out_count) {
    if (!filename || !vectors || !dimension || !out_count) {
        return STATUS_ARG_ERROR;
    }
    
    FILE *const file = fopen(filename, "r");
    if (!file) {
        return STATUS_FILE_ERROR;
    }
    
    if (fscanf(file, "%d", dimension) != 1 || *dimension <= 0) {
        fclose(file);
        return STATUS_DIMENSION_ERROR;
    }
    
    Vector *temp_vectors = NULL;
    int capacity = 10;
    int count = 0;
    
    temp_vectors = (Vector *)malloc(capacity * sizeof(Vector));
    if (!temp_vectors) {
        fclose(file);
        return STATUS_MEMORY_ERROR;
    }
    
    while (1) {
        Vector v;
        v.coords = (double *)malloc(*dimension * sizeof(double));
        if (!v.coords) {
            for (int i = 0; i < count; i++) {
                free(temp_vectors[i].coords);
            }
            free(temp_vectors);
            fclose(file);
            return STATUS_MEMORY_ERROR;
        }
        
        int read_count = 0;
        for (int i = 0; i < *dimension; i++) {
            if (fscanf(file, "%lf", &v.coords[i]) != 1) {
                break;
            }
            read_count++;
        }
        
        if (read_count != *dimension) {
            free(v.coords);
            break;
        }
        
        v.dimension = *dimension;
        
        if (count >= capacity) {
            capacity *= 2;
            Vector *const new_vectors = (Vector *)realloc(temp_vectors,
                                                          capacity * sizeof(Vector));
            if (!new_vectors) {
                free(v.coords);
                for (int i = 0; i < count; i++) {
                    free(temp_vectors[i].coords);
                }
                free(temp_vectors);
                fclose(file);
                return STATUS_MEMORY_ERROR;
            }
            temp_vectors = new_vectors;
        }
        
        temp_vectors[count] = v;
        count++;
    }
    
    fclose(file);
    
    if (count == 0) {
        free(temp_vectors);
        return STATUS_COMPUTE_ERROR;
    }
    
    *vectors = temp_vectors;
    *out_count = count;
    return STATUS_OK;
}

Status read_matrix_from_file(const char *const filename, double **const matrix,
                             int *const matrix_dim) {
    if (!filename || !matrix || !matrix_dim) {
        return STATUS_ARG_ERROR;
    }
    
    FILE *const file = fopen(filename, "r");
    if (!file) {
        *matrix = NULL;
        *matrix_dim = 0;
        return STATUS_FILE_ERROR;
    }
    
    if (fscanf(file, "%d", matrix_dim) != 1 || *matrix_dim <= 0) {
        fclose(file);
        *matrix = NULL;
        *matrix_dim = 0;
        return STATUS_DIMENSION_ERROR;
    }
    
    double *temp_matrix = (double *)malloc((*matrix_dim) * (*matrix_dim) * sizeof(double));
    if (!temp_matrix) {
        fclose(file);
        *matrix = NULL;
        *matrix_dim = 0;
        return STATUS_MEMORY_ERROR;
    }
    
    for (int i = 0; i < *matrix_dim; i++) {
        for (int j = 0; j < *matrix_dim; j++) {
            if (fscanf(file, "%lf", &temp_matrix[i * (*matrix_dim) + j]) != 1) {
                free(temp_matrix);
                fclose(file);
                *matrix = NULL;
                *matrix_dim = 0;
                return STATUS_COMPUTE_ERROR;
            }
        }
    }
    
    fclose(file);
    *matrix = temp_matrix;
    return STATUS_OK;
}

void free_all_results(AllNormResults *const results) {
    if (!results || !results->results) return;
    
    for (int i = 0; i < results->norm_count; i++) {
        free(results->results[i].vectors);
    }
    free(results->results);
    results->results = NULL;
    results->norm_count = 0;
}

void free_vectors(Vector *const vectors, const int count) {
    if (!vectors) return;
    for (int i = 0; i < count; i++) {
        free(vectors[i].coords);
    }
    free(vectors);
}

void print_error(const Status code)
{
    switch (code) {
        case STATUS_ARG_ERROR:
            fprintf(stderr, "Error: invalid arguments\n");
            break;
        case STATUS_FILE_ERROR:
            fprintf(stderr, "Error: file operation failed\n");
            break;
        case STATUS_MEMORY_ERROR:
            fprintf(stderr, "Error: memory allocation failed\n");
            break;
        case STATUS_DIMENSION_ERROR:
            fprintf(stderr, "Error: dimension error\n");
            break;
        case STATUS_COMPUTE_ERROR:
            fprintf(stderr, "Error: computation error\n");
            break;
        case STATUS_OK:
            break;
    }
}

int main(const int argc, char *const argv[])
{
    if (argc < 2) {
        print_error(STATUS_ARG_ERROR);
        return (int)STATUS_ARG_ERROR;
    }
    
    Vector *vectors = NULL;
    int dimension = 0;
    int vector_count = 0;
    const Status read_status = read_vectors_from_file(argv[1], &vectors, &dimension,
                                                      &vector_count);
    
    if (read_status != STATUS_OK) {
        print_error(read_status);
        return (int)read_status;
    }
    
    double *matrix = NULL;
    int matrix_dim = 0;
    const Status matrix_status = read_matrix_from_file("matrix.txt", &matrix, &matrix_dim);
    
    int norm_count = 2;
    
    if (matrix_status != STATUS_FILE_ERROR && matrix_status != STATUS_OK) {
        fprintf(stderr, "Warning: error reading matrix. Using only L-∞ and Lp norms.\n");
        norm_count = 2;
    } else if (matrix_status == STATUS_OK && matrix_dim != dimension) {
        fprintf(stderr, "Warning: matrix dimension %d != vector dimension %d. Using only L-∞ and Lp norms.\n",
                matrix_dim, dimension);
        free(matrix);
        matrix = NULL;
        matrix_dim = 0;
        norm_count = 2;
    } else if (matrix_status == STATUS_OK) {
        norm_count = 3;
    }
    
    const double p_value = 2.0;
    const MatrixContext matrix_context = {matrix, matrix_dim};
    
    AllNormResults results;
    if (norm_count == 3) {
        results = find_longest_vectors(
            dimension, 3, vectors, vector_count,
            &l_infinity_norm, NULL,
            &lp_norm, &p_value,
            &matrix_norm, &matrix_context
        );
    } else {
        results = find_longest_vectors(
            dimension, 2, vectors, vector_count,
            &l_infinity_norm, NULL,
            &lp_norm, &p_value
        );
    }
    
    if (!results.results) {
        print_error(STATUS_MEMORY_ERROR);
        free_vectors(vectors, vector_count);
        free(matrix);
        return (int)STATUS_MEMORY_ERROR;
    }
    
    const char *const norm_names[] = {"L-∞", "Lp (p=2)", "Matrix norm"};
    
    for (int n = 0; n < results.norm_count; n++) {
        if (results.results[n].count > 0) {
            printf("Norm '%s': %d vector(s)\n", norm_names[n], results.results[n].count);
            for (int i = 0; i < results.results[n].count; i++) {
                printf("  [");
                for (int j = 0; j < results.results[n].vectors[i].dimension; j++) {
                    printf("%.6f", results.results[n].vectors[i].coords[j]);
                    if (j < results.results[n].vectors[i].dimension - 1) printf(", ");
                }
                printf("]\n");
            }
        } else {
            printf("Norm '%s': vectors not found\n", norm_names[n]);
        }
    }
    
    free_all_results(&results);
    free_vectors(vectors, vector_count);
    free(matrix);
    
    return (int)STATUS_OK;
}