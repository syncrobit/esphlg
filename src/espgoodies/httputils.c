
#include <stdlib.h>
#include <string.h>
#include <strings.h> /* for strcasecmp */
#include <mem.h>

#include "espgoodies/common.h"
#include "espgoodies/crypto.h"
#include "espgoodies/utils.h"
#include "espgoodies/httputils.h"


static void ICACHE_FLASH_ATTR unescape_url_encoded_value(char *value);


json_t *http_parse_url_encoded(char *input) {
    char name[JSON_MAX_NAME_LEN + 1];
    char value[JSON_MAX_VALUE_LEN + 1];
    name[0] = 0;
    value[0] = 0;

    json_t *json = json_obj_new();

    if (!input) {
        return json;
    }

    char *s = input;
    int c, state = 0;
    while ((c = *s++)) {
        switch (state) {
            case 0: { /* Name */
                if (c == '=') { /* Value starts */
                    state = 1; /* Value */
                }
                else if (c == '&') { /* Field starts */
                    json_obj_append(json, name, json_str_new(value));

                    name[0] = 0;
                    value[0] = 0;
                }
                else {
                    append_max_len(name, c, JSON_MAX_NAME_LEN);
                }

                break;
            }

            case 1: { /* Value */
                if (c == '&') { /* Field starts */
                    unescape_url_encoded_value(value);

                    json_obj_append(json, name, json_str_new(value));

                    state = 0; /* Name */
                    name[0] = 0;
                    value[0] = 0;
                }
                else {
                    append_max_len(value, c, JSON_MAX_VALUE_LIST_LEN);
                }

                break;
            }
        }
    }

    /* Append the last value that hasn't yet been processed, if any */
    if (*name) {
        json_obj_append(json, name, json_str_new(value));
    }

    return json;
}

char *http_build_auth_header(char *token, char *type) {
    int len = 8 + strlen(token); /* strlen("Bearer" + " " + token) */
    char *header = malloc(len);
    snprintf(header, len, "%s %s", type, token);

    return header;
}

char *http_parse_auth_header(char *header, char *type) {
    /* Look for the first space */
    char *p = header;
    while (*p && *p != ' ') {
        p++;
    }

    if (!*p) {
        /* No space found */
        return NULL;
    }

    if (strncasecmp(header, type, p - header)) {
        /* Header does not start with given type */
        return NULL;
    }

    /* Skip all spaces */
    while (*p && *p == ' ') {
        p++;
    }

    if (!*p) {
        /* No token present */
        return NULL;
    }

    /* Everything that's left is token */
    return strdup(p);
}

bool http_decode_basic_auth(char *basic_auth, char **username, char **password) {
    char *decoded = (char *) b64_decode(basic_auth);
    if (!decoded) {
        return FALSE;
    }

    if (decoded[0] == ':') {
        /* Special case where username is empty */
        *username = strdup("");
        *password = strdup(decoded + 1);
        free(decoded);
        return TRUE;
    }

    char *token = strtok(decoded, ":");
    if (!token) {
        free(decoded);
        return FALSE;
    }
    *username = strdup(token);

    token = strtok(NULL, ":");
    if (token) {
        *password = strdup(token);
    }
    else {
        *password = strdup("");
    }

    return TRUE;
}


void unescape_url_encoded_value(char *value) {
    char *s = value, *unescaped = malloc(strlen(value) + 1);
    char hex[3] = {0, 0, 0};
    int c, p = 0, state = 0;
    
    unescaped[0] = 0;
    
    while ((c = *s)) {
        switch (state) {
            case 0: /* Outside */
                if (c == '%') {
                    state = 1; /* Percent */
                }
                else {
                    unescaped[p] = c;
                    unescaped[++p] = 0;
                }

                break;

            case 1: /* Percent */
                if (c == '%') { /* Escaped percent */
                    state = 0; /* Outside */
                    unescaped[p] = c;
                    unescaped[++p] = 0;
                }
                else if (IS_HEX(c)) {
                    state = 2; /* Hex */
                    hex[0] = c;
                }
                else {
                    state = 0; /* Outside */
                    unescaped[p] = c;
                    unescaped[++p] = 0;
                }

                break;

            case 2: /* Hex */
                if (IS_HEX(c)) {
                    hex[1] = c;
                    
                    c = strtol(hex, NULL, 16);
                    unescaped[p] = c;
                    unescaped[++p] = 0;
                }
                else {
                    unescaped[p] = c;
                    unescaped[++p] = 0;
                }

                state = 0; /* Outside */

                break;
        }
        
        s++;
    }
    
    strcpy(value, unescaped);
    free(unescaped);
}
