#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    JSON_NULL,
    JSON_BOOLEAN,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} json_type_t;

struct json_value_t;

typedef struct json_object_entry_t {
    char *key;
    struct json_value_t *value;
    struct json_object_entry_t *next;
} json_object_entry_t;

typedef struct json_array_t {
    struct json_value_t **values;
    size_t length;
    size_t capacity;
} json_array_t;

typedef struct json_object_t {
    json_object_entry_t *head;
    size_t count;
} json_object_t;

typedef struct json_value_t {
    json_type_t type;
    union {
        bool boolean;
        double number;
        char *string;
        json_array_t array;
        json_object_t object;
    } as;
} json_value_t;

// Parse a JSON string. Returns NULL on failure.
json_value_t *json_parse(const char *json_string);

// Free a JSON value and all its children.
void json_free(json_value_t *value);

// Helper functions to create JSON values
json_value_t *json_create_null(void);
json_value_t *json_create_boolean(bool b);
json_value_t *json_create_number(double n);
json_value_t *json_create_string(const char *s);
json_value_t *json_create_array(void);
json_value_t *json_create_object(void);

// Array functions
void json_array_append(json_value_t *array, json_value_t *value);
json_value_t *json_array_get(const json_value_t *array, size_t index);

// Object functions
void json_object_set(json_value_t *object, const char *key, json_value_t *value);
json_value_t *json_object_get(const json_value_t *object, const char *key);
