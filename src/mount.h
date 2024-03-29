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

#ifndef _CLR_OCI_MOUNT_H
#define _CLR_OCI_MOUNT_H

#include <stdbool.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

#include "oci.h"
#include "util.h"

/** Compare the specified string element from a mounts_to_ignore mntent
 * with an mntent from an oci_mount.
 *
 * \param mntent Mount entry.
 * \param clr_oci_mount \ref clr_oci_mount.
 * \param element \c mntent mount source path.
 *
 * \return \c true if a match is found, else \c false.
 */
#define clr_oci_found_str_mntent_match(mntent, clr_oci_mount, element) \
	((mntent)->element && \
	 (! g_strcmp0 ((mntent)->element, (clr_oci_mount)->mnt.element)))

gboolean clr_oci_handle_mounts (struct clr_oci_config *config);
gboolean clr_oci_handle_unmounts (const struct clr_oci_config *config);

void clr_oci_mounts_free_all (GSList *mounts);
void clr_oci_mount_free (struct clr_oci_mount *m);

JsonArray *clr_oci_mounts_to_json (const struct clr_oci_config *config);

#endif /* _CLR_OCI_MOUNT_H */
