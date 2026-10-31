#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_INVALID_ARGS = 1,
    STATUS_FILE_ERROR = 2,
    STATUS_MEMORY_ERROR = 3,
    STATUS_INVALID_BASE = 4,
    STATUS_INVALID_NUMBER = 5,
    STATUS_PARSE_ERROR = 6
} status_code;

static int char_to_digit(const char c) {
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

static char digit_to_char(const int digit) {
    if (digit >= 0 && digit <= 9) {
        return '0' + digit;
    }
    if (digit >= 10 && digit <= 35) {
        return 'a' + (digit - 10);
    }
    return '?';
}

static int is_valid_number(const char *const str, const int base) {
    if (!str || str[0] == '\0') {
        return 0;
    }

    for (int i = 0; str[i] != '\0'; i++) {
        int digit = char_to_digit(str[i]);
        if (digit < 0 || digit >= base) {
            return 0;
        }
    }
    return 1;
}

static char *remove_leading_zeros(char *const num_str) {
    if (!num_str) {
        return num_str;
    }

    int i = 0;
    while (num_str[i] == '0' && num_str[i + 1] != '\0') {
        i++;
    }

    if (i > 0) {
        memmove(num_str, num_str + i, strlen(num_str + i) + 1);
    }

    return num_str;
}

static status_code add_numbers_column(const char *const num1, const char *const num2, 
                                      const int base, char *const result, 
                                      const size_t max_result_size) {
    if (!num1 || !num2 || !result || max_result_size == 0) {
        return STATUS_INVALID_ARGS;
    }

    if (base < 2 || base > 36) {
        return STATUS_INVALID_BASE;
    }

    if (!is_valid_number(num1, base) || !is_valid_number(num2, base)) {
        return STATUS_INVALID_NUMBER;
    }

    int len1 = strlen(num1);
    int len2 = strlen(num2);
    int max_len = (len1 > len2) ? len1 : len2;
    
    size_t temp_size = max_len + 10;
    char *temp = (char *)malloc(temp_size);
    if (!temp) {
        return STATUS_MEMORY_ERROR;
    }

    int result_pos = 0;
    int carry = 0;

    for (int i = 0; i < max_len || carry; i++) {
        int digit1 = 0;
        int digit2 = 0;

        if (i < len1) {
            digit1 = char_to_digit(num1[len1 - 1 - i]);
        }
        if (i < len2) {
            digit2 = char_to_digit(num2[len2 - 1 - i]);
        }

        int sum = digit1 + digit2 + carry;
        int digit_result = sum % base;
        carry = sum / base;

        if (result_pos >= (int)(temp_size - 1)) {
            free(temp);
            return STATUS_MEMORY_ERROR;
        }

        temp[result_pos++] = digit_to_char(digit_result);
    }

    if (result_pos + 1 > (int)max_result_size) {
        free(temp);
        return STATUS_MEMORY_ERROR;
    }

    for (int i = 0; i < result_pos; i++) {
        result[i] = temp[result_pos - 1 - i];
    }
    result[result_pos] = '\0';

    free(temp);
    return STATUS_OK;
}

static char *sum_numbers_in_base(const int base, const int count, ...) {
    if (base < 2 || base > 36 || count <= 0) {
        return NULL;
    }

    va_list args;
    va_start(args, count);

    size_t buffer_size = 1024;
    char *result = (char *)malloc(buffer_size);
    if (!result) {
        va_end(args);
        return NULL;
    }

    strcpy(result, "0");

    for (int i = 0; i < count; i++) {
        const char *next_num = va_arg(args, const char *);
        if (!next_num) {
            free(result);
            va_end(args);
            return NULL;
        }

        if (!is_valid_number(next_num, base)) {
            free(result);
            va_end(args);
            return NULL;
        }

        char *temp = (char *)malloc(buffer_size);
        if (!temp) {
            free(result);
            va_end(args);
            return NULL;
        }

        status_code status = add_numbers_column(result, next_num, base, temp, buffer_size);
        if (status != STATUS_OK) {
            free(temp);
            free(result);
            va_end(args);
            return NULL;
        }

        free(result);
        result = temp;
    }

    va_end(args);

    remove_leading_zeros(result);

    return result;
}

static void free_string_array(char **const arr, const int count) {
    if (!arr) {
        return;
    }
    for (int i = 0; i < count; i++) {
        free(arr[i]);
    }
    free(arr);
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

static status_code process_file(const char *const input_file) {
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

    char **numbers = (char **)malloc(count * sizeof(char *));
    if (!numbers) {
        fclose(in_file);
        return STATUS_MEMORY_ERROR;
    }

    for (int i = 0; i < count; i++) {
        numbers[i] = NULL;
    }

    for (int i = 0; i < count; i++) {
        char buffer[256];
        if (fscanf(in_file, "%255s", buffer) != 1) {
            free_string_array(numbers, i);
            fclose(in_file);
            return STATUS_PARSE_ERROR;
        }

        if (!is_valid_number(buffer, base)) {
            free_string_array(numbers, i);
            fclose(in_file);
            return STATUS_INVALID_NUMBER;
        }

        numbers[i] = (char *)malloc(strlen(buffer) + 1);
        if (!numbers[i]) {
            free_string_array(numbers, i);
            fclose(in_file);
            return STATUS_MEMORY_ERROR;
        }
        strcpy(numbers[i], buffer);
    }

    fclose(in_file);

    char *result = NULL;
    
    switch (count) {
        case 1:
            result = sum_numbers_in_base(base, 1, numbers[0]);
            break;
        case 2:
            result = sum_numbers_in_base(base, 2, numbers[0], numbers[1]);
            break;
        case 3:
            result = sum_numbers_in_base(base, 3, numbers[0], numbers[1], numbers[2]);
            break;
        case 4:
            result = sum_numbers_in_base(base, 4, numbers[0], numbers[1], numbers[2], numbers[3]);
            break;
        case 5:
            result = sum_numbers_in_base(base, 5, numbers[0], numbers[1], numbers[2], numbers[3], numbers[4]);
            break;
        case 6:
            result = sum_numbers_in_base(base, 6, numbers[0], numbers[1], numbers[2], numbers[3], numbers[4], numbers[5]);
            break;
        case 7:
            result = sum_numbers_in_base(base, 7, numbers[0], numbers[1], numbers[2], numbers[3], numbers[4], numbers[5], numbers[6]);
            break;
        case 8:
            result = sum_numbers_in_base(base, 8, numbers[0], numbers[1], numbers[2], numbers[3], numbers[4], numbers[5], numbers[6], numbers[7]);
            break;
        case 9:
            result = sum_numbers_in_base(base, 9, numbers[0], numbers[1], numbers[2], numbers[3], numbers[4], numbers[5], numbers[6], numbers[7], numbers[8]);
            break;
        case 10:
            result = sum_numbers_in_base(base, 10, numbers[0], numbers[1], numbers[2], numbers[3], numbers[4], numbers[5], numbers[6], numbers[7], numbers[8], numbers[9]);
            break;
        default:
            free_string_array(numbers, count);
            return STATUS_INVALID_ARGS;
    }

    if (!result) {
        free_string_array(numbers, count);
        return STATUS_MEMORY_ERROR;
    }

    printf("Base: %d\n", base);
    printf("Result: %s\n", result);
    printf("Sum of %d numbers in base %d\n", count, base);

    free(result);
    free_string_array(numbers, count);

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