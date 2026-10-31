#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_INVALID_ARGS = 1,
    STATUS_FILE_ERROR = 2,
    STATUS_PARSE_ERROR = 3,
    STATUS_MEMORY_ERROR = 4
} status_code;

static int roman_to_int(const char *str) {
    if (!str || str[0] == '\0') {
        return -1;
    }

    const char *values_str[] = {"M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"};
    const int values[] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};

    int result = 0;
    int i = 0;

    for (int j = 0; j < 13; j++) {
        while (str[i] != '\0') {
            const char *numeral = values_str[j];
            int match = 1;

            for (int k = 0; numeral[k] != '\0'; k++) {
                if (str[i + k] != numeral[k]) {
                    match = 0;
                    break;
                }
            }

            if (match) {
                result += values[j];
                int len = 0;
                while (values_str[j][len] != '\0') {
                    len++;
                }
                i += len;
            } else {
                break;
            }
        }
    }

    if (str[i] != '\0') {
        return -1;
    }

    return result;
}

static int zeckendorf_to_int(const char *str) {
    if (!str || str[0] == '\0') {
        return -1;
    }

    unsigned int fib[64];
    int fib_count = 0;
    fib[0] = 1;
    fib[1] = 2;
    fib_count = 2;

    while (fib[fib_count - 1] < 4294967295U && fib_count < 64) {
        fib[fib_count] = fib[fib_count - 1] + fib[fib_count - 2];
        fib_count++;
    }

    int len = 0;
    while (str[len] != '\0') {
        if (str[len] != '0' && str[len] != '1') {
            return -1;
        }
        len++;
    }

    if (len < 2 || str[len - 1] != '1') {
        return -1;
    }

    unsigned int result = 0;

    for (int i = 0; i < len - 1; i++) {
        if (str[i] == '1') {
            if (i >= fib_count) {
                return -1;
            }
            result += fib[i];
        }
    }

    return (int)result;
}

static int char_to_digit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    }
    return -1;
}

static int base_string_to_int(const char *str, int base, int *result) {
    if (!str || !result || base < 2 || base > 36) {
        return 0;
    }

    long long value = 0;
    int is_negative = 0;
    int i = 0;

    if (str[0] == '-') {
        is_negative = 1;
        i = 1;
    } else if (str[0] == '+') {
        i = 1;
    }

    if (str[i] == '\0') {
        return 0;
    }

    while (str[i] != '\0') {
        int digit = char_to_digit(str[i]);

        if (digit < 0 || digit >= base) {
            break;
        }

        value = value * base + digit;
        i++;

        if (value > 2147483647LL) {
            return 0;
        }
    }

    if (i == (is_negative ? 1 : 0) || str[i] != '\0') {
        return 0;
    }

    if (is_negative) {
        value = -value;
    }

    *result = (int)value;
    return 1;
}

static void skip_whitespace(FILE *stream, int *c) {
    do {
        *c = fgetc(stream);
    } while (*c != EOF && (*c == ' ' || *c == '\t' || *c == '\n' || *c == '\r'));
}

