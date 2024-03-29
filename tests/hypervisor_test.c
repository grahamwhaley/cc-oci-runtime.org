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

#include <check.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "test_common.h"
#include "../src/logging.h"
#include "../src/hypervisor.h"
#include "../src/oci.h"

gchar *
clr_oci_vm_args_file_path (const struct clr_oci_config *config);
gboolean clr_oci_expand_cmdline (struct clr_oci_config *config,
		gchar **args);

extern gchar *sysconfdir;
extern gchar *defaultsdir;

static gboolean
check_full_expansion (struct clr_oci_config *config,
		const gchar **args, const char *image_size)
{
	if (! config || ! args || ! image_size) {
		return false;
	}

	if (g_strcmp0 (args[0], config->oci.root.path)) {
		return false;
	}

	if (g_strcmp0 (args[1], config->vm->kernel_path)) {
		return false;
	}
	if (g_strcmp0 (args[2], "param1=foo param2=bar")) {
		return false;
	}
	if (g_strcmp0 (args[3], config->vm->image_path)) {
		return false;
	}
	if (g_strcmp0 (args[4], image_size)) {
		return false;
	}
	if (g_strcmp0 (args[5], "comms-path")) {
		return false;
	}
	if (g_strcmp0 (args[6], "stdio,id=charconsole0,signal=off")) {
		return false;
	}

	/* NAME is comprised of purely alpha-numeric chars */
	if (! g_regex_match_simple ("^[\\S\\D]*$", args[7], 0, 0)) {
		return false;
	}

	/* UUID is like NAME, but longer and contains dashes */
	if (! g_regex_match_simple ("^[\\S\\D-]*$", args[8], 0, 0)) {
		return false;
	}

	/* UUID is of form "A-B-B-B-C"
	 *
	 * Where,
	 *
	 * A = 8 bytes.
	 * B = 4 bytes.
	 * C = 12 bytes.
	 *
	 * Length is therefore: A + B + C + 4 (for the dashes).
	 */
#define EXPECTED_UUID_LEN (8 + 4 + 4 + 4 + 12 + 4)
	if (! (g_utf8_strlen (args[8], EXPECTED_UUID_LEN) == EXPECTED_UUID_LEN)) {
		return false;
	}

#undef EXPECTED_UUID_LEN

	if (args[9] != NULL) {
		return false;
	}

	return true;
}

START_TEST(test_clr_oci_vm_args_file_path) {
	gboolean ret;
	struct clr_oci_config config = { { 0 } };
	gchar *tmpdir;
	gchar *args_file;

	gchar *tmp_sysconfdir;
	gchar *tmp_sysconf_args_file;

	gchar *tmp_defaultsdir;
	gchar *tmp_defaults_args_file;

	gchar *path;
	gchar *invalid_dir = "/this/directory/must/not/exist";


	ret = g_file_test (invalid_dir,
			(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR));

	/* invalid_dir must not exist :) */
	ck_assert (! ret);

	/* ensure the default locations are invalid */
	sysconfdir = invalid_dir;
	defaultsdir = invalid_dir;

	tmpdir = g_dir_make_tmp (NULL, NULL);
	ck_assert (tmpdir);

	tmp_sysconfdir = g_build_path ("/", tmpdir, "sysconfdir", NULL);
	tmp_defaultsdir = g_build_path ("/", tmpdir, "defaultsdir", NULL);

	/* create our own local default locations */
	ck_assert (! g_mkdir (tmp_sysconfdir, 0750));
	ck_assert (! g_mkdir (tmp_defaultsdir, 0750));

	args_file = g_build_path ("/", tmpdir,
			"hypervisor.args", NULL);
	tmp_sysconf_args_file = g_build_path ("/", tmp_sysconfdir,
			"hypervisor.args", NULL);
	tmp_defaults_args_file = g_build_path ("/", tmp_defaultsdir,
			"hypervisor.args", NULL);

	/* invalid config specified */
	ck_assert (! clr_oci_vm_args_file_path (NULL));

	/* no bundle path in config */
	ck_assert (! clr_oci_vm_args_file_path (&config));

	config.bundle_path = g_strdup (tmpdir);

	/* bundle path is now set, but no args file exists there,
	 * nor in any of the default locations.
	 */
	ck_assert (! clr_oci_vm_args_file_path (&config));

	/* switch to valid (but empty) default directories */
	sysconfdir = tmp_sysconfdir;
	defaultsdir = tmp_defaultsdir;

	ck_assert (! clr_oci_vm_args_file_path (&config));

	/* create an empty bundle_path file */
	ret = g_file_set_contents (args_file, "", -1, NULL);
	ck_assert (ret);

	path = clr_oci_vm_args_file_path (&config);
	ck_assert (path);
	ck_assert (! g_strcmp0 (path, args_file));
	g_free (path);

	/* remove the file */
	ck_assert (! g_remove (args_file));

	/* create an empty sysconf file */
	ret = g_file_set_contents (tmp_sysconf_args_file, "", -1, NULL);
	ck_assert (ret);

	path = clr_oci_vm_args_file_path (&config);
	ck_assert (path);

	ck_assert (! g_strcmp0 (path, tmp_sysconf_args_file));
	g_free (path);

	/* remove the file */
	ck_assert (! g_remove (tmp_sysconf_args_file));

	/* create an empty defaults file */
	ret = g_file_set_contents (tmp_defaults_args_file, "", -1, NULL);
	ck_assert (ret);

	path = clr_oci_vm_args_file_path (&config);
	ck_assert (path);
	ck_assert (! g_strcmp0 (path, tmp_defaults_args_file));
	g_free (path);

	/* remove the file */
	ck_assert (! g_remove (tmp_defaults_args_file));

	/* clean up */
	ck_assert (! g_remove (tmp_sysconfdir));
	ck_assert (! g_remove (tmp_defaultsdir));
	ck_assert (! g_remove (tmpdir));

	g_free (tmpdir);
	g_free (tmp_sysconfdir);
	g_free (tmp_defaultsdir);

	g_free (args_file);
	g_free (tmp_sysconf_args_file);
	g_free (tmp_defaults_args_file);

	clr_oci_config_free (&config);
} END_TEST

