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

#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "mount.h"
#include "common.h"

/** Mounts that will be ignored.
 *
 * These are standard mounts that will be created within the VM
 * automatically, hence do not need to be mounted before the VM is
 * started.
 */
static struct mntent clr_oci_mounts_to_ignore[] =
{
	{ NULL, (char *)"/proc"           , NULL, NULL, -1, -1 },
	{ NULL, (char *)"/dev"            , NULL, NULL, -1, -1 },
	{ NULL, (char *)"/dev/pts"        , NULL, NULL, -1, -1 },
	{ NULL, (char *)"/dev/shm"        , NULL, NULL, -1, -1 },
	{ NULL, (char *)"/dev/mqueue"     , NULL, NULL, -1, -1 },
	{ NULL, (char *)"/sys"            , NULL, NULL, -1, -1 },
	{ NULL, (char *)"/sys/fs/cgroup"  , NULL, NULL, -1, -1 }
};

/*!
 * Determine if the specified \ref clr_oci_mount represents a
 * mount that can be ignored.
 *
 * \param m \ref clr_oci_mount.
 *
 * \return \c true if mount can be ignored, else \c false.
 */
private gboolean
clr_oci_mount_ignore (struct clr_oci_mount *m)
{
	struct mntent  *me;
	size_t          i;
	size_t          max = CLR_OCI_ARRAY_SIZE (clr_oci_mounts_to_ignore);

	if (! m) {
		return false;
	}

	for (i = 0; i < max; i++) {
		me = &clr_oci_mounts_to_ignore[i];

		if (clr_oci_found_str_mntent_match (me, m, mnt_fsname)) {
			goto ignore;
		}

		if (clr_oci_found_str_mntent_match (me, m, mnt_dir)) {
			goto ignore;
		}

		if (clr_oci_found_str_mntent_match (me, m, mnt_type)) {
			goto ignore;
		}
	}

	return false;

ignore:
	m->ignore_mount = true;
	return true;
}

/*!
 * Free the specified \ref clr_oci_mount.
 *
 * \param m \ref clr_oci_mount.
 */
void
clr_oci_mount_free (struct clr_oci_mount *m)
{
	g_assert (m);

	g_free_if_set (m->mnt.mnt_fsname);
	g_free_if_set (m->mnt.mnt_dir);
	g_free_if_set (m->mnt.mnt_type);
	g_free_if_set (m->mnt.mnt_opts);

	g_free (m);
}

/*!
 * Free all mounts.
 *
 * \param mounts List of \ref clr_oci_mount.
 */
void
clr_oci_mounts_free_all (GSList *mounts)
{
	if (! mounts) {
		return;
	}

	g_slist_free_full (mounts, (GDestroyNotify)clr_oci_mount_free);
}

/*!
 * Mount the resource specified by \p m.
 *
 * \param m \ref clr_oci_mount.
 * \param dry_run If \c true, don't actually call \c mount(2),
 * just log what would be done.
 *
 * \return \c true on success, else \c false.
 */
private gboolean
clr_oci_perform_mount (const struct clr_oci_mount *m, gboolean dry_run)
{
	const char *fmt = "%smount%s %s of type %s "
		"onto %s with options '%s' "
		"and flags 0x%lx%s%s";

	int ret;

	if (! m) {
		return false;
	}

	g_debug (fmt, dry_run ? "Not " : "",
			"ing",
			m->mnt.mnt_fsname,
			m->mnt.mnt_type,
			m->dest,
			m->mnt.mnt_opts ? m->mnt.mnt_opts : "",
			m->flags,
			dry_run ? " (dry-run mode)" : "",
			"");

	if (dry_run) {
		return true;
	}

	ret = mount (m->mnt.mnt_fsname,
			m->dest,
			m->mnt.mnt_type,
			m->flags,
			m->mnt.mnt_opts);
	if (ret) {
		int saved = errno;
		gchar *msg;

		msg = g_strdup_printf (": %s", strerror (saved));

		g_critical (fmt,
				"Failed to ",
				"",
				m->mnt.mnt_fsname,
				m->mnt.mnt_type,
				m->dest,
                m->mnt.mnt_opts ? m->mnt.mnt_opts : "",
				m->flags,
				"",
				msg);
		g_free (msg);
	}

	return ret == 0;
}

