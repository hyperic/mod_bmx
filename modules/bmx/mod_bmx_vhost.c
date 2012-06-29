/*
 * mod_bmx_vhost.c: Apache Monitoring VHost Module
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

/*
 * This module was derived from Covalent's mod_snmp.c module.
 * FIXME: adhere to any Covalent licensing requirements here
 */

/**
 * BMX VHost Module
 *
 * This module produces vhost-specific metrics as well as server-wide metrics, across
 * various time ranges.
 *
 * Basic Design of mod_bmx_vhost:
 *
 * 1) Metrics are recorded for each vhost, plus one extra for system-wide
 *    totals.
 * 2) The metrics maintained span 3 time ranges:
 *      - forever
 *      - since last server start/restart
 *      - since last server graceful restart
 * 3) The metrics are all stored in a DBM file, so they can survive
 *    crashes and restarts. (We may move the ephemeral metrics, such as
 *    "since last graceful" into SHM for efficiency.)
 *
 * Basic Use Cases:
 * 1) Find mod_bmx_vhost metrics for a particular vhost:
 *    - http://localhost/bmx?query=mod_bmx_vhost:ServerName=example.com
 * 2) Find metrics for all servers since last start:
 *    - http://localhost/bmx?mod_bmx_vhost:Type=since-restart
 * 3) Find all mod_bmx_vhost metrics:
 *    - http://localhost/bmx?mod_bmx_vhost:*
 * 4) Reset all statistics: Delete the DBM file and restart Apache.
 *
 * $Id: mod_bmx_vhost.c,v 1.1 2007/11/05 22:15:44 aaron Exp $
 */

/**
 * Implementation Details:
 * 1) We rely on apr_dbm_t for DBM files.
 * 2) We use a global mutex to protect the DBM (in addition to the DBM's own
 *    mutex protections).
 * 3) We must record metrics on each hit. This happens during the logging
 *    phase of the server, which can happen before the complete response has
 *    been sent to the client. This means that congested I/O subsystem on
 *    the mod_bmx_vhost server can slow server responses.
 */

#include "httpd.h"
#include "http_config.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "http_request.h"
#include "ap_listen.h"
#include "ap_mpm.h"
#include "scoreboard.h"

#ifdef AP_NEED_SET_MUTEX_PERMS
#include "unixd.h"

#if MODULE_MAGIC_NUMBER_MAJOR >= 20081201
#define unixd_config ap_unixd_config
#define unixd_set_global_mutex_perms ap_unixd_set_global_mutex_perms
#endif
#endif

#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "apr_optional.h"
#include "apr_strings.h"
#include "apr_dbm.h"
#include "apr_global_mutex.h"
#include "mod_bmx.h"

#include "mod_status.h"

/* --------------------------------------------------------------------
 * Global definitions
 * -------------------------------------------------------------------- */

/** The prototype for our module structure, defined at the bottom. */
module AP_MODULE_DECLARE_DATA bmx_vhost_module;

#if AP_MODULE_MAGIC_AT_LEAST(20100606,0)
APLOG_USE_MODULE(bmx_vhost);
#endif

/** The default BMX Domain exported by this BMX Plugin. */
#define BMX_VHOST_DOMAIN "mod_bmx_vhost"

/** The default DB filename where persistent data is stored */
#define DBM_FNAME "logs/bmx_vhost.db"
/** The default DB lock filename used to protect access to the DB file */
#define DBMLOCK_FNAME "logs/bmx_vhost.db.lock"

#ifndef DEFAULT_TIME_FORMAT
/** The default time format used by this module */
#define DEFAULT_TIME_FORMAT "%A, %d-%b-%Y %H:%M:%S %Z"
#endif

/**
 * The string applied to any wildcard ports. We don't use asterisk (*)
 * here so as to avoid conflicts with the BMX query syntax.
 */
#define ANY_PORT "_ANY_"

#define BMX_VHOST_INFO_TYPE "info"

/**
 * The name of the DBM file where we store all persistent mod_bmx_vhost data.
 * There is only one global DBM filename for all Apache children.
 */
static char *dbm_fname;
/**
 * The name of the lock file used to protect access to the DBM file.
 * There is only one global DBM lock filename for all Apache children.
 */
static char *dbmlock_fname;
/**
 * The lock used to protect access to the DBM file. This lock is associated
 * with the lock filename above.
 */
static apr_global_mutex_t *dbmlock;
/**
 * The DBM file used to store persistent data.
 */
static apr_dbm_t *dbm;

/**
 * The server-level module config for mod_bmx_vhost contains one reusable
 * objectname for each type of metric lifetime.
 */
struct bmx_vhost_scfg {
    /**
     * An BMX Objectname for this VHost that persists forever (or until the DBM
     * file is removed).
     */
    struct bmx_objectname *forever;
    /**
     * An BMX Objectname for this VHost that contains data collected since
     * the last time Apache was started.
     */
    struct bmx_objectname *since_start;
    /**
     * An BMX Objectname for this VHost that contains data collected since
     * the last time Apache was re-started.
     */
    struct bmx_objectname *since_restart;