START_TEST(test_clr_oci_expand_cmdline) {
	gboolean ret;
	gchar **args;
	gchar *tmpdir;
	gchar *path;
	gchar  image_contents[] = "hello world";
	gchar *image_size = g_strdup_printf ("%lu",
			(unsigned long int)sizeof(image_contents)-1);
	gchar *shell;

	struct clr_oci_config config = { { 0 } };

	tmpdir = g_dir_make_tmp (NULL, NULL);
	ck_assert (tmpdir);

	ck_assert (! clr_oci_expand_cmdline (NULL, NULL));
	ck_assert (! clr_oci_expand_cmdline (NULL, NULL));

	args = g_new0 (gchar *, 3);
	ck_assert (args);
	args[0] = g_strdup ("");
	args[1] = g_strdup ("");
	args[2] = NULL;

	/* no config */
	ck_assert (! clr_oci_expand_cmdline (NULL, args));

	/* no config.vm */
	ck_assert (! clr_oci_expand_cmdline (&config, args));

	config.vm = g_new0 (struct clr_oci_vm_cfg, 1);
	ck_assert (config.vm);

	path = g_build_path ("/", tmpdir, "image", NULL);
	ck_assert (path);
	g_strlcpy (config.vm->image_path, path,
			sizeof (config.vm->image_path));
	g_free (path);

	/* image_path is ENOENT */
	ck_assert (! clr_oci_expand_cmdline (&config, args));

	/* create image_path */
	ret = g_file_set_contents (config.vm->image_path,
			image_contents, -1, NULL);
	ck_assert (ret);

	/* no kernel path */
	ck_assert (! clr_oci_expand_cmdline (&config, args));

	path = g_build_path ("/", tmpdir, "vmlinux", NULL);
	ck_assert (path);
	g_strlcpy (config.vm->kernel_path, path,
			sizeof (config.vm->kernel_path));
	g_free (path);

	/* kernel_path is ENOENT */
	ck_assert (! clr_oci_expand_cmdline (&config, args));

	/* create kernel_path */
	ret = g_file_set_contents (config.vm->kernel_path, "", -1, NULL);
	ck_assert (ret);

	/* no root.path */
	ck_assert (! clr_oci_expand_cmdline (&config, args));

	path = g_build_path ("/", tmpdir, "workload", NULL);
	ck_assert (path);
	g_strlcpy (config.oci.root.path, path,
			sizeof (config.oci.root.path));
	g_free (path);

	ck_assert (! g_mkdir (config.oci.root.path, 0750));

	g_strlcpy (config.state.comms_path, "comms-path",
			sizeof (config.state.comms_path));

	ck_assert (! config.console);
	ck_assert (clr_oci_expand_cmdline (&config, args));
	/* console should have been set */
	ck_assert (config.console);
	g_free (config.console);

	/* ensure no expansion took place */
	ck_assert (! g_strcmp0 (args[0], ""));
	ck_assert (! g_strcmp0 (args[1], ""));
	ck_assert (args[2] == NULL);
	g_strfreev (args);

	args = g_new0 (gchar *, 3);
	ck_assert (args);
	args[0] = g_strdup ("@foo@bar@baz");
	args[1] = g_strdup ("@@@@@@@@@@@");
	args[2] = NULL;

	config.console = g_strdup ("console device");
	config.vm->kernel_params = g_strdup ("param1=foo param2=bar");
	ck_assert (clr_oci_expand_cmdline (&config, args));
	ck_assert (! g_strcmp0 (config.console, "console device"));

	/* ensure no expansion took place */
	ck_assert (! g_strcmp0 (args[0], "@foo@bar@baz"));
	ck_assert (! g_strcmp0 (args[1], "@@@@@@@@@@@"));
	ck_assert (! args[2]);
	g_strfreev (args);

	args = g_new0 (gchar *, 10);
	ck_assert (args);
	args[0] = g_strdup ("@WORKLOAD_DIR@");
	args[1] = g_strdup ("@KERNEL@");
	args[2] = g_strdup ("@KERNEL_PARAMS@");
	args[3] = g_strdup ("@IMAGE@");
	args[4] = g_strdup ("@SIZE@");
	args[5] = g_strdup ("@COMMS_SOCKET@");
	args[6] = g_strdup ("@CONSOLE_DEVICE@");
	args[7] = g_strdup ("@NAME@");
	args[8] = g_strdup ("@UUID@");
	args[9] = NULL;

	ck_assert (clr_oci_expand_cmdline (&config, args));
	ck_assert (check_full_expansion (&config,
				(const gchar **)args, image_size));
	g_strfreev (args);

	/* check expansion of first param if relative */
	shell = g_find_program_in_path ("sh");
	ck_assert (shell);

	args = g_new0 (gchar *, 2);
	ck_assert (args);
	args[0] = g_strdup ("sh");
	args[1] = NULL;

	ck_assert (clr_oci_expand_cmdline (&config, args));
	ck_assert (! g_strcmp0 (args[0], shell));
	ck_assert (! args[1]);

	g_free (args[0]);
	/* test with an already specified absolute path */
	args[0] = g_strdup (shell);

	ck_assert (clr_oci_expand_cmdline (&config, args));
	ck_assert (! g_strcmp0 (args[0], shell));

	g_free (shell);
	g_strfreev (args);

	/* clean up */
	ck_assert (! g_remove (config.vm->image_path));
	ck_assert (! g_remove (config.vm->kernel_path));
	ck_assert (! g_remove (config.oci.root.path));
	ck_assert (! g_remove (tmpdir));
	g_free (tmpdir);

	g_free (image_size);
	clr_oci_config_free (&config);
} END_TEST

