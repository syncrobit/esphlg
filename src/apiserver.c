
#include <stdlib.h>
#include <string.h>
#include <strings.h> /* for strcasecmp */
#include <mem.h>
#include <c_types.h>
#include <user_interface.h>

#include "espgoodies/common.h"
#include "espgoodies/crypto.h"
#include "espgoodies/httpclient.h"
#include "espgoodies/httpserver.h"
#include "espgoodies/httputils.h"
#include "espgoodies/html.h"
#include "espgoodies/jwt.h"

#include "api.h"
#include "common.h"
#include "config.h"
#include "device.h"
#include "apiserver.h"


#define MAX_PARALLEL_HTTP_REQ       4
#define HTTP_SERVER_REQUEST_TIMEOUT 4
#define MIN_HTTP_FREE_MEM           4096  /* At least 4k of free heap to serve an HTTP request */

#define JSON_CONTENT_TYPE "application/json; charset=utf-8"
#define HTML_CONTENT_TYPE "text/html; charset=utf-8"

#define RESPOND_UNAUTHENTICATED() respond_error(conn, 401, "authentication-required");


static httpserver_context_t  http_contexts[MAX_PARALLEL_HTTP_REQ];
static char                 *extra_header_names[] = {"ESP-Free-Memory", NULL};
static char                 *extra_header_names_unauth[] = {"ESP-Free-Memory", "WWW-Authenticate", NULL};
static char                 *extra_header_value_unauth = "Basic realm=\"Helium Lite Gateway (leave username empty)\"";


static void ICACHE_FLASH_ATTR *on_tcp_conn(struct espconn *conn);
static void ICACHE_FLASH_ATTR  on_tcp_recv(struct espconn *conn, httpserver_context_t *hc, uint8 *data, int len);
static void ICACHE_FLASH_ATTR  on_tcp_sent(struct espconn *conn, httpserver_context_t *hc);
static void ICACHE_FLASH_ATTR  on_tcp_disc(struct espconn *conn, httpserver_context_t *hc);

static void ICACHE_FLASH_ATTR  on_invalid_http_request(struct espconn *conn);
static void ICACHE_FLASH_ATTR  on_http_request_timeout(struct espconn *conn);
static void ICACHE_FLASH_ATTR  on_http_request(
                                   struct espconn *conn,
                                   int method,
                                   char *path,
                                   char *query,
                                   char *header_names[],
                                   char *header_values[],
                                   int header_count,
                                   char *body
                               );

#define respond_error_field(conn, status, error, field)   respond_error_extra(conn, status, error, "field", field)
#define respond_error_header(conn, status, error, header) respond_error_extra(conn, status, error, "header", header)


void *on_tcp_conn(struct espconn *conn) {
    /* Find first free http context slot */
    httpserver_context_t *hc = NULL;
    int i;

    for (i = 0; i < MAX_PARALLEL_HTTP_REQ; i++) {
        if (http_contexts[i].req_state == HTTP_STATE_IDLE) {
            hc = http_contexts + i;
            break;
        }
    }

    if (!hc) {
        DEBUG_APISERVER_CONN(conn, "too many parallel HTTP requests");
        respond_error(conn, 503, "busy");
        return NULL;
    }

    uint32 free_mem = system_get_free_heap_size();
    if (free_mem < MIN_HTTP_FREE_MEM) {
        DEBUG_APISERVER_CONN(conn, "low memory (%d bytes available), rejecting HTTP request", free_mem);
        respond_error(conn, 503, "busy");
        return NULL;
    }

    hc->req_state = HTTP_STATE_NEW;

    /* For debugging purposes */
    memcpy(hc->ip, conn->proto.tcp->remote_ip, 4);
    hc->port = conn->proto.tcp->remote_port;

    httpserver_setup_connection(
        hc,
        conn,
        (http_invalid_callback_t) on_invalid_http_request,
        (http_timeout_callback_t) on_http_request_timeout,
        (http_request_callback_t) on_http_request
    );

    return hc;
}

void on_tcp_recv(struct espconn *conn, httpserver_context_t *hc, uint8 *data, int len) {
    if (!hc) {
        DEBUG_APISERVER_CONN(conn, "ignoring data from rejected client");
        return;
    }

    int i;
    for (i = 0; i < len; i++) {
        httpserver_parse_req_char(hc, data[i]);
    }
}

void on_tcp_sent(struct espconn *conn, httpserver_context_t *hc) {
    tcp_disconnect(conn);
}

void on_tcp_disc(struct espconn *conn, httpserver_context_t *hc) {
    if (!hc) {
        return;
    }

    /* See if the connection is asynchronous and is currently waiting for a response */
    if (api_conn_equal(conn)) {
        DEBUG_APISERVER_CONN(conn, "canceling async response to closed connection");
        api_conn_reset();
    }

    httpserver_context_reset(hc);
}


