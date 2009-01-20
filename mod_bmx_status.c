/*
 * mod_bmx_status.c: Apache Internal Status Monitoring Module
 *
 * Copyright 2007 Codemass, Inc.
 * Copyright 2007 Hyperic, Inc.
 * All rights reserved. Use is subject to license terms.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
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
 * This module was derived from Apache's mod_status.c module.
 * FIXME: adhere to any ASF licensing requirements here
 */

/**
 * $Id: mod_bmx_status.c,v 1.1 2007/11/05 22:15:44 aaron Exp $
 *
 * TBD: description
 */

#define CORE_PRIVATE
#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_protocol.h"
#include "http_main.h"
#include "ap_mpm.h"
#include "util_script.h"
#include <time.h>
#include "scoreboard.h"
#include "http_log.h"
#include "mod_status.h"

#include "apr_strings.h"
#include "mod_bmx.h"

#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#define APR_WANT_STRFUNC
#include "apr_want.h"

#ifdef NEXT
#if (NX_CURRENT_COMPILER_RELEASE == 410)
#ifdef m68k
#define HZ 64
#else
#define HZ 100
#endif
#else
#include <machine/param.h>
#endif
#endif /* NEXT */

#define STATUS_MAXLINE 64

#define KBYTE 1024
#define MBYTE 1048576L
#define GBYTE 1073741824L

#ifndef DEFAULT_TIME_FORMAT
#define DEFAULT_TIME_FORMAT "%A, %d-%b-%Y %H:%M:%S %Z"
#endif

module AP_MODULE_DECLARE_DATA bmx_status_module;

#define BMX_STATUS_DOMAIN "mod_bmx_status"
static struct bmx_objectname *bmx_status_objectname;

static int server_limit, thread_limit;

#ifdef HAVE_TIMES
/* ugh... need to know if we're running with a pthread implementation
 * such as linuxthreads that treats individual threads as distinct
 * processes; that affects how we add up CPU time in a process
 */
static pid_t child_pid;
#endif

static char status_flags[SERVER_NUM_STATUS];