/*!
 * Setup required mounts.
 *
 * \param config \ref clr_oci_config.
 *
 * \return \c true on success, else \c false.
 */
gboolean
clr_oci_handle_mounts (struct clr_oci_config *config)
{
	GSList    *l;
	gboolean   ret;
	struct stat st;
	gchar* dirname_dest = NULL;

	if (! config) {
		return false;
	}

	for (l = config->oci.mounts; l && l->data; l = g_slist_next (l)) {
		struct clr_oci_mount *m = (struct clr_oci_mount *)l->data;

		if (clr_oci_mount_ignore (m)) {
			g_debug ("ignoring mount %s", m->mnt.mnt_dir);
			continue;
		}

		g_snprintf (m->dest, sizeof (m->dest),
				"%s%s",
				config->oci.root.path, m->mnt.mnt_dir);

		if (m->mnt.mnt_fsname[0] == '/') {
			if (stat (m->mnt.mnt_fsname, &st)) {
				g_debug ("ignoring mount, %s does not exist", m->mnt.mnt_fsname);
				continue;
			}
			if (! S_ISDIR(st.st_mode)) {
				dirname_dest = g_path_get_dirname(m->dest);
			}
		}

		if (! dirname_dest) {
			dirname_dest = g_strdup(m->dest);
		}

		ret = g_mkdir_with_parents (dirname_dest, CLR_OCI_DIR_MODE);
		if (ret < 0) {
			g_critical ("failed to create mount directory: %s (%s)",
					m->dest, strerror (errno));
			g_free(dirname_dest);
			return false;
		}

		g_free(dirname_dest);
		dirname_dest = NULL;

		if (! clr_oci_perform_mount (m, config->dry_run_mode)) {
			return false;
		}
	}

	return true;
}

/*!
 * Unmount the mount specified by \p m.
 *
 * \param m \ref clr_oci_mount.
 *
 * \return \c true on success, else \c false.
 */
private gboolean
clr_oci_perform_unmount (const struct clr_oci_mount *m)
{
	if (! m) {
		return false;
	}

	g_debug ("unmounting %s", m->dest);

	return umount (m->dest) == 0;
}

/*!
 * Unmount all mounts.
 *
 * \param config \ref clr_oci_config.
 *
 * \return \c true on success, else \c false.
 */
gboolean
clr_oci_handle_unmounts (const struct clr_oci_config *config)
{
	GSList  *l;

	if (! config) {
		return false;
	}

	for (l = config->oci.mounts; l && l->data; l = g_slist_next (l)) {
		struct clr_oci_mount *m = (struct clr_oci_mount *)l->data;

		if (m->ignore_mount) {
			/* was never mounted */
			continue;
		}

		if (! clr_oci_perform_unmount (m)) {
			return false;
		}
	}

	return true;
}

/*!
 * Convert the list of mounts to a JSON array.
 *
 * Note that the returned array will be empty unless any of the list of
 * mounts provided in \ref CLR_OCI_CONFIG_FILE were actually mounted
 * (many are ignored as they are unecessary in the hypervisor case).
 *
 * \param config \ref clr_oci_config.
 *
 * \return \c JsonArray on success, else \c NULL.
 */
JsonArray *
clr_oci_mounts_to_json (const struct clr_oci_config *config)
{
	JsonArray *array = NULL;
	GSList *l;

	array  = json_array_new ();

	for (l = config->oci.mounts; l && l->data; l = g_slist_next (l)) {
		struct clr_oci_mount *m = (struct clr_oci_mount *)l->data;

		if (m->ignore_mount) {
			/* was never mounted */
			continue;
		}

		json_array_add_string_element (array, m->dest);
	}

	return array;
}