/* HTTP request/response handling */

void on_invalid_http_request(struct espconn *conn) {
    DEBUG_APISERVER_CONN(conn, "ignoring invalid request");

    respond_error(conn, 400, "malformed-request");
}

void on_http_request_timeout(struct espconn *conn) {
    DEBUG_APISERVER_CONN(conn, "closing connection due to request timeout");

    tcp_disconnect(conn);
}

void on_http_request(
    struct espconn *conn,
    int method,
    char *path,
    char *query,
    char *header_names[],
    char *header_values[],
    int header_count,
    char *body
) {
    json_t *request_json = NULL;
    json_t *query_json = http_parse_url_encoded(query);
    json_t *response_json = NULL;
    char *jwt_str = NULL;
    char *basic_str = NULL;
    jwt_t *jwt = NULL;
    int i;
    char *authorization = NULL;

    if (api_conn_busy()) {
        DEBUG_APISERVER_CONN(conn, "api busy");
        respond_error(conn, 503, "busy");
        goto done;
    }

    DEBUG_APISERVER_CONN(
        conn,
        "processing request %s %s",
        method == HTTP_METHOD_GET ? "GET" :
        method == HTTP_METHOD_POST ? "POST" :
        method == HTTP_METHOD_PUT ? "PUT" :
        method == HTTP_METHOD_PATCH ? "PATCH" :
        method == HTTP_METHOD_DELETE ? "DELETE" : "OTHER",
        path
    );

    if (method == HTTP_METHOD_POST || method == HTTP_METHOD_PATCH || method == HTTP_METHOD_PUT) {
        if (body) {
            request_json = json_parse(body);
            if (!request_json) {
                /* Invalid JSON */
                DEBUG_APISERVER_CONN(conn, "invalid JSON");
                respond_error(conn, 400, "malformed-body");
                goto done;
            }
        }
        else {
            request_json = json_obj_new();
        }
    }

    /* Automatically grant access in setup mode */
    if (system_setup_mode_active()) {
        goto skip_auth;
    }

    /* Look for authorization header */
    for (i = 0; i < header_count; i++) {
        if (!strcasecmp(header_names[i], "Authorization")) {
            authorization = header_values[i];
            break;
        }
    }

    if (!authorization) {
        if (!device_password[0]) {
            DEBUG_APISERVER_CONN(conn, "granting access due to empty password");
            goto skip_auth;
        }

        DEBUG_APISERVER_CONN(conn, "missing authorization header");
        RESPOND_UNAUTHENTICATED();
        goto done;
    }

    /* Parse Authorization header */
    jwt_str = http_parse_auth_header(authorization, "Bearer");
    basic_str = http_parse_auth_header(authorization, "Basic");
    if (jwt_str) {
        /* Parse & validate JWT token */
        jwt = jwt_parse(jwt_str);
        if (!jwt) {
            DEBUG_APISERVER_CONN(conn, "invalid JWT");
            RESPOND_UNAUTHENTICATED();
            goto done;
        }

        if (jwt->alg != JWT_ALG_HS256) {
            DEBUG_APISERVER_CONN(conn, "unknown JWT algorithm");
            RESPOND_UNAUTHENTICATED();
            goto done;
        }

        /* Validate JWT signature */
        if (!jwt_validate(jwt_str, jwt->alg, device_password)) {
            DEBUG_APISERVER_CONN(conn, "invalid JWT signature");
            RESPOND_UNAUTHENTICATED();
            goto done;
        }

        DEBUG_APISERVER_CONN(conn, "JWT valid, request authenticated");

        free(jwt_str);
        jwt_str = NULL;
    }
    else if (basic_str) {
        char *username, *password;
        if (!http_decode_basic_auth(basic_str, &username, &password)) {
            DEBUG_APISERVER_CONN(conn, "invalid basic auth");
            RESPOND_UNAUTHENTICATED();
            goto done;
        }

        /* username is ignored */
        if (strncmp(password, device_password, DEVICE_PASSWORD_LEN)) {
            DEBUG_APISERVER_CONN(conn, "invalid basic auth password");
            RESPOND_UNAUTHENTICATED();
            goto done;
        }

        free(password);
        free(username);
        free(basic_str);
        basic_str = NULL;
    }
    else {
        DEBUG_APISERVER_CONN(conn, "invalid authorization header");
        RESPOND_UNAUTHENTICATED();
        goto done;
    }

    skip_auth:

    api_conn_set(conn);

    int code;
    response_json = api_call_handle(method, path, query_json, request_json, &code);

    /* Serve the HTML page only in setup mode and on any 404 */
    if (code == 404 && method == HTTP_METHOD_GET) {
        json_free(response_json);
        api_conn_reset();

        uint32 html_len;
        uint8 *html = html_load(&html_len);
        if (html) {
            respond_html(conn, 200, html, html_len);
            free(html);
        }
        else {
            respond_html(conn, 500, (uint8 *) "Error", 5);
        }
    }
    else if (response_json) {
        respond_json(conn, code, response_json);
        api_conn_reset();
    }

    done:

    free(jwt_str);
    jwt_free(jwt);

    free(basic_str);

    json_free(query_json);
    json_free(request_json);
}