static int overscanf_impl(FILE *stream, const char *format, va_list args) {
    if (!stream || !format) {
        return -1;
    }

    int matched = 0;
    int i = 0;

    while (format[i] != '\0') {
        if (format[i] != '%') {
            if (isspace((unsigned char)format[i])) {
                int c;
                do {
                    c = fgetc(stream);
                } while (c != EOF && isspace((unsigned char)c));
                if (c != EOF) {
                    ungetc(c, stream);
                }
                i++;
                continue;
            }
            int c = fgetc(stream);
            if (c == EOF || c != (unsigned char)format[i]) {
                return matched;
            }
            i++;
            continue;
        }

        if (!format[i + 1]) {
            break;
        }

        i++;

        if (format[i] == 'R' && format[i + 1] == 'o') {
            int *ptr = va_arg(args, int *);
            char buffer[256];
            int pos = 0;
            int c;

            skip_whitespace(stream, &c);

            if (c != EOF) {
                ungetc(c, stream);
            }

            while ((c = fgetc(stream)) != EOF && isalpha(c)) {
                if (pos < 255) {
                    buffer[pos++] = c;
                }
            }

            if (c != EOF) {
                ungetc(c, stream);
            }

            if (pos == 0) {
                return matched;
            }

            buffer[pos] = '\0';
            int val = roman_to_int(buffer);
            if (val < 0) {
                return matched;
            }

            *ptr = val;
            matched++;
            i += 2;
            continue;
        }

        if (format[i] == 'Z' && format[i + 1] == 'r') {
            unsigned int *ptr = va_arg(args, unsigned int *);
            char buffer[256];
            int pos = 0;
            int c;

            skip_whitespace(stream, &c);

            if (c != EOF) {
                ungetc(c, stream);
            }

            while ((c = fgetc(stream)) != EOF && (c == '0' || c == '1')) {
                if (pos < 255) {
                    buffer[pos++] = c;
                }
            }

            if (c != EOF) {
                ungetc(c, stream);
            }

            if (pos == 0) {
                return matched;
            }

            buffer[pos] = '\0';
            int val = zeckendorf_to_int(buffer);
            if (val < 0) {
                return matched;
            }

            *ptr = (unsigned int)val;
            matched++;
            i += 2;
            continue;
        }

        if (format[i] == 'C' && format[i + 1] == 'v') {
            int *ptr = va_arg(args, int *);
            int base = va_arg(args, int);

            if (base < 2 || base > 36) {
                base = 10;
            }

            char buffer[256];
            int pos = 0;
            int c;

            skip_whitespace(stream, &c);

            if (c == '-' || c == '+') {
                if (pos < 255) {
                    buffer[pos++] = c;
                }
                c = fgetc(stream);
            }

            while (c != EOF && isalnum(c)) {
                if (pos < 255) {
                    buffer[pos++] = tolower(c);
                }
                c = fgetc(stream);
            }

            if (c != EOF) {
                ungetc(c, stream);
            }

            if (pos == 0) {
                return matched;
            }

            buffer[pos] = '\0';
            int val;
            if (!base_string_to_int(buffer, base, &val)) {
                return matched;
            }

            *ptr = val;
            matched++;
            i += 2;
            continue;
        }

        if (format[i] == 'C' && format[i + 1] == 'V') {
            int *ptr = va_arg(args, int *);
            int base = va_arg(args, int);

            if (base < 2 || base > 36) {
                base = 10;
            }

            char buffer[256];
            int pos = 0;
            int c;

            skip_whitespace(stream, &c);

            if (c == '-' || c == '+') {
                if (pos < 255) {
                    buffer[pos++] = c;
                }
                c = fgetc(stream);
            }

            while (c != EOF && isalnum(c)) {
                if (pos < 255) {
                    buffer[pos++] = tolower(c);
                }
                c = fgetc(stream);
            }

            if (c != EOF) {
                ungetc(c, stream);
            }

            if (pos == 0) {
                return matched;
            }

            buffer[pos] = '\0';
            int val;
            if (!base_string_to_int(buffer, base, &val)) {
                return matched;
            }

            *ptr = val;
            matched++;
            i += 2;
            continue;
        }

        if (format[i] == 'l' && format[i + 1] == 'f') {
            double *ptr = va_arg(args, double *);
            double val = 0.0;
            int res = fscanf(stream, "%lf", &val);
            if (res != 1) {
                return matched;
            }
            *ptr = val;
            matched++;
            i += 2;
            continue;
        }

        if (format[i] == 'l' && format[i + 1] == 'd') {
            long *ptr = va_arg(args, long *);
            long val = 0;
            int res = fscanf(stream, "%ld", &val);
            if (res != 1) {
                return matched;
            }
            *ptr = val;
            matched++;
            i += 2;
            continue;
        }

        if (format[i] == 'l' && format[i + 1] == 'u') {
            unsigned long *ptr = va_arg(args, unsigned long *);
            unsigned long val = 0;
            int res = fscanf(stream, "%lu", &val);
            if (res != 1) {
                return matched;
            }
            *ptr = val;
            matched++;
            i += 2;
            continue;
        }

        if (format[i] == 'd') {
            int *ptr = va_arg(args, int *);
            int val = 0;
            int res = fscanf(stream, "%d", &val);
            if (res != 1) {
                return matched;
            }
            *ptr = val;
            matched++;
            i++;
            continue;
        }

        if (format[i] == 's') {
            char *ptr = va_arg(args, char *);
            int res = fscanf(stream, "%255s", ptr);
            if (res != 1) {
                return matched;
            }
            matched++;
            i++;
            continue;
        }

        if (format[i] == 'c') {
            char *ptr = va_arg(args, char *);
            int c = fgetc(stream);
            if (c == EOF) {
                return matched;
            }
            *ptr = (char)c;
            matched++;
            i++;
            continue;
        }

        if (format[i] == 'u') {
            unsigned int *ptr = va_arg(args, unsigned int *);
            unsigned int val = 0;
            int res = fscanf(stream, "%u", &val);
            if (res != 1) {
                return matched;
            }
            *ptr = val;
            matched++;
            i++;
            continue;
        }

        if (format[i] == '%') {
            int c = fgetc(stream);
            if (c == EOF || c != '%') {
                return matched;
            }
            i++;
            continue;
        }

        i++;
    }

    return matched;
}