START_TEST(test_clr_oci_vm_args_get) {
	gboolean ret;
	gchar *path;
	gchar **args = NULL;
	gchar *tmpdir;
	struct clr_oci_config config = { { 0 } };
	gchar *args_file;
	gchar  image_contents[] = "hello world";
	gchar *image_size = g_strdup_printf ("%lu",
			(unsigned long int)sizeof(image_contents)-1);
	gchar *invalid_dir = "/this/directory/must/not/exist";

	tmpdir = g_dir_make_tmp (NULL, NULL);
	ck_assert (tmpdir);

	ret = g_file_test (invalid_dir,
			(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR));

	/* invalid_dir must not exist :) */
	ck_assert (! ret);

	/* ensure the default locations are invalid */
	sysconfdir = invalid_dir;
	defaultsdir = invalid_dir;

	args_file = g_build_path ("/", tmpdir,
			"hypervisor.args", NULL);
	ck_assert (args_file);

	ck_assert (! clr_oci_vm_args_get (NULL, NULL));

	ck_assert (! clr_oci_vm_args_get (NULL, &args));
	ck_assert (! clr_oci_vm_args_get (&config, NULL));

	/* no bundle_path */
	ck_assert (! clr_oci_vm_args_get (&config, &args));

	config.bundle_path = g_strdup (tmpdir);

	/* no config.vm */
	ck_assert (! clr_oci_vm_args_get (&config, &args));

	config.vm = g_new0 (struct clr_oci_vm_cfg, 1);
	ck_assert (config.vm);

	config.vm->kernel_params = g_strdup ("param1=foo param2=bar");
	g_strlcpy (config.state.comms_path, "comms-path",
			sizeof (config.state.comms_path));

	path = g_build_path ("/", tmpdir, "image", NULL);
	ck_assert (path);
	g_strlcpy (config.vm->image_path, path,
			sizeof (config.vm->image_path));
	g_free (path);

	/* create image_path */
	ret = g_file_set_contents (config.vm->image_path,
			image_contents, -1, NULL);
	ck_assert (ret);

	/* no kernel path */
	ck_assert (! clr_oci_expand_cmdline (&config, args));

	path = g_build_path ("/", tmpdir, "vmlinux", NULL);
	ck_assert (path);
	g_strlcpy (config.vm->kernel_path, path,
			sizeof (config.vm->kernel_path));
	g_free (path);

	/* create kernel_path */
	ret = g_file_set_contents (config.vm->kernel_path, "", -1, NULL);
	ck_assert (ret);

	/* no root.path */
	ck_assert (! clr_oci_expand_cmdline (&config, args));

	path = g_build_path ("/", tmpdir, "workload", NULL);
	ck_assert (path);
	g_strlcpy (config.oci.root.path, path,
			sizeof (config.oci.root.path));
	g_free (path);

	/* create root path */
	ck_assert (! g_mkdir (config.oci.root.path, 0750));

	/* bundle path is now set, but no args file exists there.
	 */
	ck_assert (! clr_oci_vm_args_get (&config, &args));

	/* create an empty bundle_path file */
	ret = g_file_set_contents (args_file, "", -1, NULL);
	ck_assert (ret);

	/* an empty string cannot be split into lines */
	ck_assert (! clr_oci_vm_args_get (&config, &args));

	/* recreate the args file */
	ret = g_file_set_contents (args_file, "\n", -1, NULL);
	ck_assert (ret);

	ck_assert (clr_oci_vm_args_get (&config, &args));

	ck_assert (! g_strcmp0 (args[0], ""));
	ck_assert (! args[1]);
	g_strfreev (args);

	/* recreate the args file */
	ret = g_file_set_contents (args_file, "foo", -1, NULL);
	ck_assert (ret);

	ck_assert (clr_oci_vm_args_get (&config, &args));

	ck_assert (! g_strcmp0 (args[0], "foo"));
	ck_assert (! args[1]);
	g_strfreev (args);

	/* recreate the args file */
	ret = g_file_set_contents (args_file,
			"hello\nworld\nfoo\nbar",
			-1, NULL);
	ck_assert (ret);

	ck_assert (clr_oci_vm_args_get (&config, &args));

	ck_assert (! g_strcmp0 (args[0], "hello"));
	ck_assert (! g_strcmp0 (args[1], "world"));
	ck_assert (! g_strcmp0 (args[2], "foo"));
	ck_assert (! g_strcmp0 (args[3], "bar"));
	ck_assert (! args[4]);
	g_strfreev (args);

	/* recreate the args file with expandable lines */
	ret = g_file_set_contents (args_file,
			"@WORKLOAD_DIR@\n"
			"@KERNEL@\n"
			"@KERNEL_PARAMS@\n"
			"@IMAGE@\n"
			"@SIZE@\n"
			"@COMMS_SOCKET@\n"
			"@CONSOLE_DEVICE@\n"
			"@NAME@\n"
			"@UUID@\n",
			-1, NULL);
	ck_assert (ret);

	ck_assert (clr_oci_vm_args_get (&config, &args));
	ck_assert (check_full_expansion (&config, (const gchar **)args,
				image_size));
	g_strfreev (args);

	/* clean up */
	ck_assert (! g_remove (args_file));
	ck_assert (! g_remove (config.vm->image_path));
	ck_assert (! g_remove (config.vm->kernel_path));
	ck_assert (! g_remove (config.oci.root.path));
	ck_assert (! g_remove (tmpdir));

	g_free (args_file);
	g_free (tmpdir);
	g_free (image_size);
	clr_oci_config_free (&config);
} END_TEST

Suite* make_hypervisor_suite(void) {
	Suite* s = suite_create(__FILE__);

	ADD_TEST(test_clr_oci_vm_args_file_path, s);
	ADD_TEST(test_clr_oci_expand_cmdline, s);
	ADD_TEST(test_clr_oci_vm_args_get, s);

	return s;
}

gboolean enable_debug = true;

int main (void) {
	int number_failed;
	Suite* s;
	SRunner* sr;
	struct clr_log_options options = { 0 };

	options.use_json = false;
	options.filename = g_strdup ("hypervisor_test_debug.log");
	(void)clr_oci_log_init(&options);

	s = make_hypervisor_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	clr_oci_log_free (&options);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