void api_server_init(void) {
    int i;
    for (i = 0; i < MAX_PARALLEL_HTTP_REQ; i++) {
        http_contexts[i].slot_index = i;
        httpserver_context_reset(http_contexts + i);
    }

    DEBUG_APISERVER("http server can handle %d parallel requests", MAX_PARALLEL_HTTP_REQ);

    tcp_server_init(
        API_SERVER_TCP_PORT,
        (tcp_conn_cb_t) on_tcp_conn,
        (tcp_recv_cb_t) on_tcp_recv,
        (tcp_sent_cb_t) on_tcp_sent,
        (tcp_disc_cb_t) on_tcp_disc
    );

    httpclient_set_user_agent("espHLG " FW_VERSION);
    httpserver_set_name(device_name);
    httpserver_set_request_timeout(HTTP_SERVER_REQUEST_TIMEOUT);
}

void respond_json(struct espconn *conn, int status, json_t *json) {
    char *body;
    uint8 *response;

#if defined(_DEBUG) && defined(_DEBUG_APISERVER)
    int free_mem_before_dump = system_get_free_heap_size();
#endif

    body = json_dump_r(json, /* free_mode = */ JSON_FREE_EVERYTHING);

#if defined(_DEBUG) && defined(_DEBUG_APISERVER)
    int free_mem_after_dump = system_get_free_heap_size();
#endif

    int len = strlen(body);
    if (status == 204) {
        len = 0;  /* 204 No Content */
    }

    static char free_mem_str[16];
    snprintf(free_mem_str, 16, "%d", system_get_free_heap_size());
    char *extra_header_values[] = {free_mem_str, NULL, NULL};
    if (status == 401) {
        extra_header_values[1] = extra_header_value_unauth;
    }
    response = httpserver_build_response(
        status, JSON_CONTENT_TYPE,
        status == 401 ? extra_header_names_unauth : extra_header_names,
        extra_header_values,
        /* header_count = */ status == 401 ? 2 : 1,
        (uint8 *) body,
        &len
    );

    if (status >= 400) {
        DEBUG_APISERVER_CONN(conn, "responding with status %d: %s", status, body);
    }
    else {
        DEBUG_APISERVER_CONN(conn, "responding with status %d", status);
    }

    tcp_send(conn, response, len, /* free on sent = */ TRUE);

#if defined(_DEBUG) && defined(_DEBUG_APISERVER)
    int free_mem_after_send = system_get_free_heap_size();

    /* Show free memory after sending the response */
    DEBUG_APISERVER(
        "free memory: before dump=%d, after dump=%d, after send=%d",
        free_mem_before_dump,
        free_mem_after_dump,
        free_mem_after_send
    );
#endif
}

void respond_error(struct espconn *conn, int status, char *error) {
    json_t *json = json_obj_new();
    json_obj_append(json, "error", json_str_new(error));

    respond_json(conn, status, json);
}

void respond_html(struct espconn *conn, int status, uint8 *html, int len) {
    uint8 *response;

#if defined(_DEBUG) && defined(_DEBUG_APISERVER)
    int free_mem_before_dump = system_get_free_heap_size();
#endif

#if defined(_DEBUG) && defined(_DEBUG_APISERVER)
    int free_mem_after_dump = system_get_free_heap_size();
#endif

    static char *header_names[] = {"Content-Encoding"};
    static char *header_values[] = {"gzip"};

    response = httpserver_build_response(
        status,
        HTML_CONTENT_TYPE,
        header_names,
        header_values,
        1,
        html,
        &len
    );

    DEBUG_APISERVER_CONN(conn, "responding with status %d", status);

    tcp_send(conn, response, len, /* free_on_sent = */ TRUE);

#if defined(_DEBUG) && defined(_DEBUG_APISERVER)
    int free_mem_after_send = system_get_free_heap_size();

    /* Show free memory after sending the response */
    DEBUG_APISERVER(
        "free memory: before dump=%d, after dump=%d, after send=%d",
        free_mem_before_dump,
        free_mem_after_dump,
        free_mem_after_send
    );
#endif
}
