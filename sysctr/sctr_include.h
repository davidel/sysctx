/*    Copyright 2023 Davide Libenzi
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 * 
 */


#if !defined(SCTR_INCLUDE_H)
#define SCTR_INCLUDE_H

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include "sctr_macros.h"
#include "sctr_llist.h"

#if defined(HAVE_LINUX_UNISTD_H)
#include "sctr_linux.h"
#else
#error Your system is not supported by the sysctr library
#error Please mail to Davide Libenzi <davidel@xmailserver.org>
#endif

#include "sysctr.h"
#include "sctr_types.h"
#include "sctr_lib.h"
#include "sctr_util.h"
#include "sctr_cmn_calls.h"


#endif

