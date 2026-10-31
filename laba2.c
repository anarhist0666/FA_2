#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <locale.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_NAME_LEN 256
#define MAX_LINE_LEN 1024
#define EPSILON 1e-9

typedef enum {
    STATUS_OK = 0,
    STATUS_INVALID_ARGS = 1,
    STATUS_FILE_ERROR = 2,
    STATUS_MEMORY_ERROR = 3,
    STATUS_INVALID_FLAG = 4,
    STATUS_PARSE_ERROR = 5,
    STATUS_SAME_FILES = 6
} StatusCode;

typedef struct {
    unsigned int id;
    char name[MAX_NAME_LEN];
    char surname[MAX_NAME_LEN];
    double salary;
} Employee;

typedef struct {
    Employee *data;
    size_t count;
    size_t capacity;
} EmployeeArray;

int validate_name(const char *const str, const size_t len) {
    if (str == NULL || len == 0) {
        return 0;
    }

    for (size_t i = 0; i < len; i++) {
        const unsigned char c = (unsigned char)str[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
            return 0;
        }
    }

    return 1;
}

StatusCode validate_flag(const char *const flag, char *const direction) {
    if (flag == NULL || direction == NULL) {
        return STATUS_INVALID_ARGS;
    }

    if ((flag[0] != '-' && flag[0] != '/') || flag[1] == '\0' || flag[2] != '\0') {
        return STATUS_INVALID_FLAG;
    }

    if (flag[1] == 'a' || flag[1] == 'A') {
        *direction = 'a';
        return STATUS_OK;
    } else if (flag[1] == 'd' || flag[1] == 'D') {
        *direction = 'd';
        return STATUS_OK;
    }

    return STATUS_INVALID_FLAG;
}

StatusCode validate_input_args(const int argc, const char *const *const argv, char *const direction) {
    if (argc != 4) {
        return STATUS_INVALID_ARGS;
    }

    if (argv == NULL || argv[1] == NULL || argv[2] == NULL || argv[3] == NULL) {
        return STATUS_INVALID_ARGS;
    }

    const StatusCode flag_status = validate_flag(argv[2], direction);
    if (flag_status != STATUS_OK) {
        return flag_status;
    }

    return STATUS_OK;
}

StatusCode check_files_are_same(const char *const input_file, const char *const output_file) {
    if (input_file == NULL || output_file == NULL) {
        return STATUS_INVALID_ARGS;
    }

    if (strcmp(input_file, output_file) == 0) {
        return STATUS_SAME_FILES;
    }

    struct stat stat_input;
    struct stat stat_output;

    if (stat(input_file, &stat_input) != 0) {
        return STATUS_OK;
    }

    if (stat(output_file, &stat_output) != 0) {
        return STATUS_OK;
    }

    if (stat_input.st_dev == stat_output.st_dev && stat_input.st_ino == stat_output.st_ino) {
        return STATUS_SAME_FILES;
    }

    return STATUS_OK;
}

