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

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <check.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <json-glib/json-glib.h>

#include "test_common.h"
#include "../src/logging.h"
#include "../src/runtime.h"
#include "../src/state.h"
#include "../src/oci.h"

gboolean clr_oci_vm_running (const struct oci_state *state);

// TODO: add a 2nd VM state file
START_TEST(test_clr_oci_list) {
	struct clr_oci_config config = { { 0 } };
	gboolean ret;
	gchar *tmpdir;
	gchar *vm1_dir;
	struct clr_oci_config vm1_config = { { 0 } };
	gchar *outfile = NULL;
	gchar *contents;
	gchar **lines;
	gchar *expected = NULL;
	JsonParser *parser = NULL;
	JsonReader *reader = NULL;
	JsonNode *node = NULL;
	const gchar *value;

	ck_assert (! clr_oci_list (NULL, NULL, true));
	ck_assert (! clr_oci_list (NULL, NULL, false));

	ck_assert (! clr_oci_list (NULL, "", true));
	ck_assert (! clr_oci_list (NULL, "", false));

	ck_assert (! clr_oci_list (&config, NULL, true));
	ck_assert (! clr_oci_list (&config, NULL, false));

	ck_assert (! clr_oci_list (&config, "", true));
	ck_assert (! clr_oci_list (&config, "", false));

	ck_assert (! clr_oci_list (&config, "", true));
	ck_assert (! clr_oci_list (&config, "", false));

	ck_assert (! clr_oci_list (&config, "invalid format", true));
	ck_assert (! clr_oci_list (&config, "invalid format", false));

	tmpdir = g_dir_make_tmp (NULL, NULL);
	ck_assert (tmpdir);

	vm1_dir = g_build_path ("/", tmpdir, "vm1", NULL);
	ck_assert (vm1_dir);

	/* specify an ENOENT path */
	config.root_dir = g_build_path ("/", tmpdir, "does-not-exist", NULL);
	ck_assert (config.root_dir);

	/**************************************/
	/* test default ASCII output - no VMs */

	SAVE_OUTPUT (outfile) {
		ret = clr_oci_list (&config, "table", false);
	}
	ck_assert (ret);

	/* check contents */
	ret = g_file_get_contents (outfile, &contents, NULL, NULL);
	ck_assert (ret);

	lines = g_strsplit (contents, "\n", -1);
	ck_assert (lines);

	ck_assert (lines[0]);
	ck_assert (! *lines[1]);

	/* expect only the header line with fields separated by
	 * whitespace.
	 */
	ret = g_regex_match_simple ("^ID\\s*"
			"PID\\s*"
			"STATUS\\s*"
			"BUNDLE\\s*"
			"CREATED\\s*$",
			contents, 0, 0);
	ck_assert (ret);

	g_free (contents);
	g_strfreev (lines);
	ck_assert (! g_remove (outfile));
	g_free (outfile);

	/**************************************/
	/* test ASCII output - no VMs, all mode */

	SAVE_OUTPUT (outfile) {
		ret = clr_oci_list (&config, "table", true);
	}
	ck_assert (ret);

	/* check contents */
	ret = g_file_get_contents (outfile, &contents, NULL, NULL);
	ck_assert (ret);

	lines = g_strsplit (contents, "\n", -1);
	ck_assert (lines);

	ck_assert (lines[0]);
	ck_assert (! *lines[1]);

	/* expect only the header line with fields separated by
	 * whitespace.
	 */
	ret = g_regex_match_simple ("^ID\\s*"
			"PID\\s*"
			"STATUS\\s*"
			"BUNDLE\\s*"
			"CREATED\\s*"
			"HYPERVISOR\\s*"
			"KERNEL\\s*"
			"IMAGE\\s*$",
			contents, 0, 0);
	ck_assert (ret);

	g_free (contents);
	g_strfreev (lines);
	ck_assert (! g_remove (outfile));
	g_free (outfile);

	/*****************************/
	/* test JSON output - no VMs */

	SAVE_OUTPUT (outfile) {
		ret = clr_oci_list (&config, "json", false);
	}
	ck_assert (ret);

	/* check contents */
	ret = g_file_get_contents (outfile, &contents, NULL, NULL);
	ck_assert (ret);

	lines = g_strsplit (contents, "\n", -1);
	ck_assert (lines);

	ck_assert (lines[0]);

	/* the expected empty JSON document */
	ck_assert (! g_strcmp0 (lines[0], "null"));
	ck_assert (! lines[1]);

	g_free (contents);
	g_strfreev (lines);
	ck_assert (! g_remove (outfile));
	g_free (outfile);

	/*****************************/
	/* test JSON output - no VMs, all mode */

	SAVE_OUTPUT (outfile) {
		ret = clr_oci_list (&config, "json", true);
	}
	ck_assert (ret);

	/* check contents */
	ret = g_file_get_contents (outfile, &contents, NULL, NULL);
	ck_assert (ret);

	lines = g_strsplit (contents, "\n", -1);
	ck_assert (lines);

	ck_assert (lines[0]);

	/* the expected empty JSON document */
	ck_assert (! g_strcmp0 (lines[0], "null"));
	ck_assert (! lines[1]);

	g_free (contents);
	g_strfreev (lines);
	ck_assert (! g_remove (outfile));
	g_free (outfile);

	/*******************************/
	/* now, switch to a valid (but empty) directory */

	g_free (config.root_dir);
	config.root_dir = g_strdup (tmpdir);

	/**************************************/
	/* test default ASCII output - no VMs */

	SAVE_OUTPUT (outfile) {
		ret = clr_oci_list (&config, "table", false);
	}
	ck_assert (ret);

	/* check contents */
	ret = g_file_get_contents (outfile, &contents, NULL, NULL);
	ck_assert (ret);

	lines = g_strsplit (contents, "\n", -1);
	ck_assert (lines);

	ck_assert (lines[0]);
	ck_assert (! *lines[1]);

	/* expect only the header line with fields separated by
	 * whitespace.
	 */
	ret = g_regex_match_simple ("^ID\\s*"
			"PID\\s*"
			"STATUS\\s*"
			"BUNDLE\\s*"
			"CREATED\\s*$",
			contents, 0, 0);
	ck_assert (ret);

	g_free (contents);
	g_strfreev (lines);
	ck_assert (! g_remove (outfile));
	g_free (outfile);

	/*****************************/
	/* test JSON output - no VMs */

	SAVE_OUTPUT (outfile) {
		ret = clr_oci_list (&config, "json", false);
	}
	ck_assert (ret);


	/* check contents */
	ret = g_file_get_contents (outfile, &contents, NULL, NULL);
	ck_assert (ret);

	lines = g_strsplit (contents, "\n", -1);
	ck_assert (lines);

	ck_assert (lines[0]);

	/* the expected empty JSON document */
	ck_assert (! g_strcmp0 (lines[0], "null"));
	ck_assert (! lines[1]);

	g_free (contents);
	g_strfreev (lines);
	ck_assert (! g_remove (outfile));
	g_free (outfile);

	/**********************************************************/
	/* now, create a real state file to simulate a running VM */

	ck_assert (test_helper_create_state_file ("vm1", tmpdir,
				&vm1_config));

	/************************************/
	/* test default ASCII output - 1 VM */

	SAVE_OUTPUT (outfile) {
		ret = clr_oci_list (&config, "table", false);
	}
	ck_assert (ret);

	/* check contents */
	ret = g_file_get_contents (outfile, &contents, NULL, NULL);
	ck_assert (ret);

	lines = g_strsplit (contents, "\n", -1);
	ck_assert (lines);

	ck_assert (lines[0]);
	ck_assert (lines[1]);
	ck_assert (! *lines[2]);

	/* expect only the header line with fields separated by
	 * whitespace.
	 */
	ret = g_regex_match_simple ("^ID\\s*"
			"PID\\s*"
			"STATUS\\s*"
			"BUNDLE\\s*"
			"CREATED\\s*$",
			lines[0], 0, 0);
	ck_assert (ret);

	expected = g_strdup_printf ("%s\\s*"
			"%d\\s*"
			"%s\\s*"
			"%s\\s*"
			"%s\\s*",
			vm1_config.optarg_container_id,
			vm1_config.state.workload_pid,
			"created",
			vm1_config.bundle_path,
			"timestamp for vm1");
	ck_assert (expected);

	ret = g_regex_match_simple (expected, lines[1], 0, 0);
	ck_assert (ret);

	g_free (contents);
	g_free (expected);
	g_strfreev (lines);
	ck_assert (! g_remove (outfile));
	g_free (outfile);

	/************************************/
	/* test default ASCII output - 1 VM, all mode */

	SAVE_OUTPUT (outfile) {
		ret = clr_oci_list (&config, "table", true);
	}
	ck_assert (ret);

	/* check contents */
	ret = g_file_get_contents (outfile, &contents, NULL, NULL);
	ck_assert (ret);

	lines = g_strsplit (contents, "\n", -1);
	ck_assert (lines);

	ck_assert (lines[0]);
	ck_assert (lines[1]);
	ck_assert (! *lines[2]);

	/* expect only the header line with fields separated by
	 * whitespace.
	 */
	ret = g_regex_match_simple ("^ID\\s*"
			"PID\\s*"
			"STATUS\\s*"
			"BUNDLE\\s*"
			"CREATED\\s*"
			"HYPERVISOR\\s*"
			"KERNEL\\s*"
			"IMAGE\\s*$",
			lines[0], 0, 0);
	ck_assert (ret);

	expected = g_strdup_printf ("%s\\s*"
			"%d\\s*"
			"%s\\s*"
			"%s\\s*"
			"%s\\s*"
			"%s\\s*"
			"%s\\s*"
			"%s\\s*",
			vm1_config.optarg_container_id,
			vm1_config.state.workload_pid,
			"created",
			vm1_config.bundle_path,
			"timestamp for vm1",
			vm1_config.vm->hypervisor_path,
			vm1_config.vm->kernel_path,
			vm1_config.vm->image_path);
	ck_assert (expected);

	ret = g_regex_match_simple (expected, lines[1], 0, 0);
	ck_assert (ret);

	g_free (contents);
	g_free (expected);
	g_strfreev (lines);
	ck_assert (! g_remove (outfile));
	g_free (outfile);

	/*****************************/
	/* test JSON output - 1 VM */

	SAVE_OUTPUT (outfile) {
		ret = clr_oci_list (&config, "json", false);
	}
	ck_assert (ret);

	parser = json_parser_new ();
	ck_assert (parser);

	ret = json_parser_load_from_file (parser, outfile, NULL);
	ck_assert (ret);

	reader = json_reader_new (NULL);
	ck_assert (reader);

	json_reader_set_root (reader, json_parser_get_root (parser));

	/* "list" returns an array of objects */
	ck_assert (json_reader_is_array (reader));

	/* we expect a single element */
	ck_assert (json_reader_count_elements (reader) == 1);

	/* read element */
	ck_assert (json_reader_read_element (reader, 0));

	ck_assert (json_reader_read_member (reader, "id"));
	node = json_reader_get_value (reader);
	ck_assert (node);
	value = json_node_get_string (node);
	ck_assert (value);
	ck_assert (! g_strcmp0 (value, vm1_config.optarg_container_id));
	json_reader_end_member (reader);

	ck_assert (json_reader_read_member (reader, "status"));
	node = json_reader_get_value (reader);
	ck_assert (node);
	value = json_node_get_string (node);
	ck_assert (value);
	ck_assert (! g_strcmp0 (value, "created"));
	json_reader_end_member (reader);

	ck_assert (json_reader_read_member (reader, "bundle"));
	node = json_reader_get_value (reader);
	ck_assert (node);
	value = json_node_get_string (node);
	ck_assert (value);
	ck_assert (! g_strcmp0 (value, vm1_config.bundle_path));
	json_reader_end_member (reader);

	ck_assert (json_reader_read_member (reader, "created"));
	node = json_reader_get_value (reader);
	ck_assert (node);
	value = json_node_get_string (node);
	ck_assert (value);
	ck_assert (! g_strcmp0 (value, "timestamp for vm1"));
	json_reader_end_member (reader);

	ck_assert (json_reader_read_member (reader, "pid"));
	node = json_reader_get_value (reader);
	ck_assert (node);
	ck_assert (vm1_config.state.workload_pid == json_node_get_int (node));
	json_reader_end_member (reader);

	g_object_unref (reader);
	g_object_unref (parser);

	ck_assert (! g_remove (outfile));
	g_free (outfile);

	/*****************************/
	/* test JSON output - 1 VM, all mode */

	SAVE_OUTPUT (outfile) {
		ret = clr_oci_list (&config, "json", true);
	}
	ck_assert (ret);

	parser = json_parser_new ();
	ck_assert (parser);

	ret = json_parser_load_from_file (parser, outfile, NULL);
	ck_assert (ret);

	reader = json_reader_new (NULL);
	ck_assert (reader);

	json_reader_set_root (reader, json_parser_get_root (parser));

	/* "list" returns an array of objects */
	ck_assert (json_reader_is_array (reader));

	/* we expect a single element */
	ck_assert (json_reader_count_elements (reader) == 1);

	/* read element */
	ck_assert (json_reader_read_element (reader, 0));

	ck_assert (json_reader_read_member (reader, "id"));
	node = json_reader_get_value (reader);
	ck_assert (node);
	value = json_node_get_string (node);
	ck_assert (value);
	ck_assert (! g_strcmp0 (value, vm1_config.optarg_container_id));
	json_reader_end_member (reader);

	ck_assert (json_reader_read_member (reader, "status"));
	node = json_reader_get_value (reader);
	ck_assert (node);
	value = json_node_get_string (node);
	ck_assert (value);
	ck_assert (! g_strcmp0 (value, "created"));
	json_reader_end_member (reader);

	ck_assert (json_reader_read_member (reader, "bundle"));
	node = json_reader_get_value (reader);
	ck_assert (node);
	value = json_node_get_string (node);
	ck_assert (value);
	ck_assert (! g_strcmp0 (value, vm1_config.bundle_path));
	json_reader_end_member (reader);

	ck_assert (json_reader_read_member (reader, "created"));
	node = json_reader_get_value (reader);
	ck_assert (node);
	value = json_node_get_string (node);
	ck_assert (value);
	ck_assert (! g_strcmp0 (value, "timestamp for vm1"));
	json_reader_end_member (reader);

	ck_assert (json_reader_read_member (reader, "hypervisor"));
	node = json_reader_get_value (reader);
	ck_assert (node);
	value = json_node_get_string (node);
	ck_assert (value);
	ck_assert (! g_strcmp0 (value, vm1_config.vm->hypervisor_path));
	json_reader_end_member (reader);

	ck_assert (json_reader_read_member (reader, "kernel"));
	node = json_reader_get_value (reader);
	ck_assert (node);
	value = json_node_get_string (node);
	ck_assert (value);
	ck_assert (! g_strcmp0 (value, vm1_config.vm->kernel_path));
	json_reader_end_member (reader);

	ck_assert (json_reader_read_member (reader, "image"));
	node = json_reader_get_value (reader);
	ck_assert (node);
	value = json_node_get_string (node);
	ck_assert (value);
	ck_assert (! g_strcmp0 (value, vm1_config.vm->image_path));
	json_reader_end_member (reader);

	ck_assert (json_reader_read_member (reader, "pid"));
	node = json_reader_get_value (reader);
	ck_assert (node);
	ck_assert (vm1_config.state.workload_pid == json_node_get_int (node));
	json_reader_end_member (reader);

	g_object_unref (reader);
	g_object_unref (parser);

	ck_assert (! g_remove (outfile));
	g_free (outfile);
	/* clean up */
	ck_assert (! g_remove (vm1_config.state.state_file_path));
	ck_assert (! g_remove (vm1_config.state.runtime_path));

	ck_assert (! g_remove (tmpdir));
	g_free (tmpdir);
	clr_oci_config_free (&vm1_config);
	clr_oci_config_free (&config);
	g_free (vm1_dir);

} END_TEST