    /**
     * A special Vhost Info bean that contains configuration data for this
     * VHost. This data is not persisted, since it is created from the
     * current runtime Apache configuration.
     */
    struct bmx_bean vhost_info;

    /**
     * The DBM key that identifies the record where this VHost's data is
     * stored in the DBM file.
     */
    apr_datum_t key;
};

/**
 * The global BMX Vhost Server Config. Unlike all other VHosts, this object
 * is not associated with a VHost entry so we must store it globally.
 */
static struct bmx_vhost_scfg *global_scfg;

/**
 * The metrics that are recorded for each VHost and for each Timespan.
 */
struct vhost_timespan {
    /** The number of bytes received from GET requests */
    apr_off_t InBytesGET;
    /** The number of bytes received from HEAD requests */
    apr_off_t InBytesHEAD;
    /** The number of bytes received from POST requests */
    apr_off_t InBytesPOST;
    /** The number of bytes received from PUT requests */
    apr_off_t InBytesPUT;

    /** The number of GET requests received. */
    apr_uint64_t InRequestsGET;
    /** The number of HEAD requests received. */
    apr_uint64_t InRequestsHEAD;
    /** The number of POST requests received. */
    apr_uint64_t InRequestsPOST;
    /** The number of PUT requests received. */
    apr_uint64_t InRequestsPUT;

    /** The number of bytes returned with 200 status codes */
    apr_off_t OutBytes200;
    /** The number of bytes returned with 301 status codes */
    apr_off_t OutBytes301;
    /** The number of bytes returned with 302 status codes */
    apr_off_t OutBytes302;
    /** The number of bytes returned with 401 status codes */
    apr_off_t OutBytes401;
    /** The number of bytes returned with 403 status codes */
    apr_off_t OutBytes403;
    /** The number of bytes returned with 404 status codes */
    apr_off_t OutBytes404;
    /** The number of bytes returned with 500 status codes */
    apr_off_t OutBytes500;

    /** The number of responses returned with 200 status codes */
    apr_uint64_t OutResponses200;
    /** The number of responses returned with 301 status codes */
    apr_uint64_t OutResponses301;
    /** The number of responses returned with 302 status codes */
    apr_uint64_t OutResponses302;
    /** The number of responses returned with 401 status codes */
    apr_uint64_t OutResponses401;
    /** The number of responses returned with 403 status codes */
    apr_uint64_t OutResponses403;
    /** The number of responses returned with 404 status codes */
    apr_uint64_t OutResponses404;
    /** The number of responses returned with 500 status codes */
    apr_uint64_t OutResponses500;

    /** The total number of bytes received in all requests */
    apr_off_t InLowBytes;
    /** The total number of bytes returned in all responses */
    apr_off_t OutLowBytes;

    /** The total number of requests received */
    apr_uint64_t InRequests;
    /** The total number of responses returned */
    apr_uint64_t OutResponses;

    /** The time when the server started */
    apr_time_t StartTime;
};

/**
 * This record is stored in the DBM for each VHost, and contains the set
 * of metrics for each of the supported timespans.
 */
struct vhost_data {
    struct vhost_timespan forever;
    struct vhost_timespan since_start;
    struct vhost_timespan since_restart;
};

/** The default prefix for each DBM key used in mod_bmx_vhost */
#define KEY_PREFIX "bmx_vhost"
/** The 3 types of vhost metrics supported */
enum vhost_type {
    /** Stats collected for all time */
    FOREVER,
    /** Stats collected since last Apache start */
    SINCE_START,
    /** Stats collected since last Apache restart */
    SINCE_RESTART,
    __N_VHOST_TYPES
};
/** String names for the three types of vhost metrics supported */
char *vhost_type_names[__N_VHOST_TYPES] = {
    "forever",
    "since-start",
    "since-restart",
};

/* --------------------------------------------------------------------
 * Configuration handling routines
 * -------------------------------------------------------------------- */

/**
 * Set the name of the DBM file where we store our persistent data.
 */
static const char *set_dbm_fname(cmd_parms *cmd, void *mconfig,
                                 const char *arg)
{
    dbm_fname = ap_server_root_relative(cmd->pool, arg);
    return NULL;
}

/**
 * Set the name of the lock we use to protect the DBM file.
 */
static const char *set_dbmlock_fname(cmd_parms *cmd, void *mconfig,
                                     const char *arg)
{
    dbmlock_fname = ap_server_root_relative(cmd->pool, arg);
    return NULL;
}

/* --------------------------------------------------------------------
 * Utility routines
 * -------------------------------------------------------------------- */

/**
 * Create a new VHost Server Config BMX Objectname for the given
 * vhost_type and hostname and port.
 */
