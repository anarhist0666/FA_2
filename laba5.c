#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <locale.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_INVALID_ARGS = 1,
    STATUS_FILE_ERROR = 2,
    STATUS_MEMORY_ERROR = 3,
    STATUS_PARSE_ERROR = 4
} status_code;

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

static int int_to_roman(int num, char *buffer, size_t max_size) {
    if (num < 1 || num > 3999 || !buffer || max_size == 0) {
        return -1;
    }

    const int values[] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};
    const char *numerals[] = {"M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"};

    int pos = 0;
    int temp_num = num;

    for (int i = 0; i < 13; i++) {
        while (temp_num >= values[i]) {
            const char *numeral = numerals[i];
            for (int j = 0; numeral[j]; j++) {
                if ((size_t)(pos + 1) >= max_size) {
                    return -1;
                }
                buffer[pos++] = numeral[j];
            }
            temp_num -= values[i];
        }
    }

    buffer[pos] = '\0';
    return pos;
}

static int int_to_zeckendorf(unsigned int num, char *buffer, size_t max_size) {
    if (!buffer || max_size == 0) {
        return -1;
    }

    if (num == 0) {
        if (max_size < 2) {
            return -1;
        }
        buffer[0] = '1';
        buffer[1] = '\0';
        return 1;
    }

    unsigned int fib[64];
    int fib_count = 0;
    fib[0] = 1;
    fib[1] = 2;
    fib_count = 2;

    while (fib[fib_count - 1] < num && fib_count < 64) {
        fib[fib_count] = fib[fib_count - 1] + fib[fib_count - 2];
        fib_count++;
    }

    int selected[64] = {0};
    int max_fib_idx = -1;
    unsigned int temp = num;

    for (int i = fib_count - 1; i >= 0; i--) {
        if (temp >= fib[i]) {
            temp -= fib[i];
            selected[i] = 1;
            if (max_fib_idx == -1) {
                max_fib_idx = i;
            }
        }
    }

    if (max_fib_idx == -1) {
        if (max_size < 2) {
            return -1;
        }
        buffer[0] = '1';
        buffer[1] = '\0';
        return 1;
    }

    int pos = 0;
    for (int i = 0; i <= max_fib_idx; i++) {
        if ((size_t)(pos + 1) >= max_size) {
            return -1;
        }
        buffer[pos++] = selected[i] ? '1' : '0';
    }

    if ((size_t)(pos + 1) >= max_size) {
        return -1;
    }
    buffer[pos++] = '1';
    buffer[pos] = '\0';

    return pos;
}

