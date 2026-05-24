#include "json.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Forward declarations
static json_value_t *parse_value(const char **ptr);
static json_value_t *parse_object(const char **ptr);
static json_value_t *parse_array(const char **ptr);
static json_value_t *parse_string(const char **ptr);
static json_value_t *parse_number(const char **ptr);
static json_value_t *parse_boolean(const char **ptr);
static json_value_t *parse_null(const char **ptr);

// Utility: skip whitespace
static void skip_whitespace(const char **ptr) {
    while (**ptr == ' ' || **ptr == '\t' || **ptr == '\n' || **ptr == '\r') {
        (*ptr)++;
    }
}

// Memory allocation helpers
json_value_t *json_create_null(void) {
    json_value_t *v = malloc(sizeof(json_value_t));
    if (v) v->type = JSON_NULL;
    return v;
}

json_value_t *json_create_boolean(bool b) {
    json_value_t *v = malloc(sizeof(json_value_t));
    if (v) {
        v->type = JSON_BOOLEAN;
        v->as.boolean = b;
    }
    return v;
}

json_value_t *json_create_number(double n) {
    json_value_t *v = malloc(sizeof(json_value_t));
    if (v) {
        v->type = JSON_NUMBER;
        v->as.number = n;
    }
    return v;
}

json_value_t *json_create_string(const char *s) {
    json_value_t *v = malloc(sizeof(json_value_t));
    if (v) {
        v->type = JSON_STRING;
        v->as.string = strdup(s);
    }
    return v;
}

json_value_t *json_create_array(void) {
    json_value_t *v = malloc(sizeof(json_value_t));
    if (v) {
        v->type = JSON_ARRAY;
        v->as.array.capacity = 4;
        v->as.array.length = 0;
        v->as.array.values = malloc(sizeof(json_value_t *) * v->as.array.capacity);
    }
    return v;
}

json_value_t *json_create_object(void) {
    json_value_t *v = malloc(sizeof(json_value_t));
    if (v) {
        v->type = JSON_OBJECT;
        v->as.object.head = NULL;
        v->as.object.count = 0;
    }
    return v;
}

void json_free(json_value_t *v) {
    if (!v) return;
    switch (v->type) {
        case JSON_STRING:
            free(v->as.string);
            break;
        case JSON_ARRAY:
            for (size_t i = 0; i < v->as.array.length; i++) {
                json_free(v->as.array.values[i]);
            }
            free(v->as.array.values);
            break;
        case JSON_OBJECT: {
            json_object_entry_t *entry = v->as.object.head;
            while (entry) {
                json_object_entry_t *next = entry->next;
                free(entry->key);
                json_free(entry->value);
                free(entry);
                entry = next;
            }
            break;
        }
        default:
            break;
    }
    free(v);
}

void json_array_append(json_value_t *array, json_value_t *value) {
    if (!array || array->type != JSON_ARRAY || !value) return;
    if (array->as.array.length >= array->as.array.capacity) {
        array->as.array.capacity *= 2;
        array->as.array.values = realloc(array->as.array.values, sizeof(json_value_t *) * array->as.array.capacity);
    }
    array->as.array.values[array->as.array.length++] = value;
}

json_value_t *json_array_get(const json_value_t *array, size_t index) {
    if (!array || array->type != JSON_ARRAY || index >= array->as.array.length) return NULL;
    return array->as.array.values[index];
}

void json_object_set(json_value_t *object, const char *key, json_value_t *value) {
    if (!object || object->type != JSON_OBJECT || !key || !value) return;

    // Check if key already exists
    json_object_entry_t *entry = object->as.object.head;
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            json_free(entry->value);
            entry->value = value;
            return;
        }
        entry = entry->next;
    }

    // Add new entry
    entry = malloc(sizeof(json_object_entry_t));
    entry->key = strdup(key);
    entry->value = value;
    entry->next = object->as.object.head;
    object->as.object.head = entry;
    object->as.object.count++;
}

json_value_t *json_object_get(const json_value_t *object, const char *key) {
    if (!object || object->type != JSON_OBJECT || !key) return NULL;
    json_object_entry_t *entry = object->as.object.head;
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}

// Parser implementation

json_value_t *json_parse(const char *json_string) {
    if (!json_string) return NULL;
    const char *ptr = json_string;
    skip_whitespace(&ptr);
    json_value_t *v = parse_value(&ptr);
    skip_whitespace(&ptr);
    if (v && *ptr != '\0') {
        // Trailing garbage
        json_free(v);
        return NULL;
    }
    return v;
}

static json_value_t *parse_value(const char **ptr) {
    skip_whitespace(ptr);
    switch (**ptr) {
        case 'n': return parse_null(ptr);
        case 't':
        case 'f': return parse_boolean(ptr);
        case '"': return parse_string(ptr);
        case '[': return parse_array(ptr);
        case '{': return parse_object(ptr);
        default:
            if ((**ptr >= '0' && **ptr <= '9') || **ptr == '-') {
                return parse_number(ptr);
            }
            return NULL; // Invalid JSON
    }
}

