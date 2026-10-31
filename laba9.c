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
    STATUS_INVALID_BASE = 4,
    STATUS_INVALID_NUMBER = 5,
    STATUS_PARSE_ERROR = 6
} status_code;

static long long gcd(const long long a, const long long b) {
    if (b == 0) {
        return a;
    }
    return gcd(b, a % b);
}

static int is_finite_in_base(const double fraction, const int base) {
    if (fraction <= 0.0 || fraction >= 1.0) {
        return 0;
    }

    if (base < 2 || base > 36) {
        return 0;
    }

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.15f", fraction);

    char *dot = strchr(buffer, '.');
    if (!dot) {
        return 0;
    }

    const char *frac_part = dot + 1;
    int frac_len = strlen(frac_part);

    if (frac_len == 0) {
        return 1;
    }

    long long numerator = 0;
    for (int i = 0; i < frac_len; i++) {
        numerator = numerator * 10 + (frac_part[i] - '0');
    }

    long long denominator = 1;
    for (int i = 0; i < frac_len; i++) {
        denominator *= 10;
    }

    if (numerator == 0) {
        return 1;
    }

    long long g = gcd(numerator, denominator);
    denominator = denominator / g;

    long long temp_denom = denominator;
    for (long long i = 2; i * i <= temp_denom; i++) {
        if (temp_denom % i == 0) {
            if (base % i != 0) {
                return 0;
            }

            while (temp_denom % i == 0) {
                temp_denom /= i;
            }
        }
    }

    if (temp_denom > 1) {
        if (base % temp_denom != 0) {
            return 0;
        }
    }

    return 1;
}

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
        case STATUS_INVALID_BASE:
            fprintf(stderr, "Error: invalid base\n");
            break;
        case STATUS_INVALID_NUMBER:
            fprintf(stderr, "Error: invalid number\n");
            break;
        case STATUS_PARSE_ERROR:
            fprintf(stderr, "Error: parse error\n");
            break;
        case STATUS_OK:
            break;
    }
}

static status_code check_fractions_in_base(const int base, 
                                           const int count, ...)
{
    if (base < 2 || base > 36) {
        return STATUS_INVALID_BASE;
    }

    if (count <= 0) {
        return STATUS_INVALID_ARGS;
    }

    va_list args;
    va_start(args, count);

    int finite_count = 0;

    printf("%-20s | Finite\n", "Fraction");
    printf("%-20s | ------\n", "");

    for (int i = 0; i < count; i++) {
        double fraction = va_arg(args, double);

        if (fraction <= 0.0 || fraction >= 1.0) {
            printf("%.15f | Invalid\n", fraction);
            continue;
        }

        int is_finite = is_finite_in_base(fraction, base);
        if (is_finite) {
            finite_count++;
        }

        const char *result = is_finite ? "Yes" : "No";
        printf("%.15f | %s\n", fraction, result);
    }

    va_end(args);

    printf("\nFinite: %d/%d\n", finite_count, count);

    return STATUS_OK;
}

static status_code process_file(const char *const input_file)
{
    if (!input_file) {
        return STATUS_INVALID_ARGS;
    }

    FILE *in_file = fopen(input_file, "r");
    if (!in_file) {
        return STATUS_FILE_ERROR;
    }

    int base = 0;
    int count = 0;

    if (fscanf(in_file, "%d %d", &base, &count) != 2) {
        fclose(in_file);
        return STATUS_PARSE_ERROR;
    }

    if (base < 2 || base > 36) {
        fclose(in_file);
        return STATUS_INVALID_BASE;
    }

    if (count <= 0 || count > 10) {
        fclose(in_file);
        return STATUS_INVALID_ARGS;
    }

    double *fractions = (double *)malloc(count * sizeof(double));
    if (!fractions) {
        fclose(in_file);
        return STATUS_MEMORY_ERROR;
    }

    for (int i = 0; i < count; i++) {
        if (fscanf(in_file, "%lf", &fractions[i]) != 1) {
            free(fractions);
            fclose(in_file);
            return STATUS_PARSE_ERROR;
        }
    }

    fclose(in_file);

    printf("Base: %d\n", base);
    printf("Total fractions: %d\n\n", count);

    status_code check_status = STATUS_OK;

    switch (count) {
        case 1:
            check_status = check_fractions_in_base(base, 1, fractions[0]);
            break;
        case 2:
            check_status = check_fractions_in_base(base, 2, fractions[0], fractions[1]);
            break;
        case 3:
            check_status = check_fractions_in_base(base, 3, fractions[0], fractions[1], fractions[2]);
            break;
        case 4:
            check_status = check_fractions_in_base(base, 4, fractions[0], fractions[1], fractions[2], fractions[3]);
            break;
        case 5:
            check_status = check_fractions_in_base(base, 5, fractions[0], fractions[1], fractions[2], fractions[3], fractions[4]);
            break;
        case 6:
            check_status = check_fractions_in_base(base, 6, fractions[0], fractions[1], fractions[2], fractions[3], fractions[4], fractions[5]);
            break;
        case 7:
            check_status = check_fractions_in_base(base, 7, fractions[0], fractions[1], fractions[2], fractions[3], fractions[4], fractions[5], fractions[6]);
            break;
        case 8:
            check_status = check_fractions_in_base(base, 8, fractions[0], fractions[1], fractions[2], fractions[3], fractions[4], fractions[5], fractions[6], fractions[7]);
            break;
        case 9:
            check_status = check_fractions_in_base(base, 9, fractions[0], fractions[1], fractions[2], fractions[3], fractions[4], fractions[5], fractions[6], fractions[7], fractions[8]);
            break;
        case 10:
            check_status = check_fractions_in_base(base, 10, fractions[0], fractions[1], fractions[2], fractions[3], fractions[4], fractions[5], fractions[6], fractions[7], fractions[8], fractions[9]);
            break;
        default:
            free(fractions);
            return STATUS_INVALID_ARGS;
    }

    free(fractions);

    return check_status;
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

    printf("\nSuccess: fractions checked\n");
    return (int)STATUS_OK;
}