static int int_to_base(int num, int base, char *buffer, size_t max_size) {
    if (base < 2 || base > 36 || !buffer || max_size == 0) {
        return -1;
    }

    int is_negative = 0;
    long long abs_num;

    if (num < 0) {
        is_negative = 1;
        abs_num = -(long long)num;
    } else {
        abs_num = (long long)num;
    }

    if (abs_num == 0) {
        if (max_size < 2) {
            return -1;
        }
        buffer[0] = '0';
        buffer[1] = '\0';
        return 1;
    }

    char digits[64];
    int len = 0;

    while (abs_num > 0 && len < 64) {
        int digit = abs_num % base;
        digits[len++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        abs_num /= base;
    }

    int total_len = len + (is_negative ? 1 : 0);
    if (total_len + 1 > (int)max_size) {
        return -1;
    }

    int pos = 0;
    if (is_negative) {
        buffer[pos++] = '-';
    }

    for (int i = len - 1; i >= 0; i--) {
        buffer[pos++] = digits[i];
    }
    buffer[pos] = '\0';

    return total_len;
}

static int int_to_base_uppercase(int num, int base, char *buffer, size_t max_size) {
    if (base < 2 || base > 36 || !buffer || max_size == 0) {
        return -1;
    }

    int is_negative = 0;
    long long abs_num;

    if (num < 0) {
        is_negative = 1;
        abs_num = -(long long)num;
    } else {
        abs_num = (long long)num;
    }

    if (abs_num == 0) {
        if (max_size < 2) {
            return -1;
        }
        buffer[0] = '0';
        buffer[1] = '\0';
        return 1;
    }

    char digits[64];
    int len = 0;

    while (abs_num > 0 && len < 64) {
        int digit = abs_num % base;
        digits[len++] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        abs_num /= base;
    }

    int total_len = len + (is_negative ? 1 : 0);
    if (total_len + 1 > (int)max_size) {
        return -1;
    }

    int pos = 0;
    if (is_negative) {
        buffer[pos++] = '-';
    }

    for (int i = len - 1; i >= 0; i--) {
        buffer[pos++] = digits[i];
    }
    buffer[pos] = '\0';

    return total_len;
}

static int base_string_to_int(const char *str, int base, int *result) {
    if (!str || !result || base < 2 || base > 36) {
        return -1;
    }

    long long value = 0;
    int parsed = 0;

    for (int i = 0; str[i]; i++) {
        char c = str[i];
        int digit = char_to_digit(c);

        if (digit < 0 || digit >= base) {
            return -1;
        }

        value = value * base + digit;
        parsed = 1;

        if (value > 2147483647LL) {
            return -1;
        }
    }

    if (parsed == 0) {
        return -1;
    }

    *result = (int)value;
    return 0;
}

static int memory_dump_binary(const unsigned char *bytes, int count, char *buffer, size_t max_size) {
    if (!buffer || max_size == 0 || !bytes) {
        return -1;
    }

    int pos = 0;

    for (int i = 0; i < count; i++) {
        if (i > 0) {
            if ((size_t)(pos + 1) >= max_size) {
                return -1;
            }
            buffer[pos++] = ' ';
        }

        unsigned char byte = bytes[i];
        for (int j = 7; j >= 0; j--) {
            if ((size_t)(pos + 1) >= max_size) {
                return -1;
            }
            buffer[pos++] = ((byte >> j) & 1) ? '1' : '0';
        }
    }

    buffer[pos] = '\0';
    return pos;
}

static int oversprint_impl(char *output_buffer, size_t max_size, const char *format, va_list args) {
    if (!output_buffer || !format || max_size == 0) {
        return -1;
    }

    int written = 0;
    size_t remaining = max_size;

    for (int i = 0; format[i] && remaining > 1; i++) {
        if (format[i] != '%') {
            output_buffer[written++] = format[i];
            remaining--;
            continue;
        }

        if (!format[i + 1]) {
            break;
        }

        i++;

        if (format[i] == 'R' && format[i + 1] == 'o') {
            int num = va_arg(args, int);
            char temp[32];
            int len = int_to_roman(num, temp, sizeof(temp));
            if (len < 0) {
                if (remaining < 4) {
                    return -1;
                }
                output_buffer[written++] = 'E';
                output_buffer[written++] = 'R';
                output_buffer[written++] = 'R';
                remaining -= 3;
            } else {
                if ((size_t)len >= remaining) {
                    return -1;
                }
                for (int j = 0; j < len; j++) {
                    output_buffer[written++] = temp[j];
                }
                remaining -= len;
            }
            i++;
            continue;
        }

        if (format[i] == 'Z' && format[i + 1] == 'r') {
            unsigned int num = va_arg(args, unsigned int);
            char temp[256];
            int len = int_to_zeckendorf(num, temp, sizeof(temp));
            if (len < 0) {
                return -1;
            }
            if ((size_t)len >= remaining) {
                return -1;
            }
            for (int j = 0; j < len; j++) {
                output_buffer[written++] = temp[j];
            }
            remaining -= len;
            i++;
            continue;
        }

        if (format[i] == 'C' && format[i + 1] == 'v') {
            int num = va_arg(args, int);
            int base = va_arg(args, int);
            char temp[64];
            int len = int_to_base(num, base, temp, sizeof(temp));
            if (len < 0) {
                return -1;
            }
            if ((size_t)len >= remaining) {
                return -1;
            }
            for (int j = 0; j < len; j++) {
                output_buffer[written++] = temp[j];
            }
            remaining -= len;
            i++;
            continue;
        }

        if (format[i] == 'C' && format[i + 1] == 'V') {
            int num = va_arg(args, int);
            int base = va_arg(args, int);
            char temp[64];
            int len = int_to_base_uppercase(num, base, temp, sizeof(temp));
            if (len < 0) {
                return -1;
            }
            if ((size_t)len >= remaining) {
                return -1;
            }
            for (int j = 0; j < len; j++) {
                output_buffer[written++] = temp[j];
            }
            remaining -= len;
            i++;
            continue;
        }

        if (format[i] == 't' && format[i + 1] == 'o') {
            const char *str = va_arg(args, const char *);
            int base = va_arg(args, int);
            int value = 0;

            if (base_string_to_int(str, base, &value) < 0) {
                if (remaining < 4) {
                    return -1;
                }
                output_buffer[written++] = 'E';
                output_buffer[written++] = 'R';
                output_buffer[written++] = 'R';
                remaining -= 3;
            } else {
                char temp[64];
                int len = snprintf(temp, sizeof(temp), "%d", value);
                if (len < 0 || (size_t)len >= remaining) {
                    return -1;
                }
                for (int j = 0; j < len; j++) {
                    output_buffer[written++] = temp[j];
                }
                remaining -= len;
            }
            i++;
            continue;
        }

        if (format[i] == 'T' && format[i + 1] == 'O') {
            const char *str = va_arg(args, const char *);
            int base = va_arg(args, int);
            int value = 0;

            if (base_string_to_int(str, base, &value) < 0) {
                if (remaining < 4) {
                    return -1;
                }
                output_buffer[written++] = 'E';
                output_buffer[written++] = 'R';
                output_buffer[written++] = 'R';
                remaining -= 3;
            } else {
                char temp[64];
                int len = snprintf(temp, sizeof(temp), "%d", value);
                if (len < 0 || (size_t)len >= remaining) {
                    return -1;
                }
                for (int j = 0; j < len; j++) {
                    output_buffer[written++] = temp[j];
                }
                remaining -= len;
            }
            i++;
            continue;
        }

        if (format[i] == 'm' && format[i + 1] == 'i') {
            int num = va_arg(args, int);
            unsigned char *bytes = (unsigned char *)&num;
            char temp[256];
            int len = memory_dump_binary(bytes, sizeof(int), temp, sizeof(temp));
            if (len < 0) {
                return -1;
            }
            if ((size_t)len >= remaining) {
                return -1;
            }
            for (int j = 0; j < len; j++) {
                output_buffer[written++] = temp[j];
            }
            remaining -= len;
            i++;
            continue;
        }

        if (format[i] == 'm' && format[i + 1] == 'u') {
            unsigned int num = va_arg(args, unsigned int);
            unsigned char *bytes = (unsigned char *)&num;
            char temp[256];
            int len = memory_dump_binary(bytes, sizeof(unsigned int), temp, sizeof(temp));
            if (len < 0) {
                return -1;
            }
            if ((size_t)len >= remaining) {
                return -1;
            }
            for (int j = 0; j < len; j++) {
                output_buffer[written++] = temp[j];
            }
            remaining -= len;
            i++;
            continue;
        }

        if (format[i] == 'm' && format[i + 1] == 'd') {
            double num = va_arg(args, double);
            unsigned char *bytes = (unsigned char *)&num;
            char temp[256];
            int len = memory_dump_binary(bytes, sizeof(double), temp, sizeof(temp));
            if (len < 0) {
                return -1;
            }
            if ((size_t)len >= remaining) {
                return -1;
            }
            for (int j = 0; j < len; j++) {
                output_buffer[written++] = temp[j];
            }
            remaining -= len;
            i++;
            continue;
        }

        if (format[i] == 'm' && format[i + 1] == 'f') {
            double dnum = va_arg(args, double);
            float fnum = (float)dnum;
            unsigned char *bytes = (unsigned char *)&fnum;
            char temp[256];
            int len = memory_dump_binary(bytes, sizeof(float), temp, sizeof(temp));
            if (len < 0) {
                return -1;
            }
            if ((size_t)len >= remaining) {
                return -1;
            }
            for (int j = 0; j < len; j++) {
                output_buffer[written++] = temp[j];
            }
            remaining -= len;
            i++;
            continue;
        }

        if (format[i] == 'd') {
            int num = va_arg(args, int);
            char temp[64];
            int len = snprintf(temp, sizeof(temp), "%d", num);
            if (len < 0 || (size_t)len >= remaining) {
                return -1;
            }
            for (int j = 0; j < len; j++) {
                output_buffer[written++] = temp[j];
            }
            remaining -= len;
            continue;
        }

        if (format[i] == 's') {
            const char *str = va_arg(args, const char *);
            if (!str) {
                return -1;
            }
            for (int j = 0; str[j]; j++) {
                if (remaining < 2) {
                    return -1;
                }
                output_buffer[written++] = str[j];
                remaining--;
            }
            continue;
        }

        if (format[i] == 'c') {
            char ch = (char)va_arg(args, int);
            if (remaining < 2) {
                return -1;
            }
            output_buffer[written++] = ch;
            remaining--;
            continue;
        }

        if (format[i] == 'u') {
            unsigned int num = va_arg(args, unsigned int);
            char temp[64];
            int len = snprintf(temp, sizeof(temp), "%u", num);
            if (len < 0 || (size_t)len >= remaining) {
                return -1;
            }
            for (int j = 0; j < len; j++) {
                output_buffer[written++] = temp[j];
            }
            remaining -= len;
            continue;
        }

        if (format[i] == 'f') {
            double num = va_arg(args, double);
            char temp[128];
            int len = snprintf(temp, sizeof(temp), "%f", num);
            if (len < 0 || (size_t)len >= remaining) {
                return -1;
            }
            for (int j = 0; j < len; j++) {
                output_buffer[written++] = temp[j];
            }
            remaining -= len;
            continue;
        }

        if (format[i] == '%') {
            if (remaining < 2) {
                return -1;
            }
            output_buffer[written++] = '%';
            remaining--;
            continue;
        }
    }

    if (remaining < 1) {
        return -1;
    }
    output_buffer[written] = '\0';
    return written;
}

int oversprint(char *output_buffer, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = oversprint_impl(output_buffer, 4096, format, args);
    va_end(args);
    return result;
}

int overprintf(FILE *stream, const char *format, ...) {
    if (!stream) {
        return -1;
    }

    char buffer[4096];
    va_list args;
    va_start(args, format);
    int result = oversprint_impl(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (result < 0) {
        return -1;
    }

    if (fputs(buffer, stream) < 0) {
        return -1;
    }

    return result;
}

static int my_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
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
        case STATUS_PARSE_ERROR:
            fprintf(stderr, "Error: parse error\n");
            break;
        case STATUS_OK:
            break;
    }
}