START_TEST(test_clr_oci_get_bundle_path) {
	gchar *path;

	ck_assert (! clr_oci_get_bundlepath_file (NULL, NULL));
	ck_assert (! clr_oci_get_bundlepath_file ("", ""));
	ck_assert (! clr_oci_get_bundlepath_file ("", NULL));
	ck_assert (! clr_oci_get_bundlepath_file (NULL, ""));

	path = clr_oci_get_bundlepath_file ("a", "b");
	ck_assert (path);
	ck_assert (! g_strcmp0 (path, "a/b"));
	g_free (path);

	path = clr_oci_get_bundlepath_file ("/a", "b");
	ck_assert (path);
	ck_assert (! g_strcmp0 (path, "/a/b"));
	g_free (path);

	path = clr_oci_get_bundlepath_file ("/a", "/b");
	ck_assert (path);
	ck_assert (! g_strcmp0 (path, "/a/b"));
	g_free (path);

	path = clr_oci_get_bundlepath_file ("/a/", "/b");
	ck_assert (path);
	ck_assert (! g_strcmp0 (path, "/a/b"));
	g_free (path);

	path = clr_oci_get_bundlepath_file ("/a/", "/b/");
	ck_assert (path);
	ck_assert (! g_strcmp0 (path, "/a/b/"));
	g_free (path);
} END_TEST

