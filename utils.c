#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <sys/stat.h>
#include "utils.h"

int string_to_int(const char *string) {
    // Check for null pointer
    if (string == NULL) {
        return 0;
    }
    
    // Skip leading whitespace
    while (*string && isspace(*string)) {
        string++;
    }
    
    // Check for empty string
    if (*string == '\0') {
        return 0;
    }
    
    // Handle sign
    int sign = 1;
    if (*string == '-') {
        sign = -1;
        string++;
    } else if (*string == '+') {
        string++;
    }
    
    // Check for no digits after sign
    if (!*string || !isdigit(*string)) {
        return 0;
    }
    
    // Convert digits
    long value = 0;
    while (*string && isdigit(*string)) {
        // Check for overflow
        if (value > INT_MAX / 10 || (value == INT_MAX / 10 && (*string - '0') > INT_MAX % 10)) {
            if (sign == 1) {
                return INT_MAX;
            } else {
                return INT_MIN;
            }
        }
        
        value = value * 10 + (*string - '0');
        string++;
    }
    
    // Check if we have non-digit characters at the end
    if (*string != '\0') {
        return 0;
    }
    
    return (int)(value * sign);
}