static int bmx_status_query_hook(request_rec *r,
                                 const struct bmx_objectname *query,
                                 bmx_bean_print print_bean_fn)
{
    struct bmx_bean *bmx_status_bean;
    apr_time_t nowtime;
    apr_interval_time_t up_time;
    int j, i, res;
    apr_uint32_t ready;
    apr_uint32_t busy;
    apr_uint64_t count;
    apr_uint64_t lres;
    apr_off_t bytes;
    apr_off_t bcount, kbcount;
#ifdef HAVE_TIMES
    float tick;
    int times_per_thread = getpid() != child_pid;
#endif
    int no_table_report;
    worker_score *ws_record;
    process_score *ps_record;
    char *stat_buffer;
    pid_t *pid_buffer;
    clock_t tu, ts, tcu, tcs;

#ifdef HAVE_TIMES
#ifdef _SC_CLK_TCK
    tick = sysconf(_SC_CLK_TCK);
#else
    tick = HZ;
#endif
#endif

    ready = 0;
    busy = 0;
    count = 0;
    bcount = 0;
    kbcount = 0;
    no_table_report = 0;

    /* FIXME: run this earlier than the above initializations to shortcut
     * bmx queries that don't match this module, for a small performance
     * improvement. */

    if (!bmx_check_constraints(query, bmx_status_objectname))
        return DECLINED;

    pid_buffer = apr_palloc(r->pool, server_limit * sizeof(pid_t));
    stat_buffer = apr_palloc(r->pool, server_limit * thread_limit * sizeof(char));

    nowtime = apr_time_now();
    tu = ts = tcu = tcs = 0;

    if (!ap_exists_scoreboard_image()) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Server status unavailable in inetd mode");
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    /* create the bean */
    bmx_bean_create(&bmx_status_bean, bmx_status_objectname, r->pool);

    for (i = 0; i < server_limit; ++i) {
#ifdef HAVE_TIMES
        clock_t proc_tu = 0, proc_ts = 0, proc_tcu = 0, proc_tcs = 0;
        clock_t tmp_tu, tmp_ts, tmp_tcu, tmp_tcs;
#endif

        ps_record = ap_get_scoreboard_process(i);
        for (j = 0; j < thread_limit; ++j) {
            int indx = (i * thread_limit) + j;

            ws_record = ap_get_scoreboard_worker(i, j);
            res = ws_record->status;
            stat_buffer[indx] = status_flags[res];

            if (!ps_record->quiescing
                && ps_record->pid) {
                if (res == SERVER_READY
                    && ps_record->generation == ap_my_generation)
                    ready++;
                else if (res != SERVER_DEAD &&
                         res != SERVER_STARTING &&
                         res != SERVER_IDLE_KILL)
                    busy++;
            }

            /* XXX what about the counters for quiescing/seg faulted
             * processes?  should they be counted or not?  GLA
             */
            if (ap_extended_status) {
                lres = ws_record->access_count;
                bytes = ws_record->bytes_served;

                if (lres != 0 || (res != SERVER_READY && res != SERVER_DEAD)) {
#ifdef HAVE_TIMES
                    tmp_tu = ws_record->times.tms_utime;
                    tmp_ts = ws_record->times.tms_stime;
                    tmp_tcu = ws_record->times.tms_cutime;
                    tmp_tcs = ws_record->times.tms_cstime;

                    if (times_per_thread) {
                        proc_tu += tmp_tu;
                        proc_ts += tmp_ts;
                        proc_tcu += tmp_tcu;
                        proc_tcs += proc_tcs;
                    }
                    else {
                        if (tmp_tu > proc_tu ||
                            tmp_ts > proc_ts ||
                            tmp_tcu > proc_tcu ||
                            tmp_tcs > proc_tcs) {
                            proc_tu = tmp_tu;
                            proc_ts = tmp_ts;
                            proc_tcu = tmp_tcu;
                            proc_tcs = proc_tcs;
                        }
                    }
#endif /* HAVE_TIMES */

                    count += lres;
                    bcount += bytes;

                    if (bcount >= KBYTE) {
                        kbcount += (bcount >> 10);
                        bcount = bcount & 0x3ff;
                    }
                }
            }
        }
#ifdef HAVE_TIMES
        tu += proc_tu;
        ts += proc_ts;
        tcu += proc_tcu;
        tcs += proc_tcs;
#endif
        pid_buffer[i] = ps_record->pid;
    }

    /* up_time in seconds */
    up_time = apr_time_sec(nowtime -
                           ap_scoreboard_image->global->restart_time);

    bmx_bean_prop_add(bmx_status_bean,
        bmx_property_string_create("ServerName",
                                   ap_get_server_name(r),
                                   r->pool));
    bmx_bean_prop_add(bmx_status_bean,
        bmx_property_string_create("ServerVersion",
                                   AP_SERVER_BASEVERSION,
                                   r->pool));
    bmx_bean_prop_add(bmx_status_bean,
        bmx_property_string_create("ServerBuilt",
                                   ap_get_server_built(),
                                   r->pool));
    bmx_bean_prop_add(bmx_status_bean,
        bmx_property_string_create("CurrentTime",
                                   ap_ht_time(r->pool, nowtime,
                                              DEFAULT_TIME_FORMAT, 0),
                                   r->pool));
    bmx_bean_prop_add(bmx_status_bean,
        bmx_property_string_create("RestartTime",
                                   ap_ht_time(r->pool,
                                      ap_scoreboard_image->global->restart_time,
                                      DEFAULT_TIME_FORMAT, 0),
                                   r->pool));
    bmx_bean_prop_add(bmx_status_bean,
        bmx_property_int32_create("ParentServerGeneration",
                                  ap_my_generation,
                                  r->pool));
    bmx_bean_prop_add(bmx_status_bean,
        bmx_property_uint64_create("ServerUptimeSeconds", up_time, r->pool));

    if (ap_extended_status) {
        bmx_bean_prop_add(bmx_status_bean,
            bmx_property_uint64_create("TotalAccesses", count, r->pool));
        bmx_bean_prop_add(bmx_status_bean,
            bmx_property_uint64_create("TotalTrafficKilobytes", kbcount, r->pool));

#ifdef HAVE_TIMES
        bmx_bean_prop_add(bmx_status_bean,
            bmx_property_string_create("CPUUsage",
                apr_psprintf(r->pool, "u%g s%g cu%g cs%g",
                            tu / tick, ts / tick, tcu / tick, tcs / tick),
                r->pool));
        if (ts || tu || tcu || tcs)
            bmx_bean_prop_add(bmx_status_bean,
                bmx_property_float_create("CPULoadPercent",
                    ((tu + ts + tcu + tcs) / tick / up_time * 100.), r->pool));
#endif
        if (up_time > 0) {
            bmx_bean_prop_add(bmx_status_bean,
                bmx_property_float_create("ReqPerSec",
                    ((float) count / (float) up_time), r->pool));
            bmx_bean_prop_add(bmx_status_bean,
                bmx_property_float_create("KilobytesPerSec",
                    (unsigned long)(KBYTE * (float) kbcount
                                    / (float) up_time), r->pool));
        }
        if (count > 0)
            bmx_bean_prop_add(bmx_status_bean,
                bmx_property_uint64_create("KilobytesPerReq",
                (apr_uint64_t)(KBYTE * (float) kbcount
                               / (float) count), r->pool));
    } /* ap_extended_status */

    bmx_bean_prop_add(bmx_status_bean,
        bmx_property_uint32_create("BusyWorkers", busy, r->pool));
    bmx_bean_prop_add(bmx_status_bean,
        bmx_property_uint32_create("IdleWorkers", ready, r->pool));

    print_bean_fn(r, bmx_status_bean);

    return OK;
}