START_TEST(test_clr_oci_config_update) {
	struct clr_oci_config config = { { 0 } };
	struct oci_state *state;
	struct clr_oci_mount *m;
	struct clr_oci_vm_cfg *vm;
	state = g_malloc0 (sizeof (struct oci_state));
	ck_assert (state);

	ck_assert (! clr_oci_config_update (NULL, NULL));
	ck_assert (! clr_oci_config_update (NULL, state));
	ck_assert (! clr_oci_config_update (&config, NULL));

	ck_assert (! config.oci.mounts);
	ck_assert (! config.console);
	ck_assert (! config.use_socket_console);
	ck_assert (! config.vm);

	/**************************/
	/* setup the state object */

	state->use_socket_console = true;
	state->console = g_strdup ("console");

	/* create mount object */
	m = g_malloc0 (sizeof (struct clr_oci_mount));
	ck_assert (m);
	m->mnt.mnt_fsname = g_strdup ("fsname");
	m->mnt.mnt_dir = g_strdup ("dir");
	m->mnt.mnt_type = g_strdup ("type");
	m->mnt.mnt_opts = g_strdup ("options");

	/* add mount object to state */
	state->mounts = g_slist_append (state->mounts, m);
	ck_assert (g_slist_length (state->mounts) == 1);

	/* create vm object */
	vm = g_malloc0 (sizeof(struct clr_oci_vm_cfg));
	ck_assert (vm);
	g_strlcpy (vm->hypervisor_path, "hypervisor_path", sizeof (vm->hypervisor_path));
	g_strlcpy (vm->image_path, "image_path", sizeof (vm->image_path));
	g_strlcpy (vm->kernel_path, "kernel_path", sizeof (vm->kernel_path));
	g_strlcpy (vm->workload_path, "workload_path", sizeof (vm->workload_path));
	vm->kernel_params = g_strdup ("kernel params");

	/* add vm object to state */
	state->vm = vm;

	/* perform the transfer */
	ck_assert (clr_oci_config_update (&config, state));

	ck_assert (! state->mounts);
	ck_assert (! state->console);
	ck_assert (! state->vm);

	ck_assert (config.oci.mounts);
	ck_assert (config.console);
	ck_assert (config.use_socket_console);
	ck_assert (config.vm);

	ck_assert (g_slist_length (config.oci.mounts) == 1);

	/* check mount object */
	m = (struct clr_oci_mount *)g_slist_nth_data (config.oci.mounts, 0);
	ck_assert (m);
	ck_assert (! g_strcmp0 (m->mnt.mnt_fsname, "fsname"));
	ck_assert (! g_strcmp0 (m->mnt.mnt_dir, "dir"));
	ck_assert (! g_strcmp0 (m->mnt.mnt_type, "type"));
	ck_assert (! g_strcmp0 (m->mnt.mnt_opts, "options"));

	/* check console */
	ck_assert (! g_strcmp0 (config.console, "console"));

	/* check vm object */
	ck_assert (! g_strcmp0 (config.vm->hypervisor_path, "hypervisor_path"));
	ck_assert (! g_strcmp0 (config.vm->image_path, "image_path"));
	ck_assert (! g_strcmp0 (config.vm->kernel_path, "kernel_path"));
	ck_assert (! g_strcmp0 (config.vm->workload_path, "workload_path"));
	ck_assert (! g_strcmp0 (config.vm->kernel_params, "kernel params"));

	/* clean up */
	clr_oci_config_free (&config);
	clr_oci_state_free (state);

} END_TEST

