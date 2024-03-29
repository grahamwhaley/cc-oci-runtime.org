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

#include <stdbool.h>
#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <check.h>

#include "test_common.h"
#include "../src/util.h"
#include "json.h"

#include "../src/command.h"

struct start_data start_data;

/**
 * Determine if the string vector contains any element which
 * matches the specified regex.
 *
 * \param strv String vector.
 * \param regex Regular expression.
 *
 * \return \c true on success, else \c false.
 */
gboolean
strv_contains_regex (gchar **strv, const char *regex)
{
	gchar **s;
	gboolean ret;

	g_assert (strv);
	g_assert (regex);

	for (s = strv; s && *s; s++) {
		/* Handle lines that contain an allocated string
		 * terminator only.
		 */
		if (! g_strcmp0 (*s, "")) {
			continue;
		}

		ret = g_regex_match_simple (regex, *s, 0, 0);
		if (ret) {
			return true;
		}
	}

	return false;
}

/**
 * Ensure the specified timestamp is in the expected ISO-8601 format.
 *
 * An example timestamp is:
 *
 *     2016-05-12T16:45:05.567822Z
 *
 * \param timestamp String to check.
 *
 * \return \c true on success, else \c false.
 */
gboolean
check_timestamp_format (const gchar *timestamp)
{
	return g_regex_match_simple (
			"\\b"                  /* start of string */
			"\\d{4}-\\d{2}-\\d{2}" /* YYYY-MM-DD */
			"T"                    /* time separator */
			"\\d{2}:\\d{2}:\\d{2}" /* HH:MM:SS */
			"\\.\\d{6}"            /* ".XXXXXX" (microseconds) */
			"\\S*"                 /* timezone (or "Z" for localtime) */
			"\\b",                 /* end of string */
			timestamp, 0, 0);
}


GNode *node_find_child(GNode* node, const gchar* data) {
	GNode* child;
	if (! node) {
		return NULL;
	}

	child = g_node_first_child(node);
	while (child) {
		if (g_strcmp0(child->data, data) == 0) {
			return child;
		}
		child = g_node_next_sibling(child);
	}

	return NULL;
}

void test_spec_handler(struct spec_handler* handler, struct spec_handler_test* tests) {
	GNode* node;
	GNode* handler_node;
	GNode test_node;
	struct clr_oci_config config;
	struct spec_handler_test* test;
	int fd;

	ck_assert(! handler->handle_section(NULL, NULL));
	ck_assert(! handler->handle_section(&test_node, NULL));
	ck_assert(! handler->handle_section(NULL, &config));

	/**
	 * Create fake files for Kernel and image so
	 * path validation won't fail
	 */

	fd = g_creat("CONTAINER-KERNEL",0755);
	if (fd < 0) {
		g_critical ("failed to create file CONTAINER-KERNEL");
	} else {
		close(fd);
	}

	fd = g_creat("CLEAR-CONTAINERS.img",0755);
	if (fd < 0) {
		g_critical ("failed to create file CLEAR-CONTAINERS.img");
	} else {
		close(fd);
	}
	fd = g_creat("QEMU-LITE",0755);
	if (fd < 0) {
		g_critical ("failed to create file QEMU-LITE");
	} else {
		close(fd);
	}

	for (test=tests; test->file; test++) {
		memset(&config, 0, sizeof(struct clr_oci_config));
		clr_oci_json_parse(&node, test->file);
		handler_node = node_find_child(node, handler->name);
		ck_assert_msg(handler->handle_section(
		    handler_node, &config) == test->test_result,
		    test->file);
		clr_oci_config_free(&config);
		g_free_node(node);
	}

	if (g_remove("CONTAINER-KERNEL") < 0) {
		g_critical ("failed to remove file CONTAINER-KERNEL");
	}

	if (g_remove ("CLEAR-CONTAINERS.img") < 0) {
		g_critical ("failed to remove file CLEAR-CONTAINERS.img");
	}

	if (g_remove ("QEMU-LITE") < 0) {
		g_critical ("failed to remove file QEMU-LITE");
	}
}

/**
 * Create a fake state file for the specified VM.
 *
 * \param name Name to use for VM.
 * \param root_dir Root directory to use.
 * \param config Already allocated \ref clr_oci_config.
 *
 * \return \c true on success, else \c false.
 */
gboolean
test_helper_create_state_file (const char *name,
		const char *root_dir,
		struct clr_oci_config *config)
{
	g_autofree gchar *timestamp = NULL;

	assert (name);
	assert (root_dir);
	assert (config);

	timestamp = g_strdup_printf ("timestamp for %s", name);
	assert (timestamp);

	config->optarg_container_id = name;

	config->root_dir = g_strdup (root_dir);
	assert (config->root_dir);

	config->console = g_strdup_printf ("console device for %s", name);
	assert (config->console);

	assert (config->console);

	config->bundle_path = g_strdup_printf ("/tmp/bundle-for-%s",
			name);
	assert (config->bundle_path);

	/* set pid to ourselves so we know it's running */
	config->state.workload_pid = getpid ();

	if (! clr_oci_runtime_dir_setup (config)) {
		fprintf (stderr, "ERROR: failed to setup runtime dir "
				"for vm %s", name);
		return false;
	}

	/* config->vm not set */
	if (clr_oci_state_file_create (config, timestamp)) {
		fprintf (stderr, "ERROR: clr_oci_state_file_create "
				"worked unexpectedly for vm %s", name);
		return false;
	}

	config->vm = g_malloc0 (sizeof(struct clr_oci_vm_cfg));
	assert (config->vm);

	g_strlcpy (config->vm->hypervisor_path, "hypervisor-path",
			sizeof (config->vm->hypervisor_path));

	g_strlcpy (config->vm->image_path, "image-path",
			sizeof (config->vm->image_path));

	g_strlcpy (config->vm->kernel_path, "kernel-path",
			sizeof (config->vm->kernel_path));

	g_strlcpy (config->vm->workload_path, "workload-path",
			sizeof (config->vm->workload_path));

	config->vm->kernel_params = g_strdup_printf ("kernel params for %s", name);

	/* config->vm now set */
	if (! clr_oci_state_file_create (config, timestamp)) {
		fprintf (stderr, "ERROR: clr_oci_state_file_create "
				"failed unexpectedly");
		return false;

	}

	return true;
}