StatusCode init_employee_array(EmployeeArray *const arr, const size_t initial_capacity) {
    if (arr == NULL) {
        return STATUS_INVALID_ARGS;
    }

    size_t capacity = initial_capacity;
    if (capacity == 0) {
        capacity = 10;
    }

    arr->data = (Employee *)malloc(capacity * sizeof(Employee));
    if (arr->data == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    arr->count = 0;
    arr->capacity = capacity;

    return STATUS_OK;
}

void free_employee_array(EmployeeArray *const arr) {
    if (arr != NULL) {
        free(arr->data);
        arr->data = NULL;
        arr->count = 0;
        arr->capacity = 0;
    }
}

StatusCode expand_employee_array(EmployeeArray *const arr) {
    if (arr == NULL) {
        return STATUS_INVALID_ARGS;
    }

    const size_t new_capacity = arr->capacity * 2;
    Employee *const new_data = (Employee *)realloc(arr->data, new_capacity * sizeof(Employee));

    if (new_data == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    arr->data = new_data;
    arr->capacity = new_capacity;

    return STATUS_OK;
}

int is_valid_number_char(const char c) {
    return (c >= '0' && c <= '9');
}

StatusCode parse_line(const char *const line, Employee *const emp) {
    if (line == NULL || emp == NULL) {
        return STATUS_INVALID_ARGS;
    }

    if (line[0] == '-' || line[0] == '+') {
        return STATUS_PARSE_ERROR;
    }

    char *end = NULL;
    const unsigned long id_val = strtoul(line, &end, 10);

    if (*end != ' ' || id_val > UINT_MAX) {
        return STATUS_PARSE_ERROR;
    }

    emp->id = (unsigned int)id_val;

    const char *name_start = end + 1;
    const char *name_end = name_start;
    while (*name_end != ' ' && *name_end != '\0') {
        name_end++;
    }

    if (name_end == name_start || *name_end != ' ') {
        return STATUS_PARSE_ERROR;
    }

    const size_t name_len = (size_t)(name_end - name_start);
    if (name_len >= MAX_NAME_LEN) {
        return STATUS_PARSE_ERROR;
    }

    if (!validate_name(name_start, name_len)) {
        return STATUS_PARSE_ERROR;
    }

    for (size_t i = 0; i < name_len; i++) {
        emp->name[i] = name_start[i];
    }
    emp->name[name_len] = '\0';

    const char *surname_start = name_end + 1;
    const char *surname_end = surname_start;
    while (*surname_end != ' ' && *surname_end != '\0') {
        surname_end++;
    }

    if (surname_end == surname_start || *surname_end != ' ') {
        return STATUS_PARSE_ERROR;
    }

    const size_t surname_len = (size_t)(surname_end - surname_start);
    if (surname_len >= MAX_NAME_LEN) {
        return STATUS_PARSE_ERROR;
    }

    if (!validate_name(surname_start, surname_len)) {
        return STATUS_PARSE_ERROR;
    }

    for (size_t i = 0; i < surname_len; i++) {
        emp->surname[i] = surname_start[i];
    }
    emp->surname[surname_len] = '\0';

    const char *salary_start = surname_end + 1;

    if (*salary_start == '\0' || *salary_start == '-' || *salary_start == '+') {
        return STATUS_PARSE_ERROR;
    }

    if (!is_valid_number_char(*salary_start)) {
        return STATUS_PARSE_ERROR;
    }

    const char *salary_ptr = salary_start;
    int has_dot = 0;

    while (*salary_ptr != '\0') {
        if (*salary_ptr == '.') {
            if (has_dot) {
                return STATUS_PARSE_ERROR;
            }
            has_dot = 1;
        } else if (!is_valid_number_char(*salary_ptr)) {
            return STATUS_PARSE_ERROR;
        }
        salary_ptr++;
    }

    char *salary_end = NULL;
    const double salary_val = strtod(salary_start, &salary_end);

    if (salary_end != salary_ptr || salary_val < 0.0 || isnan(salary_val) || isinf(salary_val)) {
        return STATUS_PARSE_ERROR;
    }

    emp->salary = salary_val;

    return STATUS_OK;
}

StatusCode add_employee(EmployeeArray *const arr, const Employee *const emp) {
    if (arr == NULL || emp == NULL) {
        return STATUS_INVALID_ARGS;
    }

    if (arr->count >= arr->capacity) {
        const StatusCode expand_status = expand_employee_array(arr);
        if (expand_status != STATUS_OK) {
            return expand_status;
        }
    }

    arr->data[arr->count] = *emp;
    arr->count++;

    return STATUS_OK;
}

int compare_salaries(const double sal1, const double sal2) {
    const double diff = sal1 - sal2;
    if (diff > EPSILON) {
        return 1;
    } else if (diff < -EPSILON) {
        return -1;
    }
    return 0;
}

int compare_strings(const char *const str1, const char *const str2) {
    for (size_t i = 0; str1[i] != '\0' || str2[i] != '\0'; i++) {
        if (str1[i] < str2[i]) {
            return -1;
        } else if (str1[i] > str2[i]) {
            return 1;
        }
    }
    return 0;
}

int compare_employees_asc(const void *const a, const void *const b) {
    const Employee *const emp1 = (const Employee *)a;
    const Employee *const emp2 = (const Employee *)b;

    const int salary_cmp = compare_salaries(emp1->salary, emp2->salary);
    if (salary_cmp != 0) {
        return salary_cmp;
    }

    const int surname_cmp = compare_strings(emp1->surname, emp2->surname);
    if (surname_cmp != 0) {
        return surname_cmp;
    }

    const int name_cmp = compare_strings(emp1->name, emp2->name);
    if (name_cmp != 0) {
        return name_cmp;
    }

    if (emp1->id < emp2->id) {
        return -1;
    } else if (emp1->id > emp2->id) {
        return 1;
    }

    return 0;
}

int compare_employees_desc(const void *const a, const void *const b) {
    return -compare_employees_asc(a, b);
}

StatusCode read_employees_from_file(const char *const input_file, EmployeeArray *const arr) {
    if (input_file == NULL || arr == NULL) {
        return STATUS_INVALID_ARGS;
    }

    FILE *const file = fopen(input_file, "r");
    if (file == NULL) {
        return STATUS_FILE_ERROR;
    }

    char line[MAX_LINE_LEN];

    while (fgets(line, sizeof(line), file) != NULL) {
        size_t len = 0;
        while (line[len] != '\0') {
            if (line[len] == '\n' || line[len] == '\r') {
                line[len] = '\0';
                break;
            }
            len++;
        }

        if (line[0] == '\0') {
            continue;
        }

        Employee emp;
        const StatusCode parse_status = parse_line(line, &emp);

        if (parse_status != STATUS_OK) {
            fclose(file);
            return parse_status;
        }

        const StatusCode add_status = add_employee(arr, &emp);
        if (add_status != STATUS_OK) {
            fclose(file);
            return add_status;
        }
    }

    const int file_error = ferror(file);
    if (fclose(file) == EOF || file_error) {
        return STATUS_FILE_ERROR;
    }

    return STATUS_OK;
}

StatusCode write_employees_to_file(const char *const output_file, const EmployeeArray *const arr) {
    if (output_file == NULL || arr == NULL) {
        return STATUS_INVALID_ARGS;
    }

    FILE *const file = fopen(output_file, "w");
    if (file == NULL) {
        return STATUS_FILE_ERROR;
    }

    for (size_t i = 0; i < arr->count; i++) {
        const int print_result = fprintf(file, "%u %s %s %.2f\n",
                                         arr->data[i].id,
                                         arr->data[i].name,
                                         arr->data[i].surname,
                                         arr->data[i].salary);

        if (print_result < 0) {
            fclose(file);
            return STATUS_FILE_ERROR;
        }
    }

    if (fclose(file) == EOF) {
        return STATUS_FILE_ERROR;
    }

    return STATUS_OK;
}

void print_error(const StatusCode code) {
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

        case STATUS_INVALID_FLAG:
            fprintf(stderr, "Error: invalid flag\n");
            break;

        case STATUS_PARSE_ERROR:
            fprintf(stderr, "Error: invalid data format in file\n");
            break;

        case STATUS_SAME_FILES:
            fprintf(stderr, "Error: input and output files are the same\n");
            break;

        case STATUS_OK:
            break;
    }
}

int main(const int argc, char *const argv[]) {
    setlocale(LC_NUMERIC, "C");

    if (argc != 4) {
        fprintf(stderr, "Error: invalid argument count\n");
        return (int)STATUS_INVALID_ARGS;
    }

    char direction = '\0';
    const StatusCode val_status = validate_input_args(argc, (const char *const *)argv, &direction);
    if (val_status != STATUS_OK) {
        print_error(val_status);
        return (int)val_status;
    }

    const StatusCode files_check = check_files_are_same(argv[1], argv[3]);
    if (files_check != STATUS_OK) {
        print_error(files_check);
        return (int)files_check;
    }

    EmployeeArray arr;
    const StatusCode init_status = init_employee_array(&arr, 10);
    if (init_status != STATUS_OK) {
        print_error(init_status);
        return (int)init_status;
    }

    const StatusCode read_status = read_employees_from_file(argv[1], &arr);
    if (read_status != STATUS_OK) {
        print_error(read_status);
        free_employee_array(&arr);
        return (int)read_status;
    }

    if (arr.count == 0) {
        fprintf(stderr, "Warning: no employees found in file\n");
    } else {
        if (direction == 'a') {
            qsort(arr.data, arr.count, sizeof(Employee), compare_employees_asc);
        } else {
            qsort(arr.data, arr.count, sizeof(Employee), compare_employees_desc);
        }
    }

    const StatusCode write_status = write_employees_to_file(argv[3], &arr);
    if (write_status != STATUS_OK) {
        print_error(write_status);
        free_employee_array(&arr);
        return (int)write_status;
    }

    free_employee_array(&arr);
    return (int)STATUS_OK;
}