START_TEST(test_clr_oci_get_config_and_state) {
	struct clr_oci_config config = { { 0 } };
	struct clr_oci_config vm1_config = { { 0 } };
	struct oci_state *state;
	gchar *config_file = NULL;
	gchar *tmpdir;
	g_autofree gchar *path = NULL;

	tmpdir = g_dir_make_tmp (NULL, NULL);
	ck_assert (tmpdir);

	ck_assert (! clr_oci_get_config_and_state (NULL, NULL, NULL));
	ck_assert (! clr_oci_get_config_and_state (NULL, &config, &state));
	ck_assert (! clr_oci_get_config_and_state (NULL, NULL, &state));
	ck_assert (! clr_oci_get_config_and_state (&config_file, NULL, NULL));
	ck_assert (! clr_oci_get_config_and_state (NULL, &config, NULL));
	ck_assert (! clr_oci_get_config_and_state (&config_file, &config, NULL));

	/* no container id */
	ck_assert (! clr_oci_get_config_and_state (&config_file, &config, &state));

	/* create a VM state file */
	ck_assert (test_helper_create_state_file ("vm1", tmpdir, &vm1_config));
	ck_assert (! vm1_config.oci.mounts);

	config.optarg_container_id = "vm1";
	config.root_dir = g_strdup (tmpdir);
	ck_assert (config.root_dir);

	/* load details of vm1_config into config and state */
	ck_assert (clr_oci_get_config_and_state (&config_file, &config, &state));

	/* check config */
	ck_assert (! g_strcmp0 (config.state.runtime_path, vm1_config.state.runtime_path));
	ck_assert (! g_strcmp0 (config.state.state_file_path, vm1_config.state.state_file_path));
	ck_assert (! g_strcmp0 (config.state.comms_path, vm1_config.state.comms_path));
	ck_assert (! g_strcmp0 (config.bundle_path, vm1_config.bundle_path));

	ck_assert (config.state.workload_pid == vm1_config.state.workload_pid);
	ck_assert (! config.oci.mounts);

	ck_assert (config_file);
	path = g_build_path ("/", config.bundle_path, "config.json", NULL);
	ck_assert (! g_strcmp0 (path, config_file));

	ck_assert (! config.vm);

	/* check state */
	ck_assert (state);
	ck_assert (! g_strcmp0 (state->oci_version, "1.0.0-rc1"));
	ck_assert (! g_strcmp0 (state->id, config.optarg_container_id));

	ck_assert (state->pid == config.state.workload_pid);
	ck_assert (state->status == OCI_STATUS_CREATED);
	ck_assert (! g_strcmp0 (state->bundle_path, config.bundle_path));
	ck_assert (! g_strcmp0 (state->comms_path, config.state.comms_path));

	ck_assert (! g_strcmp0 (state->console, vm1_config.console));
	ck_assert (! config.console);
	ck_assert (! g_strcmp0 (state->create_time, "timestamp for vm1"));
	ck_assert (! state->mounts);

	ck_assert (state->vm);
	ck_assert (! g_strcmp0 (state->vm->hypervisor_path, vm1_config.vm->hypervisor_path));
	ck_assert (! g_strcmp0 (state->vm->image_path, vm1_config.vm->image_path));
	ck_assert (! g_strcmp0 (state->vm->kernel_path, vm1_config.vm->kernel_path));
	ck_assert (! g_strcmp0 (state->vm->workload_path, vm1_config.vm->workload_path));
	ck_assert (state->vm->kernel_params);
	ck_assert (! g_strcmp0 (state->vm->kernel_params, vm1_config.vm->kernel_params));

	/* clean up */
	ck_assert (! g_remove (vm1_config.state.state_file_path));
	ck_assert (! g_remove (vm1_config.state.runtime_path));

	ck_assert (! g_remove (tmpdir));
	g_free (tmpdir);
	clr_oci_config_free (&vm1_config);
	clr_oci_config_free (&config);
	g_free (config_file);
	clr_oci_state_free (state);

} END_TEST

