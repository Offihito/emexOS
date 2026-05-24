#include "css.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Utility: trim whitespace from both ends of a string (modifies the string and returns a pointer to the start)
static char *trim_whitespace(char *str) {
    if (!str) return NULL;

    // Trim leading
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }

    // Trim trailing
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }

    return str;
}

static void skip_comments(const char **ptr) {
    while (**ptr) {
        if (**ptr == '/' && *(*ptr + 1) == '*') {
            *ptr += 2;
            while (**ptr && !(**ptr == '*' && *(*ptr + 1) == '/')) {
                (*ptr)++;
            }
            if (**ptr == '*' && *(*ptr + 1) == '/') {
                *ptr += 2;
            }
        } else {
            break;
        }
    }
}

static void skip_whitespace_and_comments(const char **ptr) {
    while (**ptr) {
        if (**ptr == ' ' || **ptr == '\t' || **ptr == '\n' || **ptr == '\r') {
            (*ptr)++;
        } else if (**ptr == '/' && *(*ptr + 1) == '*') {
            skip_comments(ptr);
        } else {
            break;
        }
    }
}

css_stylesheet_t *css_parse(const char *css_string) {
    if (!css_string) return NULL;

    css_stylesheet_t *stylesheet = malloc(sizeof(css_stylesheet_t));
    if (!stylesheet) return NULL;
    stylesheet->rules = NULL;

    const char *ptr = css_string;
    css_rule_t *last_rule = NULL;

    while (*ptr) {
        skip_whitespace_and_comments(&ptr);
        if (!*ptr) break;

        // At-rules like @media are not fully supported, we just skip them for now
        if (*ptr == '@') {
            while (*ptr && *ptr != '{' && *ptr != ';') ptr++;
            if (*ptr == '{') {
                int braces = 1;
                ptr++;
                while (*ptr && braces > 0) {
                    if (*ptr == '{') braces++;
                    else if (*ptr == '}') braces--;
                    ptr++;
                }
            } else if (*ptr == ';') {
                ptr++;
            }
            continue;
        }

        // Parse selector
        const char *selector_start = ptr;
        while (*ptr && *ptr != '{') {
            ptr++;
        }

        if (!*ptr) break; // EOF before '{'

        size_t selector_len = ptr - selector_start;
        char *selector_raw = malloc(selector_len + 1);
        strncpy(selector_raw, selector_start, selector_len);
        selector_raw[selector_len] = '\0';
        char *selector = trim_whitespace(selector_raw);

        if (strlen(selector) == 0) {
            free(selector_raw);
            ptr++; // skip '{'
            // skip until '}'
            while (*ptr && *ptr != '}') ptr++;
            if (*ptr == '}') ptr++;
            continue;
        }

        css_rule_t *rule = malloc(sizeof(css_rule_t));
        rule->selector = strdup(selector);
        rule->declarations = NULL;
        rule->next = NULL;
        free(selector_raw);

        if (!stylesheet->rules) {
            stylesheet->rules = rule;
        } else {
            last_rule->next = rule;
        }
        last_rule = rule;

        ptr++; // skip '{'

        // Parse declarations
        css_declaration_t *last_decl = NULL;
        while (*ptr && *ptr != '}') {
            skip_whitespace_and_comments(&ptr);
            if (*ptr == '}') break;

            const char *prop_start = ptr;
            while (*ptr && *ptr != ':' && *ptr != '}') ptr++;
            if (*ptr == '}') break;

            size_t prop_len = ptr - prop_start;
            char *prop_raw = malloc(prop_len + 1);
            strncpy(prop_raw, prop_start, prop_len);
            prop_raw[prop_len] = '\0';
            char *property = trim_whitespace(prop_raw);

            ptr++; // skip ':'

            const char *val_start = ptr;
            while (*ptr && *ptr != ';' && *ptr != '}') ptr++;

            size_t val_len = ptr - val_start;
            char *val_raw = malloc(val_len + 1);
            strncpy(val_raw, val_start, val_len);
            val_raw[val_len] = '\0';
            char *value = trim_whitespace(val_raw);

            if (strlen(property) > 0) {
                css_declaration_t *decl = malloc(sizeof(css_declaration_t));
                decl->property = strdup(property);
                decl->value = strdup(value);
                decl->next = NULL;

                if (!rule->declarations) {
                    rule->declarations = decl;
                } else {
                    last_decl->next = decl;
                }
                last_decl = decl;
            }

            free(prop_raw);
            free(val_raw);

            if (*ptr == ';') ptr++;
        }

        if (*ptr == '}') ptr++;
    }

    return stylesheet;
}

void css_free(css_stylesheet_t *stylesheet) {
    if (!stylesheet) return;

    css_rule_t *rule = stylesheet->rules;
    while (rule) {
        css_rule_t *next_rule = rule->next;

        free(rule->selector);

        css_declaration_t *decl = rule->declarations;
        while (decl) {
            css_declaration_t *next_decl = decl->next;
            free(decl->property);
            free(decl->value);
            free(decl);
            decl = next_decl;
        }

        free(rule);
        rule = next_rule;
    }

    free(stylesheet);
}

css_rule_t *css_get_rule(const css_stylesheet_t *stylesheet, const char *selector) {
    if (!stylesheet || !selector) return NULL;

    css_rule_t *rule = stylesheet->rules;
    while (rule) {
        if (strcmp(rule->selector, selector) == 0) {
            return rule;
        }
        rule = rule->next;
    }

    return NULL;
}

const char *css_get_value(const css_rule_t *rule, const char *property) {
    if (!rule || !property) return NULL;

    css_declaration_t *decl = rule->declarations;
    while (decl) {
        if (strcmp(decl->property, property) == 0) {
            return decl->value;
        }
        decl = decl->next;
    }

    return NULL;
}
