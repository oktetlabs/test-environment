/*	target/scsi_target_module.c

    	vi: set autoindent tabstop=4 shiftwidth=4 :

 	This file must be included by every front-end target driver that is
 	trying to register with the mid-level. This is the common portion
 	that gets included with all the low-level drivers.

 	To use this the low-level driver must define the variable my_template
 	of the type Scsi_Target_Template with all the needed functions and
 	values defined in scsi_target.h

*/
/*
	Copyright (C) 2001-2003 InterOperability Lab (IOL)
	University of New Hampshier (UNH)
	Durham, NH 03824

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
	USA.

	The name of IOL and/or UNH may not be used to endorse or promote products
	derived from this software without specific prior written permission.
*/

# include <linux/module.h>
# include <linux/init.h>
# include <linux/kernel.h>
# include "scsi_target.h"

int
scsi_target_module_init(void)
{

	debugInit("scsi_target_module_init: Initializing: %s\n", 
		my_template.name);
	if (register_target_template(&my_template) < 0)
		return (-1);

	return (0);
}

void
scsi_target_module_cleanup(void)
{

#ifdef CONFIG_PROC_FS
	/* Don't show the /proc/scsi_target files */
	remove_proc_entry("scsi_target/scsi_target", 0);
	remove_proc_entry("scsi_target", 0);
#endif

	if (deregister_target_template(&my_template) < 0)
		printk("Error in the deregistration process\n");

	debugInit("scsi_target_module_cleanup exit\n");
}

module_init(scsi_target_module_init);
module_exit(scsi_target_module_cleanup);