static int bmx_status_init(apr_pool_t *p, apr_pool_t *plog, apr_pool_t *ptemp,
                       server_rec *s)
{
    status_flags[SERVER_DEAD] = '.';  /* We don't want to assume these are in */
    status_flags[SERVER_READY] = '_'; /* any particular order in scoreboard.h */
    status_flags[SERVER_STARTING] = 'S';
    status_flags[SERVER_BUSY_READ] = 'R';
    status_flags[SERVER_BUSY_WRITE] = 'W';
    status_flags[SERVER_BUSY_KEEPALIVE] = 'K';
    status_flags[SERVER_BUSY_LOG] = 'L';
    status_flags[SERVER_BUSY_DNS] = 'D';
    status_flags[SERVER_CLOSING] = 'C';
    status_flags[SERVER_GRACEFUL] = 'G';
    status_flags[SERVER_IDLE_KILL] = 'I';
    ap_mpm_query(AP_MPMQ_HARD_LIMIT_THREADS, &thread_limit);
    ap_mpm_query(AP_MPMQ_HARD_LIMIT_DAEMONS, &server_limit);
    return OK;
}

#ifdef HAVE_TIMES
static void bmx_status_child_init(apr_pool_t *p, server_rec *s)
{
    child_pid = getpid();
}
#endif

static int bmx_status_pre_config(apr_pool_t *pconf, apr_pool_t *plog,
                                  apr_pool_t *ptemp)
{
    /* create the objectname: "mod_bmx_status:Type=BMXExampleModule" */
    bmx_objectname_create(&bmx_status_objectname, BMX_STATUS_DOMAIN, pconf);
    apr_table_setn(bmx_status_objectname->props, "Name", "ServerStatus");
    if (ap_extended_status) {
        apr_table_setn(bmx_status_objectname->props, "Type", "Extended");
    } else {
        apr_table_setn(bmx_status_objectname->props, "Type", "Normal");
    }

    APR_OPTIONAL_HOOK(bmx, query_hook, bmx_status_query_hook, NULL, NULL,
                      APR_HOOK_MIDDLE);
    return OK;
}

static void register_hooks(apr_pool_t *p)
{
    ap_hook_pre_config(bmx_status_pre_config, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_post_config(bmx_status_init, NULL, NULL, APR_HOOK_MIDDLE);
#ifdef HAVE_TIMES
    ap_hook_child_init(bmx_status_child_init, NULL, NULL, APR_HOOK_MIDDLE);
#endif
}

module AP_MODULE_DECLARE_DATA bmx_status_module =
{
    STANDARD20_MODULE_STUFF,
    NULL,                       /* dir config creater */
    NULL,                       /* dir merger --- default is to override */
    NULL,                       /* server config */
    NULL,                       /* merge server config */
    NULL,                       /* command table */
    register_hooks              /* register_hooks */
};

