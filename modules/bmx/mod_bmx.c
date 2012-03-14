/*
 * mod_bmx.c: Apache Monitoring Core Module
 *
 * See the NOTICE file distributed with this work for information
 * regarding copyright ownership. This file is licensed to You under
 * the Apache License, Version 2.0 (the "License"); you may not use
 * this file except in compliance with the License.  You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * $Id: mod_bmx.c,v 1.1 2007/11/05 22:15:44 aaron Exp $
 *
 * TBD: description
 */

#include "httpd.h"
#include "http_config.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "http_request.h"

#include "apr_strings.h"
#include "apr_optional.h"
#include "mod_bmx.h"

#if AP_MODULE_MAGIC_AT_LEAST(20100606,0)
APLOG_USE_MODULE(bmx);
#endif

/* --------------------------------------------------------------------
 * Global definitions
 * -------------------------------------------------------------------- */

/**
 * The prototype for our module structure, defined at the bottom.
 */
module AP_MODULE_DECLARE_DATA bmx_module;

/**
 * Special bmx_objectname object which is used to query all domains.
 */
struct bmx_objectname bmx_query_all;

/**
 * BMX Handler string, for SetHandler configuration directives.
 */
#define BMX_HANDLER "bmx-handler"

/* --------------------------------------------------------------------
 * Configuration handling routines
 * -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
 * External Utility routines
 * -------------------------------------------------------------------- */

/**
 * Print the BMX Bean Property to a string. If the property data type is
 * a built-in type then we convert directly, otherwise we use the
 * user-defined callback to perform the string convesion.
 */
static char *property_print(apr_pool_t *p, struct bmx_property *prop)
{
    switch (prop->value_type) {
    case BMX_BOOLEAN:
        if (prop->value.boolean)
            return "true";
        else
            return "false";
    case BMX_BYTE:
        return apr_psprintf(p, "%du", prop->value.byte);
    case BMX_INT16:
        return apr_psprintf(p, "%d", prop->value.int16);
    case BMX_UINT16:
        return apr_psprintf(p, "%du", prop->value.uint16);
    case BMX_INT32:
        return apr_psprintf(p, "%d", prop->value.int32);
    case BMX_UINT32:
        return apr_psprintf(p, "%du", prop->value.uint32);
    case BMX_INT64:
        return apr_psprintf(p, "%" APR_INT64_T_FMT, prop->value.int64);
    case BMX_UINT64:
        return apr_psprintf(p, "%" APR_UINT64_T_FMT, prop->value.uint64);
    case BMX_FLOAT:
        return apr_psprintf(p, "%f", prop->value.f);
    case BMX_DOUBLE:
        return apr_psprintf(p, "%lf", prop->value.d);
    case BMX_STRING:
        return prop->value.s;
    case BMX_OTHER:
        return prop->print_fn(p, prop->value.o);
    case BMX_NULL:
    default:
        return "";
    }
}

/**
 * Create a Boolean Bean Property.
 */
struct bmx_property *bmx_property_boolean_create(const char *key, int boolean,
                                                 apr_pool_t *p)
{
    struct bmx_property *ret = apr_pcalloc(p, sizeof(*ret));
    ret->key = apr_pstrdup(p, key);
    ret->value_type = BMX_BOOLEAN;
    ret->value.boolean = boolean;
    return ret;
}

/**
 * Create a Byte (8-bit unsigned) Bean Property.
 */
struct bmx_property *bmx_property_byte_create(const char *key,
                                              apr_byte_t byte,
                                              apr_pool_t *p)
{
    struct bmx_property *ret = apr_pcalloc(p, sizeof(*ret));
    ret->key = apr_pstrdup(p, key);
    ret->value_type = BMX_BYTE;
    ret->value.byte = byte;
    return ret;
}

/**
 * Create a signed 16-bit short Bean Property.
 */
struct bmx_property *bmx_property_int16_create(const char *key,
                                               apr_int16_t int16,
                                               apr_pool_t *p)
{
    struct bmx_property *ret = apr_pcalloc(p, sizeof(*ret));
    ret->key = apr_pstrdup(p, key);
    ret->value_type = BMX_INT16;
    ret->value.int16 = int16;
    return ret;
}

/**
 * Create an unsigned 16-bit short Bean Property.
 */