static int create_scfg_objectname(apr_pool_t *p,
                                  struct bmx_objectname **objectname,
                                  enum vhost_type vhost_type,
                                  const char *hostname,
                                  short port)
{
    int rv = 0;

    bmx_objectname_create(objectname, BMX_VHOST_DOMAIN, p);
    apr_table_set((*objectname)->props, "Type", vhost_type_names[vhost_type]);
    apr_table_set((*objectname)->props, "Host", hostname);
    apr_table_set((*objectname)->props, "Port",
                   port ? apr_psprintf(p, "%d", port) : ANY_PORT);

    return rv;
}

/** Name for the special Global VHost Bean */
#define GLOBAL_SERVER_NAME "_GLOBAL_"
/** Port used for the special Global VHost Bean */
#define GLOBAL_PORT 0

/**
 * Create a string representing all the Listen addresses used by this
 * particular VHost.
 * @param p The pool where the string is to be allocated.
 * @param s The server_rec representing this VHost.
 * @returns A string representation of the Listen addresses used by this VHost.
 */
static char *vhost_listen_addresses_str(apr_pool_t *p, server_rec *s)
{
    apr_ssize_t slen = 0;
    char *str = NULL, *pstr;
    struct server_addr_rec *sar;
    int first = 1;

    for (sar = s->addrs; sar; sar = sar->next) {
        if (sar->host_addr) {
            struct apr_sockaddr_t *addr = sar->host_addr;
            if (addr->hostname && addr->port) {
                if (first)
                    first = 0;
                else
                    slen += sizeof(",") - 1;
                /* 5 is the largest size for a port (a short int) */
                slen += strlen(addr->hostname) + sizeof(":") - 1 + 5;
            }
        }
    }

    if (slen == 0)
        return "";

    pstr = str = apr_palloc(p, slen + 1);
    first = 1;

    for (sar = s->addrs; sar; sar = sar->next) {
        if (sar->host_addr) {
            struct apr_sockaddr_t *addr = sar->host_addr;
            if (addr->hostname && addr->port) {
                if (first) {
                    first = 0;
                    pstr += sprintf(pstr, "%s:%u", addr->hostname, addr->port);
                } else {
                    pstr += sprintf(pstr, ",%s:%u",
                                    addr->hostname, addr->port);
                }
            }
        }
    }

    return str;
}

/**
 * Create a string representing all the ServerAlias values used by this VHost.
 * @param p The pool where the string is to be allocated.
 * @param s The server_rec representing this VHost.
 * @returns A string representing of the ServerAlias addresses used by
 *          this VHost.
 */
static char *vhost_server_aliases_str(apr_pool_t *p, server_rec *s)
{
    apr_array_header_t *allnames = NULL;
    if (s->names && s->wild_names)
        allnames = apr_array_append(p, s->names, s->wild_names);
    else if (s->names)
        allnames = s->names;
    else if (s->wild_names)
        allnames = s->wild_names;
    else
        return "";
    return apr_array_pstrcat(p, allnames, ',');
}

/**
 * Create an instance of an BMX VHost Info Bean.
 * @param p The pool out of which to allocate this Bean.
 * @param vhost_info A pointer to an BMX Bean object that will be
 *        initialized and associated with this VHost.
 * @param s The server_rec containing this VHost's data.
 * @param hostname The hostname to use for this VHost.
 * @param port The port to use for this VHost.
 */
static void create_vhost_info_bean(apr_pool_t *p,
                                   struct bmx_bean *vhost_info,
                                   server_rec *s)
{
    const char *server_name;
    const char *listen_addresses;
    const char *server_aliases;
    struct bmx_objectname *vhost_info_objn;

    /* create the ServerName */
    if (s->port && s->server_hostname)
        server_name = apr_psprintf(p, "%s:%d", s->server_hostname, s->port);
    else if (s->server_hostname)
        server_name = s->server_hostname;
    else
        server_name = "";

    listen_addresses = vhost_listen_addresses_str(p, s);
    if (!listen_addresses)
        listen_addresses = "";
    server_aliases = vhost_server_aliases_str(p, s);
    if (!server_aliases)
        server_aliases = "";

    /* create the objectname for this vhost's bean */
    bmx_objectname_create(&vhost_info_objn, BMX_VHOST_DOMAIN, p);
    apr_table_set(vhost_info_objn->props, "Type", BMX_VHOST_INFO_TYPE);
    apr_table_set(vhost_info_objn->props, "Host", s->server_hostname);
    apr_table_set(vhost_info_objn->props, "Port",
                  s->port ? apr_psprintf(p, "%d", s->port) : ANY_PORT);

    bmx_bean_init(vhost_info, vhost_info_objn);

    bmx_bean_prop_add(vhost_info,
        bmx_property_string_create("ServerName", server_name, p));
    bmx_bean_prop_add(vhost_info,
        bmx_property_string_create("ServerAliases", server_aliases, p));
    bmx_bean_prop_add(vhost_info,
        bmx_property_string_create("ListenAddresses", listen_addresses, p));
}

/**
 * Create the Server Config Bean for this VHost and associate the data
 * with the server data for this VHost (so we can retrieve it later when
 * responding to BMX Queries).
 * @param p The pool out of which to allocate needed data.
 * @param hostname The hostname to associate this VHost's data.
 * @param hostname The port to associate this VHost's data.
 */
