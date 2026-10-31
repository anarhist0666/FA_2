#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_INVALID_ARGS = 1,
    STATUS_FILE_ERROR = 2,
    STATUS_MEMORY_ERROR = 3,
    STATUS_INVALID_DEGREE = 4,
    STATUS_PARSE_ERROR = 5
} status_code;

void print_error(const status_code code)
{
    switch (code) {
        case STATUS_INVALID_ARGS:
            fprintf(stderr, "Error: invalid arguments\n");
            break;
        case STATUS_FILE_ERROR:
            fprintf(stderr, "Error: file operation failed\n");
            break;
        case STATUS_MEMORY_ERROR:
            fprintf(stderr, "Error: memory allocation failed\n");
            break;
        case STATUS_INVALID_DEGREE:
            fprintf(stderr, "Error: invalid degree\n");
            break;
        case STATUS_PARSE_ERROR:
            fprintf(stderr, "Error: parse error\n");
            break;
        case STATUS_OK:
            break;
    }
}

static void print_polynomial(FILE *const out_file, const char *const name, 
                           const double *const coeffs, const int degree, 
                           const int use_x_minus_a, const double a) {
    if (!out_file || !coeffs) {
        return;
    }

    fprintf(out_file, "%s(", name);
    
    if (use_x_minus_a) {
        fprintf(out_file, "x");
        if (a > 0) {
            fprintf(out_file, " - %.6f", a);
        } else if (a < 0) {
            fprintf(out_file, " + %.6f", -a);
        }
    } else {
        fprintf(out_file, "x");
    }
    
    fprintf(out_file, ") = ");

    int first = 1;
    for (int i = 0; i <= degree; i++) {
        if (fabs(coeffs[i]) < 1e-15) {
            continue;
        }

        if (!first) {
            if (coeffs[i] > 0) {
                fprintf(out_file, " + ");
            } else {
                fprintf(out_file, " - ");
            }
        } else if (coeffs[i] < 0) {
            fprintf(out_file, "-");
        }

        double abs_coeff = fabs(coeffs[i]);

        if (i == 0) {
            fprintf(out_file, "%.6f", abs_coeff);
        } else if (fabs(abs_coeff - 1.0) < 1e-15) {
            if (i == 1) {
                if (use_x_minus_a) {
                    if (a >= 0) {
                        fprintf(out_file, "(x - %.6f)", a);
                    } else {
                        fprintf(out_file, "(x + %.6f)", -a);
                    }
                } else {
                    fprintf(out_file, "x");
                }
            } else {
                if (use_x_minus_a) {
                    if (a >= 0) {
                        fprintf(out_file, "(x - %.6f)^%d", a, i);
                    } else {
                        fprintf(out_file, "(x + %.6f)^%d", -a, i);
                    }
                } else {
                    fprintf(out_file, "x^%d", i);
                }
            }
        } else {
            fprintf(out_file, "%.6f", abs_coeff);
            if (i == 1) {
                if (use_x_minus_a) {
                    if (a >= 0) {
                        fprintf(out_file, "*(x - %.6f)", a);
                    } else {
                        fprintf(out_file, "*(x + %.6f)", -a);
                    }
                } else {
                    fprintf(out_file, "*x");
                }
            } else {
                if (use_x_minus_a) {
                    if (a >= 0) {
                        fprintf(out_file, "*(x - %.6f)^%d", a, i);
                    } else {
                        fprintf(out_file, "*(x + %.6f)^%d", -a, i);
                    }
                } else {
                    fprintf(out_file, "*x^%d", i);
                }
            }
        }

        first = 0;
    }

    if (first) {
        fprintf(out_file, "0");
    }

    fprintf(out_file, "\n");
}

static status_code taylor_expansion(const double eps, const double a,
                                   double **const result_coeffs,
                                   const int degree, ...) {
    if (!result_coeffs || degree < 0 || degree > 100) {
        return STATUS_INVALID_ARGS;
    }

    if (eps <= 0.0) {
        return STATUS_INVALID_ARGS;
    }

    va_list args;
    va_start(args, degree);

    double *f_coeffs = (double *)malloc((degree + 1) * sizeof(double));
    if (!f_coeffs) {
        va_end(args);
        return STATUS_MEMORY_ERROR;
    }

    for (int i = 0; i <= degree; i++) {
        f_coeffs[i] = va_arg(args, double);
    }

    va_end(args);

    *result_coeffs = (double *)malloc((degree + 1) * sizeof(double));
    if (!(*result_coeffs)) {
        free(f_coeffs);
        return STATUS_MEMORY_ERROR;
    }

    double *g_coeffs = *result_coeffs;

    for (int i = 0; i <= degree; i++) {
        g_coeffs[i] = 0.0;
    }

    double **binomial_table = (double **)malloc((2 * degree + 2) * sizeof(double *));
    if (!binomial_table) {
        free(f_coeffs);
        free(*result_coeffs);
        return STATUS_MEMORY_ERROR;
    }

    for (int i = 0; i <= 2 * degree + 1; i++) {
        binomial_table[i] = (double *)calloc((degree + 2), sizeof(double));
        if (!binomial_table[i]) {
            for (int j = 0; j < i; j++) {
                free(binomial_table[j]);
            }
            free(binomial_table);
            free(f_coeffs);
            free(*result_coeffs);
            return STATUS_MEMORY_ERROR;
        }
    }

    for (int i = 0; i <= 2 * degree + 1; i++) {
        binomial_table[i][0] = 1.0;
        for (int j = 1; j <= degree && j <= i; j++) {
            binomial_table[i][j] = binomial_table[i - 1][j - 1] + binomial_table[i - 1][j];
        }
    }

    for (int k = 0; k <= degree; k++) {
        for (int j = 0; j <= degree - k; j++) {
            if (k + j <= degree) {
                double binom = binomial_table[k + j][k];
                double a_power = pow(a, (double)j);
                g_coeffs[k] += f_coeffs[k + j] * binom * a_power;
            }
        }

        if (fabs(g_coeffs[k]) < eps) {
            g_coeffs[k] = 0.0;
        }
    }

    for (int i = 0; i <= 2 * degree + 1; i++) {
        free(binomial_table[i]);
    }
    free(binomial_table);
    free(f_coeffs);

    return STATUS_OK;
}