struct bmx_property *bmx_property_uint16_create(const char *key,
                                                apr_uint16_t uint16,
                                                apr_pool_t *p)
{
    struct bmx_property *ret = apr_pcalloc(p, sizeof(*ret));
    ret->key = apr_pstrdup(p, key);
    ret->value_type = BMX_UINT16;
    ret->value.uint16 = uint16;
    return ret;
}

/**
 * Create a signed 32-bit integer Bean Property.
 */
struct bmx_property *bmx_property_int32_create(const char *key,
                                               apr_int32_t int32,
                                               apr_pool_t *p)
{
    struct bmx_property *ret = apr_pcalloc(p, sizeof(*ret));
    ret->key = apr_pstrdup(p, key);
    ret->value_type = BMX_INT32;
    ret->value.int32 = int32;
    return ret;
}

/**
 * Create an unsigned 32-bit integer Bean Property.
 */
struct bmx_property *bmx_property_uint32_create(const char *key,
                                                apr_uint32_t uint32,
                                                apr_pool_t *p)
{
    struct bmx_property *ret = apr_pcalloc(p, sizeof(*ret));
    ret->key = apr_pstrdup(p, key);
    ret->value_type = BMX_UINT32;
    ret->value.uint32 = uint32;
    return ret;
}

/**
 * Create a signed 64-bit long integer Bean Property.
 */
struct bmx_property *bmx_property_int64_create(const char *key,
                                               apr_int64_t int64,
                                               apr_pool_t *p)
{
    struct bmx_property *ret = apr_pcalloc(p, sizeof(*ret));
    ret->key = apr_pstrdup(p, key);
    ret->value_type = BMX_INT64;
    ret->value.int64 = int64;
    return ret;
}

/**
 * Create an unsigned 64-bit long integer Bean Property.
 */
struct bmx_property *bmx_property_uint64_create(const char *key,
                                                apr_uint64_t uint64,
                                                apr_pool_t *p)
{
    struct bmx_property *ret = apr_pcalloc(p, sizeof(*ret));
    ret->key = apr_pstrdup(p, key);
    ret->value_type = BMX_UINT64;
    ret->value.uint64 = uint64;
    return ret;
}

/**
 * Create a floating point Bean Property.
 */
struct bmx_property *bmx_property_float_create(const char *key, float f,
                                               apr_pool_t *p)
{
    struct bmx_property *ret = apr_pcalloc(p, sizeof(*ret));
    ret->key = apr_pstrdup(p, key);
    ret->value_type = BMX_FLOAT;
    ret->value.f = f;
    return ret;
}

/**
 * Create a double-precision floating point Bean Property.
 */
struct bmx_property *bmx_property_double_create(const char *key,
                                                double d, apr_pool_t *p)
{
    struct bmx_property *ret = apr_pcalloc(p, sizeof(*ret));
    ret->key = apr_pstrdup(p, key);
    ret->value_type = BMX_DOUBLE;
    ret->value.d = d;
    return ret;
}

/**
 * Create a string (standard C-style string: null-terminated character array)
 * Bean Property.
 */
struct bmx_property *bmx_property_string_create(const char *key,
                                                const char *s,
                                                apr_pool_t *p)
{
    struct bmx_property *ret = apr_pcalloc(p, sizeof(*ret));
    ret->key = apr_pstrdup(p, key);
    ret->value_type = BMX_STRING;
    ret->value.s = apr_pstrdup(p, s);
    return ret;
}

/**
 * Create a user-deined Bean Property.
 * @param key The key name to give this property.
 * @param value A pointer to the opaque value for this property.
 * @param print_fn A user-defined function for printing this opaque
 *        data value as a string.
 * @param p The pool from where this bean should be allocated.
 * @returns A new user-defined bmx_property bean object pointer.
 */
struct bmx_property *bmx_property_generic_create(const char *key,
                                                 void *value,
                                                 bmx_property_print print_fn,
                                                 apr_pool_t *p)
{
    struct bmx_property *ret = apr_pcalloc(p, sizeof(*ret));
    ret->key = apr_pstrdup(p, key);
    ret->value_type = BMX_OTHER;
    ret->value.o = value;
    ret->print_fn = print_fn;
    return ret;
}

/**
 * Create a new BMX Bean, given the objectname and pool.
 * @param bean A pointer to an bmx_bean pointer where the address of the
 *        newly created bean will be stored.
 * @param objectname The BMX Objectname for this new bean.
 * @param pool The pool from where this bean should be allocated.
 */
