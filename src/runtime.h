/*
 * This file is part of clr-oci-runtime.
 * 
 * Copyright (C) 2016 Intel Corporation
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _CLR_OCI_RUNTIME_H
#define _CLR_OCI_RUNTIME_H

#include <glib.h>

gboolean clr_oci_runtime_path_get (struct clr_oci_config *config);
gboolean clr_oci_runtime_dir_setup (struct clr_oci_config *config);
gboolean clr_oci_runtime_dir_delete (struct clr_oci_config *config);

#endif /* _CLR_OCI_RUNTIME_H */
