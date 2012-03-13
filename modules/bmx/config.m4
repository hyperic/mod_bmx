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
APACHE_MODPATH_INIT(bmx)

# TODO: toggle/force mod_bmx for any bmx_extention 

APACHE_MODULE(bmx, [BMX Monitoring Core (mod_bmx)], , , most)
APACHE_MODULE(bmx_example, [BMX Example Plugin (mod_bmx_example)], , , no)
APACHE_MODULE(bmx_status, [BMX Status Plugin (mod_bmx_status)], , , most)
APACHE_MODULE(bmx_vhost, [BMX VHost Plugin (mod_bmx_vhost)], , , most)

dnl #  end of module specific part
APACHE_MODPATH_FINISH
