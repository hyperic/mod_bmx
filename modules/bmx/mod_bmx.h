/*
 * mod_bmx.h: Apache Monitoring Core Module
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
 * $Id: mod_bmx.h,v 1.1 2007/11/05 22:15:44 aaron Exp $
 *
 * TBD: description
 */

#ifndef MOD_BMX_H
#define MOD_BMX_H

#include "apr_general.h" /* stringify */

#define MODBMX_COPYRIGHT \
  "Copyright 2012 VMware, Inc."

#define MODBMX_VERSION_MAJOR  0
#define MODBMX_VERSION_MINOR  9
#define MODBMX_VERSION_SUBVER 3
#define MODBMX_VERSION_DEV    0

#if MODBMX_VERSION_DEV
#define MODBMX_VERSION_DEVSTR "-dev"
#else
#define MODBMX_VERSION_DEVSTR ""
#endif

#define MODBMX_REVISION      APR_STRINGIFY(MODBMX_VERSION_MAJOR) \
                         "." APR_STRINGIFY(MODBMX_VERSION_MINOR) \
                         "." APR_STRINGIFY(MODBMX_VERSION_SUBVER)
#define MODBMX_VERSION       MODBMX_REVISION MODBMX_VERSION_DEVSTR

#ifndef VERSION_ONLY

#include "httpd.h"
#include "ap_config.h"
#include "apr_tables.h"
#include "apr_ring.h"

#if !defined(WIN32)
#define BMX_DECLARE(type)            type
#define BMX_DECLARE_NONSTD(type)     type
#define BMX_DECLARE_DATA
#elif defined(BMX_DECLARE_STATIC)
#define BMX_DECLARE(type)            type __stdcall
#define BMX_DECLARE_NONSTD(type)     type
#define BMX_DECLARE_DATA
#elif defined(BMX_DECLARE_EXPORT)
#define BMX_DECLARE(type)            __declspec(dllexport) type __stdcall
#define BMX_DECLARE_NONSTD(type)     __declspec(dllexport) type
#define BMX_DECLARE_DATA             __declspec(dllexport)
#else
#define BMX_DECLARE(type)            __declspec(dllimport) type __stdcall
#define BMX_DECLARE_NONSTD(type)     __declspec(dllimport) type
#define BMX_DECLARE_DATA             __declspec(dllimport)
#endif

struct bmx_property;

/**
 * This callback is called to convert the value part of a user-defined
 * bmx_property into a string.
 */
typedef char *(*bmx_property_print)(apr_pool_t *pool, void *opaque);

/**
 * An bmx_property is a key/value pair that is contained within an
 * bmx_bean. The keys must all be strings, but the values can by
 * of ap redefined primitive type (boolean, integer, long, float, double,
 * or string) or can be a user-defined type.
 */
/* FIXME: change these to APR types */
struct bmx_property {
    /** The unique name given to this Bean Property. */
    char *key;
    /** The data type of the value contained within this Bean Property. */
    enum value_type {
        BMX_NULL,
        BMX_BOOLEAN,
        BMX_BYTE,
        BMX_INT16,
        BMX_UINT16,
        BMX_INT32,
        BMX_UINT32,
        BMX_INT64,
        BMX_UINT64,
        BMX_FLOAT,
        BMX_DOUBLE,
        BMX_STRING,
        BMX_OTHER
    } value_type;
    /** The actual data value for this Bean Property. */
    union value {
        int boolean;
        apr_byte_t byte;
        apr_int16_t int16;
        apr_uint16_t uint16;
        apr_int32_t int32;
        apr_uint32_t uint32;
        apr_int64_t int64;
        apr_uint64_t uint64;
        float f;
        double d;
        char *s;
        void *o; /* opaque data (user-defined bmx_property value) */
    } value;
    /**
     * An optional user-defined function that can print this Bean Property
     * as a string. Only called if the value_type is BMX_OTHER.
     */
    bmx_property_print print_fn;
    /**
     * An APR_RING link to the next and previous bmx_properties associated
     * with a Bean.
     */
    APR_RING_ENTRY(bmx_property) link;
};

/**
 * A special type representing the head of a ring of bmx_property objects.
 */
APR_RING_HEAD(bmx_properties, bmx_property);

/**
 * Create a Boolean Bean Property.
 */
BMX_DECLARE(struct bmx_property *) bmx_property_boolean_create(
                                       const char *key, int b,
                                       apr_pool_t *p);
/**
 * Create a Byte (8-bit unsigned) Bean Property.
 */
BMX_DECLARE(struct bmx_property *) bmx_property_byte_create(
                                       const char *key, apr_byte_t b,
                                       apr_pool_t *p);
/**
 * Create a signed 16-bit short Bean Property.
 */
BMX_DECLARE(struct bmx_property *) bmx_property_int16_create(
                                       const char *key, apr_int16_t i,
                                       apr_pool_t *p);
/**
 * Create an unsigned 16-bit short Bean Property.
 */
BMX_DECLARE(struct bmx_property *) bmx_property_uint16_create(
                                       const char *key, apr_uint16_t ui,
                                       apr_pool_t *p);
/**
 * Create a signed 32-bit integer Bean Property.
 */
BMX_DECLARE(struct bmx_property *) bmx_property_int32_create(
                                       const char *key, apr_int32_t i,
                                       apr_pool_t *p);