static status_code process_file(const char *const input_file) {
    if (!input_file) {
        return STATUS_INVALID_ARGS;
    }

    FILE *in_file = fopen(input_file, "r");
    if (!in_file) {
        return STATUS_FILE_ERROR;
    }

    double eps = 0.0;
    double a = 0.0;
    int degree = 0;

    if (fscanf(in_file, "%lf %lf %d", &eps, &a, &degree) != 3) {
        fclose(in_file);
        return STATUS_PARSE_ERROR;
    }

    if (eps <= 0.0) {
        fclose(in_file);
        return STATUS_INVALID_ARGS;
    }

    if (degree < 0 || degree > 10) {
        fclose(in_file);
        return STATUS_INVALID_DEGREE;
    }

    double *f_coeffs = (double *)malloc((degree + 1) * sizeof(double));
    if (!f_coeffs) {
        fclose(in_file);
        return STATUS_MEMORY_ERROR;
    }

    for (int i = 0; i <= degree; i++) {
        if (fscanf(in_file, "%lf", &f_coeffs[i]) != 1) {
            free(f_coeffs);
            fclose(in_file);
            return STATUS_PARSE_ERROR;
        }
    }

    fclose(in_file);

    double *g_coeffs = NULL;
    status_code status = STATUS_OK;

    switch (degree) {
        case 0:
            status = taylor_expansion(eps, a, &g_coeffs, 0, f_coeffs[0]);
            break;
        case 1:
            status = taylor_expansion(eps, a, &g_coeffs, 1, f_coeffs[0], f_coeffs[1]);
            break;
        case 2:
            status = taylor_expansion(eps, a, &g_coeffs, 2, f_coeffs[0], f_coeffs[1], f_coeffs[2]);
            break;
        case 3:
            status = taylor_expansion(eps, a, &g_coeffs, 3, f_coeffs[0], f_coeffs[1], f_coeffs[2], f_coeffs[3]);
            break;
        case 4:
            status = taylor_expansion(eps, a, &g_coeffs, 4, f_coeffs[0], f_coeffs[1], f_coeffs[2], f_coeffs[3], f_coeffs[4]);
            break;
        case 5:
            status = taylor_expansion(eps, a, &g_coeffs, 5, f_coeffs[0], f_coeffs[1], f_coeffs[2], f_coeffs[3], f_coeffs[4], f_coeffs[5]);
            break;
        case 6:
            status = taylor_expansion(eps, a, &g_coeffs, 6, f_coeffs[0], f_coeffs[1], f_coeffs[2], f_coeffs[3], f_coeffs[4], f_coeffs[5], f_coeffs[6]);
            break;
        case 7:
            status = taylor_expansion(eps, a, &g_coeffs, 7, f_coeffs[0], f_coeffs[1], f_coeffs[2], f_coeffs[3], f_coeffs[4], f_coeffs[5], f_coeffs[6], f_coeffs[7]);
            break;
        case 8:
            status = taylor_expansion(eps, a, &g_coeffs, 8, f_coeffs[0], f_coeffs[1], f_coeffs[2], f_coeffs[3], f_coeffs[4], f_coeffs[5], f_coeffs[6], f_coeffs[7], f_coeffs[8]);
            break;
        case 9:
            status = taylor_expansion(eps, a, &g_coeffs, 9, f_coeffs[0], f_coeffs[1], f_coeffs[2], f_coeffs[3], f_coeffs[4], f_coeffs[5], f_coeffs[6], f_coeffs[7], f_coeffs[8], f_coeffs[9]);
            break;
        case 10:
            status = taylor_expansion(eps, a, &g_coeffs, 10, f_coeffs[0], f_coeffs[1], f_coeffs[2], f_coeffs[3], f_coeffs[4], f_coeffs[5], f_coeffs[6], f_coeffs[7], f_coeffs[8], f_coeffs[9], f_coeffs[10]);
            break;
        default:
            free(f_coeffs);
            return STATUS_INVALID_DEGREE;
    }

    if (status != STATUS_OK) {
        free(f_coeffs);
        return status;
    }

    printf("Input polynomial:\n");
    print_polynomial(stdout, "f", f_coeffs, degree, 0, a);

    printf("\nExpanded polynomial:\n");
    print_polynomial(stdout, "g", g_coeffs, degree, 1, a);

    printf("\nCoefficients of g(x - a):\n");
    for (int i = 0; i <= degree; i++) {
        printf("g_%d = %.15f\n", i, g_coeffs[i]);
    }

    free(f_coeffs);
    free(g_coeffs);

    return STATUS_OK;
}

int main(const int argc, char *const argv[])
{
    if (argc != 2) {
        print_error(STATUS_INVALID_ARGS);
        return (int)STATUS_INVALID_ARGS;
    }

    const char *const input_file = argv[1];

    status_code status = process_file(input_file);

    if (status != STATUS_OK) {
        print_error(status);
        return (int)status;
    }

    return (int)STATUS_OK;
}