struct bmx_vhost_scfg *bmx_vhost_create_scfg(apr_pool_t *p, 
                                             const char *hostname, int port)
{
    struct bmx_vhost_scfg *scfg = apr_pcalloc(p, sizeof(*scfg));

    create_scfg_objectname(p, &scfg->forever, FOREVER, hostname, port);
    create_scfg_objectname(p, &scfg->since_start, SINCE_START, hostname, port);
    create_scfg_objectname(p, &scfg->since_restart, SINCE_RESTART, hostname,
                           port);

    /* Create the DBM key to use to refer to the data for the given VHost. */
    scfg->key.dptr = apr_psprintf(p, "%s-%s:%d", KEY_PREFIX, hostname, port);
    scfg->key.dsize = strlen(scfg->key.dptr);

    return scfg;
}


/**
 * Fetch the vhost data for a given VHost and reset the 'since-restart" data
 * or allocate an entirely new record if the record did not already exist.
 */
static int vhost_data_reset(apr_dbm_t *dbm, server_rec *s, apr_pool_t *ptemp,
                            const struct bmx_vhost_scfg *scfg, int startup)
{
    int rv = APR_SUCCESS;
    apr_datum_t value;
    struct vhost_data vhost_data;
    apr_time_t now = apr_time_now();

    memset(&value, 0, sizeof(value));

    rv = apr_dbm_fetch(dbm, scfg->key, &value);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_CRIT, rv, s, "Failed to fetch "
                     "mod_bmx_vhost record for vhost '%s'", scfg->key.dptr);
        goto out;
    }

    /* create the record the first time */
    if (!value.dptr || value.dsize != sizeof(vhost_data)) {
        memset(&vhost_data, 0, sizeof(vhost_data));
        vhost_data.since_start.StartTime = now;
        vhost_data.since_restart.StartTime = now;
        vhost_data.forever.StartTime = now;
    }
    /* otherwise reuse the previous record */
    else {
        memcpy(&vhost_data, value.dptr, sizeof(vhost_data));
        if (startup) {
            /* clear since-start parts */
            memset(&vhost_data.since_start, 0, sizeof(vhost_data.since_start));
            vhost_data.since_start.StartTime = now;
        }
        /* clear since-restart parts */
        memset(&vhost_data.since_restart, 0, sizeof(vhost_data.since_restart));
        vhost_data.since_restart.StartTime = now;
    }

    value.dsize = sizeof(vhost_data);
    value.dptr = (void *)&vhost_data;

    rv = apr_dbm_store(dbm, scfg->key, value);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_CRIT, rv, s, "Failed to store "
                     "mod_bmx_vhost record for vhost '%s'", scfg->key.dptr);
        goto out;
    }
out:
    return rv;
}

/**
 * Update a timespan record in the DBM according to the data contained
 * in the given request_rec.
 */
static void vhost_timespan_update(struct vhost_timespan *ts,
                                  request_rec *r, request_rec *last)
{
    switch (r->method_number) {
    case M_GET:
        if (r->header_only) { /* HEAD */
            ts->InBytesHEAD += r->read_length;
            ts->InRequestsHEAD++;
        } else { /* GET */
            ts->InBytesGET += r->read_length;
            ts->InRequestsGET++;
        }
        break;
    case M_POST:
        ts->InBytesPOST += r->read_length;
        ts->InRequestsPOST++;
        break;
    case M_PUT:
        ts->InBytesPUT += r->read_length;
        ts->InRequestsPUT++;
        break;
    };

    switch (r->status) {
    case 200:
        ts->OutBytes200 += r->bytes_sent ? r->bytes_sent : last->bytes_sent;
        ts->OutResponses200++;
        break;
    case 301:
        ts->OutBytes301 += r->bytes_sent ? r->bytes_sent : last->bytes_sent;
        ts->OutResponses301++;
        break;
    case 302:
        ts->OutBytes302 += r->bytes_sent ? r->bytes_sent : last->bytes_sent;
        ts->OutResponses302++;
        break;
    case 401:
        ts->OutBytes401 += r->bytes_sent ? r->bytes_sent : last->bytes_sent;
        ts->OutResponses401++;
        break;
    case 403:
        ts->OutBytes403 += r->bytes_sent ? r->bytes_sent : last->bytes_sent;
        ts->OutResponses403++;
        break;
    case 404:
        ts->OutBytes404 += r->bytes_sent ? r->bytes_sent : last->bytes_sent;
        ts->OutResponses404++;
        break;
    case 500:
        ts->OutBytes500 += r->bytes_sent ? r->bytes_sent : last->bytes_sent;
        ts->OutResponses500++;
        break;
    };

    ts->InLowBytes += r->read_length;
    ts->OutLowBytes += r->bytes_sent ? r->bytes_sent : last->bytes_sent;

    ts->InRequests++;
    ts->OutResponses++;
}

