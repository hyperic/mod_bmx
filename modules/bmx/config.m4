dnl See the NOTICE file distributed with this work for information
dnl regarding copyright ownership. This file is licensed to You under
dnl the Apache License, Version 2.0 (the "License"); you may not use
dnl this file except in compliance with the License.  You may obtain
dnl a copy of the License at
dnl
dnl       http://www.apache.org/licenses/LICENSE-2.0
dnl
dnl Unless required by applicable law or agreed to in writing, software
dnl distributed under the License is distributed on an "AS IS" BASIS,
dnl WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
dnl See the License for the specific language governing permissions and
dnl limitations under the License.

dnl #  start of module specific part
APACHE_MODPATH_INIT(fcgid)

case $host in
    *mingw*)
        fcgid_platform_objs="fcgid_pm_win.lo fcgid_proc_win.lo fcgid_proctbl_win.lo"
        ;;
    *)
        fcgid_platform_objs="fcgid_pm_unix.lo fcgid_proc_unix.lo fcgid_proctbl_unix.lo fcgid_mutex_unix.lo"
        ;;
esac

dnl #  list of module object files
fcigd_objs="dnl
mod_fcgid.lo dnl
fcgid_bridge.lo dnl
fcgid_conf.lo dnl
fcgid_pm_main.lo dnl
fcgid_protocol.lo dnl
fcgid_spawn_ctl.lo dnl
fcgid_bucket.lo dnl
fcgid_filter.lo dnl
$fcgid_platform_objs dnl
"

APACHE_MODULE(fcgid, [FastCGI support (mod_fcgid)], $fcigd_objs, , no, [
    AC_CHECK_HEADERS(sys/file.h)
    AC_CHECK_HEADERS(sys/mman.h)
    AC_CHECK_HEADERS(sys/mutex.h)
    AC_CHECK_HEADERS(sys/shm.h)
])

dnl #  end of module specific part
APACHE_MODPATH_FINISH