/**
 * Create an unsigned 32-bit integer Bean Property.
 */
BMX_DECLARE(struct bmx_property *) bmx_property_uint32_create(
                                       const char *key, apr_uint32_t ui,
                                       apr_pool_t *p);
/**
 * Create a signed 64-bit long integer Bean Property.
 */
BMX_DECLARE(struct bmx_property *) bmx_property_int64_create(
                                       const char *key, apr_int64_t i,
                                       apr_pool_t *p);
/** 
 * Create an unsigned 64-bit long integer Bean Property.
 */ 
BMX_DECLARE(struct bmx_property *) bmx_property_uint64_create(
                                       const char *key, apr_uint64_t ui,
                                       apr_pool_t *p);
/** 
 * Create a floating point Bean Property.
 */ 
BMX_DECLARE(struct bmx_property *) bmx_property_float_create(
                                       const char *key, float f,
                                       apr_pool_t *p);
/**
 * Create a double-precision floating point Bean Property.
 */
BMX_DECLARE(struct bmx_property *) bmx_property_double_create(
                                       const char *key, double d,
                                       apr_pool_t *p);
/**
 * Create a string (standard C-style string: null-terminated character array)
 * Bean Property.
 */ 
BMX_DECLARE(struct bmx_property *) bmx_property_string_create(
                                       const char *key, const char *s,
                                       apr_pool_t *p);
/**
 * Create a user-deined Bean Property.
 * @param key The key name to give this property.
 * @param value A pointer to the opaque value for this property.
 * @param print_fn A user-defined function for printing this opaque
 *        data value as a string.
 * @param p The pool from where this bean should be allocated.
 * @returns A new user-defined bmx_property bean object pointer.
 */
BMX_DECLARE(struct bmx_property *) bmx_property_generic_create(
                                       const char *key,
                                       void *value,
                                       bmx_property_print print_fn,
                                       apr_pool_t *p);

/**
 * An BMX Objectname represents the domain of the bean (the name) and a set
 * of key/value properties. Each domain should identify one or more beans
 * with common data, and each instance of a bean domain should have
 * a unique set of key/value property pairs.
 */
struct bmx_objectname {
    char *domain;
    apr_table_t *props;
};

/**
 * Special bmx_objectname object that represents a wildcard query.
 */
extern struct bmx_objectname BMX_DECLARE_DATA bmx_query_all;
#define BMX_QUERY_ALL (&bmx_query_all)

/**
 * Create an bmx_objectname with the given domain.
 */
BMX_DECLARE(void) bmx_objectname_create(struct bmx_objectname **objectname,
                                        const char *domain, apr_pool_t *pool);

/**
 * Check if the given query matches the given objectname and return
 * non-zero if true, otherwise return zero.
 */
BMX_DECLARE(int) bmx_check_constraints(const struct bmx_objectname *query,
                                       const struct bmx_objectname *objectname);

/**
 * Count how long the objectname would be if converted to a string.
 */
BMX_DECLARE(apr_size_t) bmx_objectname_strlen(const struct bmx_objectname *on);

/**
 * Convert the objectname to a string, and store it into the buf string
 * which is of size buflen.
 */
BMX_DECLARE(apr_size_t) bmx_objectname_str(const struct bmx_objectname *on,
                                           char *buf, apr_size_t buflen);


/**
 * An BMX Bean contains an BMX Objectname and a set of BMX Bean Properties.
 * Each BMX Bean represents a set of related data metrics which is generated
 * by the server in response to an BMX Query.
 */
struct bmx_bean {
    struct bmx_objectname *objectname;
    struct bmx_properties bean_props;
};

/**
 * Create an bmx_bean of the given objectname.
 */
BMX_DECLARE(void) bmx_bean_create(struct bmx_bean **bean,
                                  struct bmx_objectname *objectname,
                                  apr_pool_t *pool);

/**
 * Initialize a pre-allocated bmx_bean with the given objectname.
 */
BMX_DECLARE(void) bmx_bean_init(struct bmx_bean *bean,
                                struct bmx_objectname *objectname);

/**
 * Add a property to an bmx_bean.
 */
BMX_DECLARE(void) bmx_bean_prop_add(struct bmx_bean *bean,
                                    struct bmx_property *prop);

/**
 * Fetch the objectname for the given bean.
 */
BMX_DECLARE(const struct bmx_objectname *)bmx_bean_get_objectname(
                                              struct bmx_bean *bean);

/**
 * Callback definition used by bmx plugins to print the contents of
 * bean back to the querying client. This interface allows mod_bmx
 * to support multiple output formats without requiring bmx plugins
 * to be aware of the formats.
 */
typedef apr_status_t (*bmx_bean_print)(request_rec *r,
                                       const struct bmx_bean *bean);

/**
 * Hook that is implemented by other modules that which to respond to
 * bmx queries.
 * @param r The request_rec struct representing this request.
 * @param query The query that the implemented should respond to.
 * @param bean_print_fn A callback function to use when printing a bean.
 */
APR_DECLARE_EXTERNAL_HOOK(bmx, BMX, int, query_hook,
                          (request_rec *r,
                           const struct bmx_objectname *query,
                           bmx_bean_print print_bean_fn))

#endif /* !defined (VERSION_ONLY) */

#endif /* MOD_BMX_H */


