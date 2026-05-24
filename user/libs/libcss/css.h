#pragma once

#include <stddef.h>

// A CSS property-value pair (e.g., "color: red")
typedef struct css_declaration_t {
    char *property;
    char *value;
    struct css_declaration_t *next;
} css_declaration_t;

// A CSS rule containing a selector and a list of declarations
typedef struct css_rule_t {
    char *selector;
    css_declaration_t *declarations;
    struct css_rule_t *next;
} css_rule_t;

// A CSS stylesheet containing a list of rules
typedef struct css_stylesheet_t {
    css_rule_t *rules;
} css_stylesheet_t;

// Parse a CSS string into a stylesheet structure
css_stylesheet_t *css_parse(const char *css_string);

// Free a stylesheet and all its contents
void css_free(css_stylesheet_t *stylesheet);

// Look up a specific rule by its selector (returns the first matching rule)
css_rule_t *css_get_rule(const css_stylesheet_t *stylesheet, const char *selector);

// Look up a specific property's value within a rule
const char *css_get_value(const css_rule_t *rule, const char *property);