void bmx_bean_create(struct bmx_bean **bean,
                     struct bmx_objectname *objectname,
                     apr_pool_t *pool)
{
    struct bmx_bean *ret = apr_pcalloc(pool, sizeof(*ret));
    bmx_bean_init(ret, objectname);
    (*bean) = ret;
}

/**
 * Initialize a bean that has already been allocated with the given
 * BMX Objectname.
 * @param bean The pre-allocated bean object to initialize.
 * @param objectname The BMX Objectname for this new bean.
 */
void bmx_bean_init(struct bmx_bean *bean,
                   struct bmx_objectname *objectname)
{
    bean->objectname = objectname;
    APR_RING_INIT(&(bean->bean_props), bmx_property, link);
}

/**
 * Add a new BMX Property to an BMX Bean.
 * @param bean The bean where the property will be added.
 * @param prop The property to add.
 */
void bmx_bean_prop_add(struct bmx_bean *bean, struct bmx_property *prop)
{
    APR_RING_INSERT_TAIL(&(bean->bean_props), prop, bmx_property, link);
}

/**
 * Create a new BMX Objectname from the given BMX Bean Domain.
 * @param objectname A pointer to an BMX Objectname pointer where the
 *        new BMX Objectname will be stored.
 * @param domain The domain for the new objectname.
 * @param pool The pool from where this objectname should be allocated.
 */
void bmx_objectname_create(struct bmx_objectname **objectname,
                           const char *domain, apr_pool_t *pool)
{
    struct bmx_objectname *ret = apr_pcalloc(pool, sizeof(*ret));
    ret->domain = apr_pstrdup(pool, domain);
    ret->props = apr_table_make(pool, 0);
    (*objectname) = ret;
}

/**
 * Fetch the objectname for a given bean.
 * @param bean The bean from which to fetch the objectname.
 * @returns The objectname from the given bean.
 */
const struct bmx_objectname *bmx_bean_get_objectname(struct bmx_bean *bean)
{
    return bean->objectname;
}

/**
 * A private internal data structure that is used while iterating through
 * the table of bean properties while looking for matching key-value pairs.
 */
struct bmx_check_constraints_data {
    /** A table of bean properties */
    apr_table_t *beanprops;
    /** True if all properties have matched the original key-value pair. */
    int all_match;
};

static int bmx_check_constraints_iterator(void *rec, const char *key,
                                          const char *value)
{
    struct bmx_check_constraints_data *data
        = (struct bmx_check_constraints_data *)rec;
    const char *beanprop_value = apr_table_get(data->beanprops, key);
    if (beanprop_value && (strcmp(value, beanprop_value) == 0)) {
        data->all_match = 1;
        return TRUE;
    } else {
        data->all_match = 0;
        return FALSE;
    }
}

int bmx_check_constraints(const struct bmx_objectname *query,
                          const struct bmx_objectname *objectname)
{
    struct bmx_check_constraints_data data = { objectname->props, 0 };

    /* Check if we're doing a wildcard query */
    if (query == BMX_QUERY_ALL)
        return TRUE;

    /* Fail if the domains don't match. */
    if (strcmp(query->domain, objectname->domain))
        return FALSE;

    if (!query->props)
        return TRUE; /* domins match and wildcard properties */

    /* The domains do match, now check each query property */
    apr_table_do(bmx_check_constraints_iterator, &data, query->props, NULL);

    return data.all_match;
}

/* --------------------------------------------------------------------
 * Utility routines
 * -------------------------------------------------------------------- */