/**
 * Update all timespan records within the given VHost Data record.
 */
static void vhost_data_update(struct vhost_data *vhost_data,
                              request_rec *r, request_rec *last)
{
    vhost_timespan_update(&(vhost_data->forever), r, last);
    vhost_timespan_update(&(vhost_data->since_start), r, last);
    vhost_timespan_update(&(vhost_data->since_restart), r, last);
}

/**
 * Print the given VHost bean to the response.
 */
static void print_vhost_bean(request_rec *r,
                             bmx_bean_print print_bean_fn,
                             struct bmx_objectname *objectname,
                             struct vhost_timespan *timespan)
{
    struct bmx_bean bean;
    apr_time_t now = apr_time_now();

    bmx_bean_init(&bean, objectname);

    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("InBytesGET",
                                   timespan->InBytesGET, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("InBytesHEAD",
                                   timespan->InBytesHEAD, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("InBytesPOST",
                                   timespan->InBytesPOST, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("InBytesPUT",
                                   timespan->InBytesPUT, r->pool));

    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("InRequestsGET",
                                   timespan->InRequestsGET, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("InRequestsHEAD",
                                   timespan->InRequestsHEAD, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("InRequestsPOST",
                                   timespan->InRequestsPOST, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("InRequestsPUT",
                                   timespan->InRequestsPUT, r->pool));

    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("OutBytes200",
                                   timespan->OutBytes200, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("OutBytes301",
                                   timespan->OutBytes301, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("OutBytes302",
                                   timespan->OutBytes302, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("OutBytes401",
                                   timespan->OutBytes401, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("OutBytes403",
                                   timespan->OutBytes403, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("OutBytes404",
                                   timespan->OutBytes404, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("OutBytes500",
                                   timespan->OutBytes500, r->pool));

    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("OutResponses200",
                                   timespan->OutResponses200, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("OutResponses301",
                                   timespan->OutResponses301, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("OutResponses302",
                                   timespan->OutResponses302, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("OutResponses401",
                                   timespan->OutResponses401, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("OutResponses403",
                                   timespan->OutResponses403, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("OutResponses404",
                                   timespan->OutResponses404, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("OutResponses500",
                                   timespan->OutResponses500, r->pool));

    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("InLowBytes",
                                   timespan->InLowBytes, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("OutLowBytes",
                                   timespan->OutLowBytes, r->pool));

    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("InRequests",
                                   timespan->InRequests, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("OutResponses",
                                   timespan->OutResponses, r->pool));

    bmx_bean_prop_add(&bean,
        bmx_property_string_create("StartDate",
                                   ap_ht_time(r->pool, timespan->StartTime,
                                              DEFAULT_TIME_FORMAT, 0),
                                   r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("StartTime",
                                   timespan->StartTime, r->pool));
    bmx_bean_prop_add(&bean,
        bmx_property_uint64_create("StartElapsed",
                                   now - timespan->StartTime, r->pool));
        
    print_bean_fn(r, &bean);
}

/* --------------------------------------------------------------------
 * Hook processing
 * -------------------------------------------------------------------- */

/**
 * Process an BMX Query by checking if the Query applies to our timespan
 * beans and then by fetching the appropriate data from the DBM and returning
 * it to the requesting client. 
 */
static int process_vhost_query(request_rec *r, 
                               const struct bmx_objectname *query,
                               bmx_bean_print print_bean_fn,
                               struct bmx_vhost_scfg *scfg)
{
    int rv = DECLINED;
    int forever = 0, since_start = 0, since_restart = 0;

    forever = bmx_check_constraints(query, scfg->forever);
    since_start = bmx_check_constraints(query, scfg->since_start);
    since_restart = bmx_check_constraints(query, scfg->since_restart);

    if (forever || since_start || since_restart) {
        apr_datum_t value;
        struct vhost_data vhost_data;

        memset(&value, 0, sizeof(value));

        /* lock the DBM */
        rv = apr_global_mutex_lock(dbmlock);
        if (rv != APR_SUCCESS) {
            ap_log_rerror(APLOG_MARK, APLOG_CRIT, rv, r, "Lock failure while "
                          "processing BMX query");
            goto error;
        }

        /* open the DBM */
        rv = apr_dbm_open(&dbm, dbm_fname, APR_DBM_READONLY,
                          APR_OS_DEFAULT, r->pool);
        if (rv != APR_SUCCESS) {
            ap_log_rerror(APLOG_MARK, APLOG_CRIT, rv, r, "DBM failure while "
                          "processing BMX query");
            goto unlock_error;
        }

        /* fetch the record for this vhost */
        rv = apr_dbm_fetch(dbm, scfg->key, &value);
        if (rv != APR_SUCCESS) {
            ap_log_rerror(APLOG_MARK, APLOG_CRIT, rv, r, "DBM fetch failure "
                          "while processing BMX query");
            goto close_error;
        }

        if (value.dptr) {
            memcpy(&vhost_data, value.dptr, value.dsize);
            value.dptr = (void *)&vhost_data;
        } else {
            ap_log_rerror(APLOG_MARK, APLOG_CRIT, 0, r, "No DBM record found "
                          "while processing BMX query");
            goto close_error;
        }

        /* close the DBM */
        apr_dbm_close(dbm);

        /* unlock the DBM */
        rv = apr_global_mutex_unlock(dbmlock);
        if (rv != APR_SUCCESS) {
            ap_log_rerror(APLOG_MARK, APLOG_CRIT, rv, r, "Unlock failure while "
                          "logging transaction in mod_bmx_vhost");
            goto error;
        }

        if (forever) {
            print_vhost_bean(r, print_bean_fn, scfg->forever,
                             &vhost_data.forever);
            rv = OK;
        }

        if (since_start) {
            print_vhost_bean(r, print_bean_fn, scfg->since_start,
                             &vhost_data.since_start);
            rv = OK;
        }

        if (since_restart) {
            print_vhost_bean(r, print_bean_fn, scfg->since_restart,
                             &vhost_data.since_restart);
            rv = OK;
        }
    }

    return rv;

close_error:
    apr_dbm_close(dbm);

unlock_error:
    (void)apr_global_mutex_unlock(dbmlock);

error:
    return HTTP_INTERNAL_SERVER_ERROR;
}

/**
 * Process an BMX Query by checking if the Query applies to our bean(s)
 * and then by calling the appropriate bean generation routines.
 */
static int bmx_vhost_query_hook(request_rec *r,
                                const struct bmx_objectname *query,
                                bmx_bean_print print_bean_fn)
{
    int rv, rv2;
    server_rec *s;

    /* check the global too */
    rv = process_vhost_query(r, query, print_bean_fn, global_scfg);
    if (rv != OK && rv != DECLINED) {
        /* we hit some error (reported already) */
        return rv;
    }

    for (s = r->connection->base_server; s; s = s->next) {
        struct bmx_vhost_scfg *scfg = ap_get_module_config(s->module_config,
                                                           &bmx_vhost_module);
        /* Print out the mod_bmx_vhost:Type=forever/since-start/since-restart
           beans for this vhost */
        rv2 = process_vhost_query(r, query, print_bean_fn, scfg);
        if (rv2 == OK) {
            rv = OK;
        } else if (rv2 != DECLINED) {
            /* we hit some error (reported already) */
            return rv2;
        }

        if (bmx_check_constraints(query,
                                  bmx_bean_get_objectname(&scfg->vhost_info))) {
            print_bean_fn(r, &scfg->vhost_info);
            rv = OK;
        }
    }

    return rv;
}

/*
 *  BMX Virtual Host Extension to mod_status
 */
static int bmx_vhost_status_hook(request_rec *r, int flags)
{
    if (flags & AP_STATUS_SHORT)
        return OK;

#ifdef TODO_STATUS_REPORT
    /* status output, will probably introduce a custom print_bean_fn e.g. */
        ap_rputs("\n\n<table border=\"0\"><tr>"
                 "<th>SSes</th><th>Timeout</th><th>Method</th>"
                 "</tr>\n", r);
        ap_rprintf(r, "<tr><td>%s</td><td>%s</td></tr>\n", name, value);
        ap_rputs("</table>\n", r);

    int rv, rv2;
    server_rec *s;

    ap_rputs("<hr />\n<h1>BMX Virtual Host Tracking Summary</h1>\n", r);

    /* check the global too */
    rv = process_vhost_query(r, query, print_bean_fn, global_scfg);
    if (rv != OK && rv != DECLINED) {
        /* we hit some error (reported already) */
        return rv;
    }

    for (s = r->connection->base_server; s; s = s->next) {
        struct bmx_vhost_scfg *scfg = ap_get_module_config(s->module_config,
                                                           &bmx_vhost_module);
        /* Print out the mod_bmx_vhost:Type=forever/since-start/since-restart
           beans for this vhost */
        rv2 = process_vhost_query(r, query, print_bean_fn, scfg);
        if (rv2 == OK) {
            rv = OK;
        } else if (rv2 != DECLINED) {
            /* we hit some error (reported already) */
            return rv2;
        }

        if (bmx_check_constraints(query,
                                  bmx_bean_get_objectname(&scfg->vhost_info))) {
            print_bean_fn(r, &scfg->vhost_info);
            rv = OK;
        }

    }
#endif

    return OK;
}

static int bmx_vhost_pre_config(apr_pool_t *pconf, apr_pool_t *plog,
                                apr_pool_t *ptemp)
{
    dbm_fname = ap_server_root_relative(pconf, DBM_FNAME);
    dbmlock_fname = ap_server_root_relative(pconf, DBMLOCK_FNAME);

    APR_OPTIONAL_HOOK(bmx, query_hook, bmx_vhost_query_hook, NULL, NULL,
                      APR_HOOK_MIDDLE);
    APR_OPTIONAL_HOOK(ap, status_hook, bmx_vhost_status_hook, NULL, NULL,
                      APR_HOOK_MIDDLE);
    return OK;
}

static int bmx_vhost_post_config(apr_pool_t *pconf, apr_pool_t *plog,
                                 apr_pool_t *ptemp, server_rec *s)
{
    const char *preflight_key = "bmx_vhost_preflight";
    void *preflight = NULL;
    server_rec *vhost;
    int startup;
    apr_status_t rv;

    /* open a DBM to check that it can be created, see WARN below */
    rv = apr_dbm_open(&dbm, dbm_fname, APR_DBM_RWCREATE,
                      APR_OS_DEFAULT, ptemp);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_CRIT, rv, s, "Failed to open "
                     "mod_bmx_vhost DBM file '%s'", dbm_fname);
        return HTTP_INTERNAL_SERVER_ERROR;
    }

#if !defined(OS2) && !defined(WIN32) && !defined(BEOS) && !defined(NETWARE)
    /*
     * We have to make sure the Apache child processes have access to
     * the DBM file.  WARN: With apr_dbm_open_ex, apr_dbm_get_usednames_ex
     * is required to determine the correct filenames.
     */
    if (geteuid() == 0 /* is superuser */)
    {
        const char *dbmfile1 = NULL, *dbmfile2 = NULL;
        apr_dbm_get_usednames(ptemp, dbm_fname, &dbmfile1, &dbmfile2);
	if (dbmfile1)
            chown(dbmfile1, unixd_config.user_id, -1 /* no gid change */);
	if (dbmfile2)
            chown(dbmfile2, unixd_config.user_id, -1 /* no gid change */);
    }
#endif

    /* create a mutex to protect the DBM */
    rv = apr_global_mutex_create(&dbmlock, dbmlock_fname, APR_LOCK_DEFAULT,
                                 s->process->pool);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_CRIT, rv, s, "Failed to create "
                     "mod_bmx_vhost global mutex for DBM in file '%s'",
                     dbmlock_fname);
        rv = HTTP_INTERNAL_SERVER_ERROR;
        goto out;
    }

#ifdef AP_NEED_SET_MUTEX_PERMS
    rv = unixd_set_global_mutex_perms(dbmlock);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_CRIT, rv, s,
                     "mod_bmx_vhost could not set permissions on global mutex"
                     " for DBM in file '%s'; check User and Group directives",
                     dbmlock_fname);
        rv = HTTP_INTERNAL_SERVER_ERROR;
        goto out;
    }
#endif

    /* Check if this is configtest or a preflight phase, clear nothing! */
    apr_pool_userdata_get(&preflight, preflight_key, s->process->pool);
    if (!preflight) {
        apr_pool_userdata_set(preflight_key, preflight_key,
                              apr_pool_cleanup_null, s->process->pool);
        rv = OK;
        goto out;
    }

#if MODULE_MAGIC_NUMBER_MAJOR >= 20090401
    /* Check if this is the first generation supported in 2.3.3+ */
    if (ap_mpm_query(AP_MPMQ_GENERATION, &startup) == APR_SUCCESS)
	startup = (startup == 0);
#else
    startup = (ap_my_generation == 0);
#endif

    /* Create the global server config */
    global_scfg = bmx_vhost_create_scfg(pconf, GLOBAL_SERVER_NAME, GLOBAL_PORT);

    rv = vhost_data_reset(dbm, s, ptemp, global_scfg, startup);
    if (rv != APR_SUCCESS) {
         rv = HTTP_INTERNAL_SERVER_ERROR;
         goto out;
    }

    /* create a server config for each vhost */
    for (vhost = s; vhost; vhost = vhost->next)
    {
        /* create our module config for this server */
        struct bmx_vhost_scfg *scfg;
        scfg = bmx_vhost_create_scfg(pconf, vhost->server_hostname, vhost->port);
        ap_set_module_config(vhost->module_config, &bmx_vhost_module, scfg);

        /* create our info bean for this server (none for global) */
        create_vhost_info_bean(pconf, &scfg->vhost_info, vhost);

        /* reset the DBM record - global server s is used for error logging */
        rv = vhost_data_reset(dbm, s, ptemp, scfg, startup);
        if (rv != APR_SUCCESS) {
             rv = HTTP_INTERNAL_SERVER_ERROR;
             goto out;
        }
    }

out:
    apr_dbm_close(dbm);
    return rv;
}

static int bmx_vhost_log_transaction(request_rec *r)
{
    struct bmx_vhost_scfg *scfg = ap_get_module_config(r->server->module_config,
                                                       &bmx_vhost_module);
    int rv = OK;
    apr_datum_t value, global_value;
    struct vhost_data vhost_data, global_data;
    request_rec *last = r;


    memset(&value, 0, sizeof(value));
    memset(&global_value, 0, sizeof(global_value));

    /* find the last response (in case of internal redirect?) */
    while (last->next) {
        last = last->next;
    }

    /* FIXME: make any pre-calculations so we reduce the time spent holding
       the lock. */

    /* lock the DBM */
    rv = apr_global_mutex_lock(dbmlock);
    if (rv != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_CRIT, rv, r, "Lock failure while "
                     "logging transaction in mod_bmx_vhost");
        goto error;
    }

    /* open the DBM */
    rv = apr_dbm_open(&dbm, dbm_fname, APR_DBM_READWRITE,
                      APR_OS_DEFAULT, r->pool);
    if (rv != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_CRIT, rv, r, "DBM failure while "
                      "logging transaction in mod_bmx_vhost");
        goto unlock_error;
    }

    /* fetch the record for this vhost */
    rv = apr_dbm_fetch(dbm, scfg->key, &value);
    if (rv != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_CRIT, rv, r, "DBM fetch failure while "
                      "logging transaction in mod_bmx_vhost (fetching vhost "
                      "data structures)");
        goto close_error;
    }

    if (value.dptr) {
        memcpy(&vhost_data, value.dptr, value.dsize);
        value.dptr = (void *)&vhost_data;
    } else {
        value.dsize = sizeof(vhost_data);
        value.dptr = (void *)&vhost_data;
        memset(&vhost_data, 0, value.dsize);
    }

    /* fetch the global record */
    rv = apr_dbm_fetch(dbm, global_scfg->key, &global_value);
    if (rv != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_CRIT, rv, r, "DBM fetch failure while "
                      "logging transaction in mod_bmx_vhost (fetching global "
                      "data structures)");
        goto close_error;
    }

    if (global_value.dptr) {
        memcpy(&global_data, global_value.dptr, global_value.dsize);
        global_value.dptr = (void *)&global_data;
    } else {
        ap_log_rerror(APLOG_MARK, APLOG_CRIT, 0, r, "DBM fetch failed while "
                      "retrieving global vhost error in mod_bmx_vhost");
        goto close_error;
    }

    /* update the records for each timespan */
    vhost_data_update(&vhost_data, r, last);
    vhost_data_update(&global_data, r, last);

    /* store the records */
    rv = apr_dbm_store(dbm, scfg->key, value);
    if (rv != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_CRIT, rv, r, "DBM store failure while "
                      "logging transaction in mod_bmx_vhost (storing vhost "
                      "data structures)");
        goto close_error;
    }

    rv = apr_dbm_store(dbm, global_scfg->key, global_value);
    if (rv != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_CRIT, rv, r, "DBM fetch failure while "
                      "logging transaction in mod_bmx_vhost (storing global "
                      "data structures)");
        goto close_error;
    }


    /* close the DBM */
    apr_dbm_close(dbm);

    /* unlock the DBM */
    rv = apr_global_mutex_unlock(dbmlock);
    if (rv != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_CRIT, rv, r, "Unlock failure while "
                      "logging transaction in mod_bmx_vhost");
        goto error;
    }

    return rv;

close_error:
    apr_dbm_close(dbm);

unlock_error:
    (void)apr_global_mutex_unlock(dbmlock);

error:
    return HTTP_INTERNAL_SERVER_ERROR;
}

static void bmx_vhost_child_init(apr_pool_t *pchild, server_rec *s)
{
    int rv = 0;
    rv = apr_global_mutex_child_init(&dbmlock, dbmlock_fname, pchild);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_CRIT, rv, s, "Failed to re-open "
                     "global mutex for DBM in mod_bmx_vhost during child_init");
        /* FIXME: prevent the server from starting */
    }
}

