# Test Environment
#
# Builder configuration macros definitions
#
# Copyright (C) 2003 Test Environment authors (see file AUTHORS in the 
# root directory of the distribution).
#
# Test Environment is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License as 
# published by the Free Software Foundation; either version 2 of 
# the License, or (at your option) any later version.
# 
# Test Environment is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
# MA  02111-1307  USA
#
# Author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
#
# $Id: OKT-DD-0000039-TE_BS_CONF.m4,v 1.1 2003/10/28 09:35:41 arybchik Exp $
 
# Declares the list of tools to be built by "make all" command.
# May be called several times.
#
# Parameters:
#       list of names of directories in ${TE_BASE}/tools 
#               separated by spaces (may be empty)
#
TE_TOOLS([tsc rgt])

# Specifies additional parameters to be passed to configure script of the
# tool. May be called once for each tool.
#
# Parameters:
#       name of the directory in ${TE_BASE}/tools
#       additional parameters to configure script (may be empty)
#       additional compiler flags
#       additional linker flags
#
TE_TOOL_PARMS([rgt], [], [-I/home/oleg/include], [-L/home/oleg/lib])

# Specifies parameters for NUT bootable image building.
# Moreover the NUT image name is included to the list of NUT images
# to be built by "make all" command.
# May be called once for each NUT image name. Source location may
# be the same for several NUT image names.
#
# Parameters:
#       NUT image name
#       source and building script location directory (absolute or 
#               relative from ${TE_BASE})
#       additional parameters to the building script (may be empty)
#
# Optional parameters:
#       full filename of the image in the storage 
#       storage configuration string
#
TE_NUT([vxworks], [${WIND_BASE}/target/src/bsp], [m68k --enable-shell])

# Specifies common TCE parameters for the NUT. May be called once for 
# each NUT image name.
#
# Parameters:
#       NUT image name
#       Test Agent name
#       Test Agent type
#       TCE type: branch, loop, multi, relational, routine, call or all
#               (see OKT-HLD-0000037-TE_TCE for details).
#       TCE data format (FILE or LOGGER)
#
TE_NUT_TCE([vxworks],[primo1],[vxworks-m68k],[multi],[LOGGER])

# Specifies NUT sources to be instrumented. May be called several times.
# 
# Parameters:
#       NUT image name
#       list of files/directories to be instrumented separated by spaces; 
#               if a directory is specified, all .c and .h files in it
#               and in all its subdirectories are instrumented; 
#               TCE data array and auxiliary routine definitions are 
#               appended to the first .c file met during macros processing
#       additional compiler flags to be used for sources compilation
# 
TE_NUT_TCE_SOURCES([vxworks], [main.c foo/bar.c], [-DI${WIND_BASE}/target/h])

# Requests for checking of program presence. May be called several times.
#
# Parameters:
#       program name
#
TE_HOST_EXEC([snmpbulkget])

# Declares a platform for and specifies platform-specific 
# parameters for configure script as well platform-specific 
# CFLAGS and LDFLAGS. 
# May be called once for each platform (including host platform).
#
# Parameters:
#       canonical host name
#       configure parameters (may be empty)
#       additional compiler flags
#       additional linker flags
#
TE_PLATFORM([powerpc-wrs-vxworks],[--enable-shell],
            [-I${WIND_BASE}/target/h], [-L${WIND_BASE}/target/lib])

# Specifies list of mandatory libraries to be built (may be empty). 
# May be called only once. If the macro is not called, all mandatory
# libraries are built.
# Libraries should be listed in order "independent first".
#
# Parameters:
#       list of directory names in ${TE_BASE}/lib separated by spaces
#
TE_LIB([ ipc rcfapi ])

# Specifies additional parameters to be passed to configure script of the
# mandatory or external library (common for all platforms). May be called 
# once for each library.
#
# Parameters:
#       name of the directory in ${TE_BASE}/lib
#       additional parameters to configure script (may be empty)
#       additional compiler flags
#       additional linker flags
#
TE_LIB_PARAM([rcfapi], [], [-DDEBUG], [--rdynamic])

# Specifies external libraries necessary for TEN applications, Test Agents
# and Test Suites. May be called once.
# Libraries should be listed in order "independent first".
#
# Parameters:
#       list of directory names in ${TE_BASE}/lib separated by spaces
#
TE_EL([comm_net_agent comm_net_engine])

# Specifies list of TEN applications to be built (may be empty). 
# May be called only once. If the macro is not called, all TEN applications
# are built.
#
# Parameters:
#       list of directory names in ${TE_BASE}/engine separated by spaces
#
TE_APP([logger rcf])

# Specifies additional parameters to be passed to configure script 
# of the application and list of external libraries to be linked with
# the application. May be called once for each TEN application.
#
# Parameters:
#       name of the directory in ${TE_BASE}/engine
#       additional parameters to configure script (may be empty)
#       additional compiler flags
#       additional linker flags
#       list of external libraries names (names of directories 
#              in ${TE_BASE}/lib in the order "independent last")
#
TE_APP_PARAM([rcf], [], [-DNDN_SUPPORT], [], [asn1])

# Specifies parameters for the Test Agent. Can be called once for each TA.
#
# Parameters:
#       type of the Test Agent 
#       sources location (name of the directory in ${TE_BASE}/agents or
#       full absolute path to sources)
#       platform (may be empty)
#       additional parameters to configure script (may be empty)
#       additional compiler flags
#       additional linker flags
#       list of external libraries names 
#               (names of directories in ${TE_BASE}/lib)
#       
TE_TA_TYPE([vxworks-ppc],[vxworks],[powerpc-wrs-vxworks], [--enable-logging],
           [-g -I/home/helen/include], [-u rcf_pch_start], 
           [rcfpch comm_net_agent])