#define EXEC_CMD(fmt, ...) \
    do { \
        if (overprintf(stdout, fmt, __VA_ARGS__) < 0) { \
            return STATUS_MEMORY_ERROR; \
        } \
    } while (0)

#define EXEC_CMD_BUF(fmt, buf, ...) \
    do { \
        if (oversprint(buf, fmt, __VA_ARGS__) < 0) { \
            return STATUS_MEMORY_ERROR; \
        } \
    } while (0)

static status_code process_file(FILE *const file)
{
    char cmd[16];

    while (fscanf(file, "%15s", cmd) == 1) {
        if (my_strcmp(cmd, "Ro") == 0) {
            int num;
            if (fscanf(file, "%d", &num) != 1) {
                return STATUS_PARSE_ERROR;
            }
            EXEC_CMD("Roman: %%Ro -> %Ro\n", num);

        } else if (my_strcmp(cmd, "Zr") == 0) {
            unsigned int num;
            if (fscanf(file, "%u", &num) != 1) {
                return STATUS_PARSE_ERROR;
            }
            EXEC_CMD("Zeckendorf: %%Zr -> %Zr\n", num);

        } else if (my_strcmp(cmd, "Cv") == 0) {
            int num, base;
            if (fscanf(file, "%d %d", &num, &base) != 2) {
                return STATUS_PARSE_ERROR;
            }
            EXEC_CMD("Cv: %%Cv -> %Cv\n", num, base);

        } else if (my_strcmp(cmd, "CV") == 0) {
            int num, base;
            if (fscanf(file, "%d %d", &num, &base) != 2) {
                return STATUS_PARSE_ERROR;
            }
            EXEC_CMD("CV: %%CV -> %CV\n", num, base);

        } else if (my_strcmp(cmd, "to") == 0) {
            char str[256];
            int base;
            if (fscanf(file, "%255s %d", str, &base) != 2) {
                return STATUS_PARSE_ERROR;
            }
            char buffer[4096];
            EXEC_CMD_BUF("to: %%to -> %to", buffer, str, base);
            EXEC_CMD("%s\n", buffer);

        } else if (my_strcmp(cmd, "TO") == 0) {
            char str[256];
            int base;
            if (fscanf(file, "%255s %d", str, &base) != 2) {
                return STATUS_PARSE_ERROR;
            }
            char buffer[4096];
            EXEC_CMD_BUF("TO: %%TO -> %TO", buffer, str, base);
            EXEC_CMD("%s\n", buffer);

        } else if (my_strcmp(cmd, "mi") == 0) {
            int num;
            if (fscanf(file, "%d", &num) != 1) {
                return STATUS_PARSE_ERROR;
            }
            EXEC_CMD("mi: %%mi -> %mi\n", num);

        } else if (my_strcmp(cmd, "mu") == 0) {
            unsigned int num;
            if (fscanf(file, "%u", &num) != 1) {
                return STATUS_PARSE_ERROR;
            }
            EXEC_CMD("mu: %%mu -> %mu\n", num);

        } else if (my_strcmp(cmd, "md") == 0) {
            double num;
            if (fscanf(file, "%lf", &num) != 1) {
                return STATUS_PARSE_ERROR;
            }
            EXEC_CMD("md: %%md -> %md\n", num);

        } else if (my_strcmp(cmd, "mf") == 0) {
            double num;
            if (fscanf(file, "%lf", &num) != 1) {
                return STATUS_PARSE_ERROR;
            }
            EXEC_CMD("mf: %%mf -> %mf\n", num);

        } else if (my_strcmp(cmd, "d") == 0) {
            int num;
            if (fscanf(file, "%d", &num) != 1) {
                return STATUS_PARSE_ERROR;
            }
            EXEC_CMD("Standard %%d -> %d\n", num);

        } else if (my_strcmp(cmd, "s") == 0) {
            char str[256];
            if (fscanf(file, "%255s", str) != 1) {
                return STATUS_PARSE_ERROR;
            }
            EXEC_CMD("Standard %%s -> %s\n", str);

        } else if (my_strcmp(cmd, "c") == 0) {
            char ch;
            if (fscanf(file, " %c", &ch) != 1) {
                return STATUS_PARSE_ERROR;
            }
            EXEC_CMD("Standard %%c -> %c\n", ch);

        } else if (my_strcmp(cmd, "u") == 0) {
            unsigned int num;
            if (fscanf(file, "%u", &num) != 1) {
                return STATUS_PARSE_ERROR;
            }
            EXEC_CMD("Standard %%u -> %u\n", num);

        } else if (my_strcmp(cmd, "f") == 0) {
            double num;
            if (fscanf(file, "%lf", &num) != 1) {
                return STATUS_PARSE_ERROR;
            }
            EXEC_CMD("Standard %%f -> %f\n", num);

        } else {
            printf("Unknown command: %s\n", cmd);
        }
    }

    return STATUS_OK;
}

int main(const int argc, char *const argv[])
{
    setlocale(LC_NUMERIC, "C");
    
    if (argc != 2) {
        print_error(STATUS_INVALID_ARGS);
        return (int)STATUS_INVALID_ARGS;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        print_error(STATUS_FILE_ERROR);
        return (int)STATUS_FILE_ERROR;
    }

    status_code status = process_file(file);
    fclose(file);

    if (status != STATUS_OK) {
        print_error(status);
        return (int)status;
    }

    return (int)STATUS_OK;
}