/*
 * mod_bmx_example.c: Apache Monitoring Example Module
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
 * mod_bmx_example is an example BMX Plugin module that demonstrates
 * proper use of the BMX Query handler and on-the-fly bean generation
 * using all supported BMX Bean Property types.
 *
 * $Id: mod_bmx_example.c,v 1.1 2007/11/05 22:15:44 aaron Exp $
 */

#include "httpd.h"
#include "http_config.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "http_request.h"

#include "apr_optional.h"
#include "mod_bmx.h"

/* --------------------------------------------------------------------
 * Global definitions
 * -------------------------------------------------------------------- */

/** The prototype for our module structure, defined at the bottom */
module AP_MODULE_DECLARE_DATA bmx_example_module;

#if AP_MODULE_MAGIC_AT_LEAST(20100606,0)
APLOG_USE_MODULE(bmx_example);
#endif

/** The BMX Bean domain used for all beans exported by mod_bmx_example. */
#define BMX_EXAMPLE_DOMAIN "mod_bmx_example"

/** The single BMX Bean exported by mod_bmx_example. */
static struct bmx_bean *bmx_example_bean;
/** The single BMX Objectname used by the bmx_example_bean object. */
static struct bmx_objectname *bmx_example_objectname;

/* --------------------------------------------------------------------
 * Hook processing
 * -------------------------------------------------------------------- */

/**
 * This hook is called by mod_bmx each time it receives an BMX Query.
 * Each BMX Plugin gets a chance to respond to the query. It is the
 * responsibility of each plugin to decide if it should respond to
 * the query, though the bmx_check_constrains() utility routine helps
 * determine if the received Query applies to this module's bean(s).
 * @param r The request received by Apache.
 * @param query The BMX Query received by mod_bmx.
 * @param print_bean_fn A function that this routine can use to send
 *        our BMX Bean back to the client in reponse to the BMX Query.
 * @returns OK if a bean was printed, DECLINED if this plugin decided not
 *          to respond to the Query, or any other APR error if there is
 *          some fatal and unrecoverable internal error.
 */
static int bmx_example_query_hook(request_rec *r,
                                  const struct bmx_objectname *query,
                                  bmx_bean_print print_bean_fn)
{
    if (bmx_check_constraints(query, bmx_example_objectname)) {
        print_bean_fn(r, bmx_example_bean);
        return OK;
    } else {
        return DECLINED;
    }
}

/**
 * The standard Apache 'pre_config' hook that is used to create our
 * global BMX Bean and global BMX Objectname once at startup (along
 * with some sample Bean Properties), and also is used to hook in to
 * the BMX 'query_hook' routine which is called on each BMX Query.
 */
static int bmx_example_pre_config(apr_pool_t *pconf, apr_pool_t *plog,
                                  apr_pool_t *ptemp)
{
    /* create the objectname: "mod_bmx_example:Type=BMXExampleModule" */
    bmx_objectname_create(&bmx_example_objectname, BMX_EXAMPLE_DOMAIN, pconf);
    apr_table_setn(bmx_example_objectname->props, "Type", "BMXExampleModule");
    apr_table_setn(bmx_example_objectname->props, "Something", "Else");

    /* create the bean */
    bmx_bean_create(&bmx_example_bean, bmx_example_objectname, pconf);

    /* add properties to the bean */
    bmx_bean_prop_add(bmx_example_bean,
        bmx_property_boolean_create("SomeBool", 1, pconf));
    bmx_bean_prop_add(bmx_example_bean,
        bmx_property_byte_create("SomeByte", 32, pconf));
    bmx_bean_prop_add(bmx_example_bean,
        bmx_property_int16_create("SomeINT16", -42, pconf));
    bmx_bean_prop_add(bmx_example_bean,
        bmx_property_uint16_create("SomeUINT16", 42, pconf));
    bmx_bean_prop_add(bmx_example_bean,
        bmx_property_int32_create("SomeINT32", -123456, pconf));
    bmx_bean_prop_add(bmx_example_bean,
        bmx_property_uint32_create("SomeUINT32", 123456, pconf));
    bmx_bean_prop_add(bmx_example_bean,
        bmx_property_int64_create("SomeINT64", APR_INT64_C(-1231231231231232037), pconf));
    bmx_bean_prop_add(bmx_example_bean,
        bmx_property_uint64_create("SomeUINT64", APR_UINT64_C(1231231231231232037), pconf));
    bmx_bean_prop_add(bmx_example_bean,
        bmx_property_float_create("SomeFloat", 3.1415f, pconf));
    bmx_bean_prop_add(bmx_example_bean,
        bmx_property_double_create("SomeDouble", 2.71828183f, pconf));
    bmx_bean_prop_add(bmx_example_bean,
        bmx_property_string_create("SomeString", "This is a string", pconf));

    /* FIXME: give an example of opaque bmx_property creation */

    APR_OPTIONAL_HOOK(bmx, query_hook, bmx_example_query_hook, NULL, NULL,
                      APR_HOOK_MIDDLE);
    return OK;
}

/* --------------------------------------------------------------------
 * Module internals
 * -------------------------------------------------------------------- */

/**
 * Standard Apache 'register_hooks' callback that is used to hook into
 * the 'pre_config' hook, implemented above.
 */
static void bmx_example_register_hooks(apr_pool_t *p)
{
    ap_hook_pre_config(bmx_example_pre_config, NULL, NULL, APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA bmx_example_module =
{
    STANDARD20_MODULE_STUFF,
    NULL,                            /* per-directory config creator */
    NULL,                            /* dir config merger */
    NULL,                            /* server config creator */
    NULL,                            /* server config merger */
    NULL,                            /* command table */
    bmx_example_register_hooks,      /* set up other request processing hooks */
};