START_TEST(test_clr_oci_vm_running) {
	struct oci_state state = {0};

	ck_assert (! clr_oci_vm_running (NULL));

	/* no pid */
	ck_assert (! clr_oci_vm_running (&state));

	/* our pid */
	state.pid = getpid ();
	ck_assert (clr_oci_vm_running (&state));

	/* invalid pid (we hope: this is potential an unreliable test).
	 */
	state.pid = (pid_t)INT_MAX;
	ck_assert (! clr_oci_vm_running (&state));

} END_TEST

Suite* make_oci_suite(void) {
	Suite* s = suite_create(__FILE__);

	ADD_TEST (test_clr_oci_list, s);
	ADD_TEST (test_clr_oci_get_bundle_path, s);
	ADD_TEST (test_clr_oci_config_update, s);
	ADD_TEST (test_clr_oci_get_config_and_state, s);
	ADD_TEST (test_clr_oci_vm_running, s);

	return s;
}

gboolean enable_debug = true;

int main (void) {
	int number_failed;
	Suite* s;
	SRunner* sr;
	struct clr_log_options options = { 0 };

	options.use_json = false;
	options.filename = g_strdup ("oci_test_debug.log");
	(void)clr_oci_log_init(&options);

	s = make_oci_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	clr_oci_log_free (&options);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