#define MAX_DOMAIN_LEN 128
#define MAX_CONSTRAINTS_LEN 1024
#define QUERY_FORMAT "query=%128[^:]:%1024s"
#define ALL_QUERY "query=*:*"
static int parse_query(request_rec *r, struct bmx_objectname **query)
{
    char domain[MAX_DOMAIN_LEN];
    char constraints[MAX_CONSTRAINTS_LEN];
    static const char *format = QUERY_FORMAT;

    /* no query args? return everything */
    if (!r->args || r->args[0] == '\0') {
        *query = BMX_QUERY_ALL;
        goto out;
    }

    /* shortcut for full-query matches */
    if (0 == strncmp(r->args, ALL_QUERY, sizeof(ALL_QUERY))) {
        *query = BMX_QUERY_ALL;
        goto out;
    }

    /* parse out the domain and constraints */
    if ((2 == sscanf(r->args, format, domain, constraints))) {
        struct bmx_objectname *ret = apr_pcalloc(r->pool, sizeof(*ret));
        if (domain[0] == '\0')
            ret->domain = "*";
        else
            ret->domain = apr_pstrdup(r->pool, domain);

        /* tokenize the constraints */
        if (constraints[0] == '*' && (constraints[1] == '\0'
                                      || constraints[1] == '&'))
            ret->props = NULL; /* no constraints */
        else if (constraints[0] != '\0') {
            size_t size = 1;
            char *last = constraints;
            char *token;
            /* count the number of tokens */
            while (*last++ != '\0') {
                if (*last == ',')
                    size++;
            }
            ret->props = apr_table_make(r->pool, size);
            /* pull out each token */
            for (token = apr_strtok(constraints, ",", &last);
                 token && token[0] != '\0';
                 token = apr_strtok(NULL, ",", &last)) {
                char *c = strchr(token, '=');
                char *key = (token[0] == '\0') ? "" : token;
                if (c && *c != '\0') {
                    char *value;
                    char t = *c;
                    *c = '\0';
                    value = (c[1] == '\0') ? "" : c + 1;
                    apr_table_set(ret->props, key, value);
                    *c = t; /* put back the '=' */
                } else {
                    apr_table_set(ret->props, key, "");
                }
            }
        }
        *query = ret;
    } else {
        return APR_EINVAL; /* invalid parameters */
    }

out:
    return APR_SUCCESS;
}

/* --------------------------------------------------------------------
 * Hook processing
 * -------------------------------------------------------------------- */

/*
struct bmx_objectname_prop_print_data {
    request_rec *r;
    int first;
};

static int bmx_objectname_prop_print(void *rec, const char *key,
                                     const char *value)
{
    struct bmx_objectname_prop_print_data *data
        = (struct bmx_objectname_prop_print_data *)rec;
    if (data->first) {
        data->first = 0;
        (void)ap_rprintf(data->r, ":%s=%s", key, value);
    } else {
        (void)ap_rprintf(data->r, ",%s=%s", key, value);
    }
    return 1;
}
*/

struct bmx_objectname_strlen_data {
    int first;
    apr_size_t sz;
};

static int bmx_objectname_strlen_iterator(void *rec, const char *key,
                                          const char *value)
{
    struct bmx_objectname_strlen_data *data
        = (struct bmx_objectname_strlen_data *)rec;

    /* ignore anything with missing keys or values */
    if (key && value) {
        data->sz += strlen(key);
        if (data->first) {
            data->first = 0;
            data->sz += 1; /* for the '=' */
        } else {
            data->sz += 2; /* for the ',' and the '=' */
        }
        data->sz += strlen(value);
    }
    return 1;
}

apr_size_t bmx_objectname_strlen(const struct bmx_objectname *on)
{
    apr_size_t sz = 0;
    if (on) {
        struct bmx_objectname_strlen_data data = { 1, 0 };
        if (on->domain) {
            sz += strlen(on->domain) + 1; /* one extra for the ':' */
        } else {
            sz += sizeof("*:") - 1;
        }
        if (on->props) {
            (void)apr_table_do(bmx_objectname_strlen_iterator, &data,
                               on->props, NULL);
            if (data.sz == 0) {
                sz += sizeof("*") - 1;
            } else {
                sz += data.sz;
            }
        } else {
            sz += sizeof("*") - 1;
        }
    }
    return sz;
}

struct bmx_objectname_str_data {
    int first;
    char *buf;
    apr_size_t buflen;
};

static int bmx_objectname_str_iterator(void *rec, const char *key,
                                       const char *value)
{
    struct bmx_objectname_str_data *data
        = (struct bmx_objectname_str_data *)rec;
    char *p = data->buf;

    /* ignore anything with missing keys or values */
    if (key && value) {
        if (data->first) {
            data->first = 0;
            p += apr_snprintf(data->buf, data->buflen, "%s=%s", key, value);
        } else {
            p += apr_snprintf(data->buf, data->buflen, ",%s=%s", key, value);
        }

        data->buflen -= p - data->buf;
        data->buf = p;
    }
    return 1;
}