/* --------------------------------------------------------------------
 * Module internals
 * -------------------------------------------------------------------- */

static void bmx_vhost_register_hooks(apr_pool_t *p)
{
    ap_hook_pre_config(bmx_vhost_pre_config, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_post_config(bmx_vhost_post_config, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_child_init(bmx_vhost_child_init, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_log_transaction(bmx_vhost_log_transaction, NULL, NULL,
                            APR_HOOK_MIDDLE);
}

static const command_rec bmx_vhost_cmds[] =
{
    AP_INIT_TAKE1("BMXVHostDBMFilename", set_dbm_fname, NULL, RSRC_CONF,
                  "Name of the DBM file in which to store persistent data "
                  "for mod_bmx_vhost. Relative to the server root by "
                  "default [\"" DBM_FNAME "\"]"),
    AP_INIT_TAKE1("BMXVHostLockFilename", set_dbmlock_fname, NULL, RSRC_CONF,
                  "Name of the Lock file used to protect access to the DBM "
                  "used in mod_bmx_vhost. Relative to the server root by "
                  "default [\"" DBMLOCK_FNAME "\"]"),
    {NULL}
};

module AP_MODULE_DECLARE_DATA bmx_vhost_module =
{
    STANDARD20_MODULE_STUFF,
    NULL,                            /* per-directory config creator */
    NULL,                            /* dir config merger */
    NULL,                            /* server config creator */
    NULL,                            /* server config merger */
    bmx_vhost_cmds,                  /* command table */
    bmx_vhost_register_hooks,      /* set up other request processing hooks */
};