static json_value_t *parse_null(const char **ptr) {
    if (strncmp(*ptr, "null", 4) == 0) {
        *ptr += 4;
        return json_create_null();
    }
    return NULL;
}

static json_value_t *parse_boolean(const char **ptr) {
    if (strncmp(*ptr, "true", 4) == 0) {
        *ptr += 4;
        return json_create_boolean(true);
    }
    if (strncmp(*ptr, "false", 5) == 0) {
        *ptr += 5;
        return json_create_boolean(false);
    }
    return NULL;
}

static json_value_t *parse_number(const char **ptr) {
    char *endptr;
    double val = strtod(*ptr, &endptr);
    if (*ptr == endptr) return NULL;
    *ptr = endptr;
    return json_create_number(val);
}

static int parse_hex4(const char **ptr) {
    int val = 0;
    for (int i = 0; i < 4; i++) {
        char c = **ptr;
        (*ptr)++;
        val <<= 4;
        if (c >= '0' && c <= '9') val |= (c - '0');
        else if (c >= 'a' && c <= 'f') val |= (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') val |= (c - 'A' + 10);
        else return -1;
    }
    return val;
}

static json_value_t *parse_string(const char **ptr) {
    (*ptr)++; // skip opening quote
    size_t cap = 32;
    size_t len = 0;
    char *str = malloc(cap);
    if (!str) return NULL;

    while (**ptr != '"' && **ptr != '\0') {
        if (len + 4 >= cap) {
            cap *= 2;
            str = realloc(str, cap);
        }

        if (**ptr == '\\') {
            (*ptr)++;
            switch (**ptr) {
                case '"': str[len++] = '"'; break;
                case '\\': str[len++] = '\\'; break;
                case '/': str[len++] = '/'; break;
                case 'b': str[len++] = '\b'; break;
                case 'f': str[len++] = '\f'; break;
                case 'n': str[len++] = '\n'; break;
                case 'r': str[len++] = '\r'; break;
                case 't': str[len++] = '\t'; break;
                case 'u': {
                    (*ptr)++;
                    int u = parse_hex4(ptr);
                    if (u < 0) {
                        free(str);
                        return NULL; // Invalid unicode
                    }
                    // Very basic UTF-8 encoding (only handles up to U+FFFF)
                    if (u <= 0x7F) {
                        str[len++] = (char)u;
                    } else if (u <= 0x7FF) {
                        str[len++] = (char)(0xC0 | ((u >> 6) & 0x1F));
                        str[len++] = (char)(0x80 | (u & 0x3F));
                    } else {
                        str[len++] = (char)(0xE0 | ((u >> 12) & 0x0F));
                        str[len++] = (char)(0x80 | ((u >> 6) & 0x3F));
                        str[len++] = (char)(0x80 | (u & 0x3F));
                    }
                    (*ptr)--; // adjust because loop will increment
                    break;
                }
                default:
                    free(str);
                    return NULL;
            }
        } else {
            str[len++] = **ptr;
        }
        (*ptr)++;
    }

    if (**ptr == '"') {
        (*ptr)++;
    } else {
        free(str);
        return NULL; // Unclosed string
    }

    str[len] = '\0';
    json_value_t *v = json_create_string(str);
    free(str);
    return v;
}

static json_value_t *parse_array(const char **ptr) {
    (*ptr)++; // skip '['
    json_value_t *arr = json_create_array();
    skip_whitespace(ptr);
    if (**ptr == ']') {
        (*ptr)++;
        return arr;
    }

    while (1) {
        json_value_t *elem = parse_value(ptr);
        if (!elem) {
            json_free(arr);
            return NULL;
        }
        json_array_append(arr, elem);
        skip_whitespace(ptr);
        if (**ptr == ',') {
            (*ptr)++;
            skip_whitespace(ptr);
        } else if (**ptr == ']') {
            (*ptr)++;
            break;
        } else {
            json_free(arr);
            return NULL;
        }
    }
    return arr;
}

static json_value_t *parse_object(const char **ptr) {
    (*ptr)++; // skip '{'
    json_value_t *obj = json_create_object();
    skip_whitespace(ptr);
    if (**ptr == '}') {
        (*ptr)++;
        return obj;
    }

    while (1) {
        skip_whitespace(ptr);
        if (**ptr != '"') {
            json_free(obj);
            return NULL;
        }
        json_value_t *key_val = parse_string(ptr);
        if (!key_val) {
            json_free(obj);
            return NULL;
        }

        skip_whitespace(ptr);
        if (**ptr != ':') {
            json_free(key_val);
            json_free(obj);
            return NULL;
        }
        (*ptr)++; // skip ':'

        json_value_t *val = parse_value(ptr);
        if (!val) {
            json_free(key_val);
            json_free(obj);
            return NULL;
        }

        json_object_set(obj, key_val->as.string, val);
        json_free(key_val);

        skip_whitespace(ptr);
        if (**ptr == ',') {
            (*ptr)++;
        } else if (**ptr == '}') {
            (*ptr)++;
            break;
        } else {
            json_free(obj);
            return NULL;
        }
    }
    return obj;
}