apr_size_t bmx_objectname_str(const struct bmx_objectname *on,
                              char *buf, apr_size_t buflen)
{
    char *p = buf;
    if (on) {
        if (on->domain) {
            p += apr_snprintf(p, buflen, "%s:", on->domain);
        } else {
            *p++ = '*';
            *p++ = ':';
        }
        if (on->props) {
            struct bmx_objectname_str_data data = { 1, p, buflen - (p - buf) };
            (void)apr_table_do(bmx_objectname_str_iterator, &data,
                               on->props, NULL);
            if (data.buf == p) { /* none were printed */
                *p++ = '*';
            } else {
                p = data.buf;
            }
        } else {
            *p++ = '*';
        }
    }
    return p - buf;
}

/**
 * Called by other modules to print their "jmx beans" to the response in
 * whatever format was requested by the client.
 */
static apr_status_t bmx_bean_print_text_plain(request_rec *r,
                                              const struct bmx_bean *bean)
{
    apr_size_t objectname_strlen = bmx_objectname_strlen(bean->objectname) + 1;
    char *objectname_str = apr_palloc(r->pool, objectname_strlen);
    (void)bmx_objectname_str(bean->objectname, objectname_str,
                             objectname_strlen);
    (void)ap_rputs("Name: ", r);
    (void)ap_rputs(objectname_str, r);
    (void)ap_rputs("\n", r);

    /* for each element in bean->bean_properties, print it */
    if (!APR_RING_EMPTY(&(bean->bean_props), bmx_property, link)) {
        struct bmx_property *p = NULL;
        const char *value;
        for (p = APR_RING_FIRST(&(bean->bean_props));
             p != APR_RING_SENTINEL(&(bean->bean_props), bmx_property, link);
             p = APR_RING_NEXT(p, link)) {
            (void)ap_rputs(p->key, r);
            (void)ap_rputs(": ", r);
            value = property_print(r->pool, p);
            if (value)
                (void)ap_rputs(value, r);
            (void)ap_rputs("\n", r);
        }
    }
    (void)ap_rputs("\n", r);
    return APR_SUCCESS;
}

/* Implement 'bmx_run_query_hook'. This hook is used by mod_bmx plugins
 * to respond to queries. Implementations must call bean_print_fn() callback
 * for each bean they wish to return to the client. */
APR_IMPLEMENT_OPTIONAL_HOOK_RUN_ALL(bmx, BMX, int, query_hook,
                (request_rec *r, const struct bmx_objectname *query,
                 bmx_bean_print print_bean_fn),
                (r, query, print_bean_fn),
                OK, DECLINED)


static int bmx_handler(request_rec *r)
{
    apr_status_t rv;
    struct bmx_objectname *query = NULL;

    /* Determine if we are the handler for this request. */
    if (r->handler && strcmp(r->handler, BMX_HANDLER)) {
        return DECLINED;
    }

    /* Disallow any method except GET */
    if (r->method_number != M_GET) {
        return DECLINED;
    }

    /* FIXME: check that the Accept: header has a mime-type we can accept */
    /* FIXME: if no valid types in Accept: header, return code 415 */
    /* FIXME: return the correct mime-type */
    ap_set_content_type(r, "text/plain");

    if (r->header_only) {
        return OK;
    }

    rv = parse_query(r, &query);
    if (rv != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_CRIT, rv, r, "Failed to parse query");
        return HTTP_BAD_REQUEST;
    }

    /* FIXME: allow other bmx_bean_print functions other than text/plain */
    rv = bmx_run_query_hook(r, query, bmx_bean_print_text_plain);
    if (rv != OK) {
        ap_log_rerror(APLOG_MARK, APLOG_CRIT, rv, r, "Error running "
                      "bmx_run_query_hook, BMX Query failed");
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    return OK;
}

/* --------------------------------------------------------------------
 * Module internals
 * -------------------------------------------------------------------- */

static int bmx_pre_config(apr_pool_t *pconf, apr_pool_t *plog,
                          apr_pool_t *ptemp)
{
    ap_add_version_component(pconf, "mod_bmx/" MODBMX_VERSION);
    return OK;
}

static void bmx_register_hooks(apr_pool_t *p)
{
    ap_hook_pre_config(bmx_pre_config, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_handler(bmx_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA bmx_module =
{
    STANDARD20_MODULE_STUFF,
    NULL,                            /* per-directory config creator */
    NULL,                            /* dir config merger */
    NULL,                            /* server config creator */
    NULL,                            /* server config merger */
    NULL,                            /* command table */
    bmx_register_hooks,              /* set up other request processing hooks */
};