int oversscanf(const char *str, const char *format, ...) {
    if (!str || !format) {
        return -1;
    }

    FILE *stream = fmemopen((void *)str, strlen(str), "r");
    if (stream == NULL) {
        return -1;
    }

    va_list args;
    va_start(args, format);
    int result = overscanf_impl(stream, format, args);
    va_end(args);

    fclose(stream);
    return result;
}

int overfscanf(FILE *stream, const char *format, ...) {
    if (!stream || !format) {
        return -1;
    }

    va_list args;
    va_start(args, format);
    int result = overscanf_impl(stream, format, args);
    va_end(args);

    return result;
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
        case STATUS_PARSE_ERROR:
            fprintf(stderr, "Error: parse error\n");
            break;
        case STATUS_MEMORY_ERROR:
            fprintf(stderr, "Error: memory allocation failed\n");
            break;
        case STATUS_OK:
            break;
    }
}

typedef struct {
    int roman;
    int hex_val;
    unsigned int zeck;
    int neg;
    double dnum;
    char str[256];
    char ch;
    int bin;
} ParsedData;

static status_code process_file(FILE *const file, ParsedData *const data)
{
    if (file == NULL || data == NULL) {
        return STATUS_INVALID_ARGS;
    }

    int res = 0;

    res = overfscanf(file, "%Ro %CV", &data->roman, &data->hex_val, 16);
    if (res != 2) {
        return STATUS_PARSE_ERROR;
    }

    res = overfscanf(file, "%Zr %Cv", &data->zeck, &data->neg, 10);
    if (res != 2) {
        return STATUS_PARSE_ERROR;
    }

    res = overfscanf(file, "%lf %s %c", &data->dnum, data->str, &data->ch);
    if (res != 3) {
        return STATUS_PARSE_ERROR;
    }

    res = overfscanf(file, "%Cv", &data->bin, 2);
    if (res != 1) {
        return STATUS_PARSE_ERROR;
    }

    return STATUS_OK;
}

int main(const int argc, char *const argv[])
{
    if (argc != 2) {
        print_error(STATUS_INVALID_ARGS);
        return (int)STATUS_INVALID_ARGS;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        print_error(STATUS_FILE_ERROR);
        return (int)STATUS_FILE_ERROR;
    }

    ParsedData data = {0};
    status_code status = process_file(file, &data);

    fclose(file);

    if (status != STATUS_OK) {
        print_error(status);
        return (int)status;
    }

    printf("Matched file data:\n");
    printf("Roman: %d, Hex: %d\n", data.roman, data.hex_val);
    printf("Zeckendorf: %u, Negative: %d\n", data.zeck, data.neg);
    printf("Double: %.2f, String: %s, Char: %c\n", data.dnum, data.str, data.ch);
    printf("Binary: %d\n", data.bin);

    printf("\nString parsing tests:\n");

    char buffer[256];
    ParsedData string_data = {0};

    strcpy(buffer, "MCMXCIV 100 ff");
    int res = oversscanf(buffer, "%Ro %Cv %Cv", &string_data.roman, &string_data.hex_val, 16, &string_data.neg, 16);
    printf("String test 1 - Matched: %d, Roman: %d, Hex1: %d, Hex2: %d\n", res, string_data.roman, string_data.hex_val, string_data.neg);

    strcpy(buffer, "010011 -123");
    res = oversscanf(buffer, "%Zr %Cv", &string_data.zeck, &string_data.neg, 10);
    printf("String test 2 - Matched: %d, Zeckendorf: %u, Negative: %d\n", res, string_data.zeck, string_data.neg);

    strcpy(buffer, "IX 1010 FF");
    res = oversscanf(buffer, "%Ro %Cv %CV", &string_data.roman, &string_data.bin, 2, &string_data.hex_val, 16);
    printf("String test 3 - Matched: %d, Roman: %d, Binary: %d, Hex: %d\n", res, string_data.roman, string_data.bin, string_data.hex_val);

    strcpy(buffer, "aBcDeF 123");
    res = oversscanf(buffer, "%Cv %Cv", &string_data.hex_val, 16, &string_data.neg, 10);
    printf("String test 4 - Matched: %d, Mixed case hex: %d, Decimal: %d\n", res, string_data.hex_val, string_data.neg);

    return (int)STATUS_OK;
}