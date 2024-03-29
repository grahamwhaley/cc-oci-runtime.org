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

#include "command.h"
#include "oci.h"
#include "../src/logging.h"
#include "priv.h"

START_TEST(test_clr_oci_get_priv_level) {
	int                    argc = 2;
	gchar                 *argv[] = { "IGNORED", "arg", NULL };
	struct clr_oci_config  config = { {0} };
	struct subcommand      sub;
	gchar                 *tmpdir = g_dir_make_tmp (NULL, NULL);
	gchar                 *tmpdir_enoent = g_build_path ("/", tmpdir, "foo", NULL);

	sub.name = "help";
	ck_assert (clr_oci_get_priv_level (argc, argv, &sub, &config) == -1);

	sub.name = "version";
	ck_assert (clr_oci_get_priv_level (argc, argv, &sub, &config) == -1);

	sub.name = "list";
	argv[1] = "--help";
	ck_assert (clr_oci_get_priv_level (argc, argv, &sub, &config) == -1);

	sub.name = "list";
	argv[1] = "-h";
	ck_assert (clr_oci_get_priv_level (argc, argv, &sub, &config) == -1);

	/* set to a non-"--help" value */
	argv[1] = "foo";

	/* root_dir not specified, so root required */
	ck_assert (clr_oci_get_priv_level (argc, argv, &sub, &config) == 1);

	config.root_dir = "/";

	if (getuid ()) {
		/* non-root cannot write to "/" */
		ck_assert (clr_oci_get_priv_level (argc, argv, &sub, &config) == 1);
	} else {
		/* root can write to "/" */
		ck_assert (clr_oci_get_priv_level (argc, argv, &sub, &config) == 0);
	}

	config.root_dir = tmpdir;

	ck_assert (clr_oci_get_priv_level (argc, argv, &sub, &config) == 0);

	/* make directory inaccessible to non-root */
	ck_assert (! g_chmod (tmpdir, 0));

	if (getuid ()) {
		ck_assert (clr_oci_get_priv_level (argc, argv, &sub, &config) == 1);
	} else {
		/* root can write to any directory */
		ck_assert (clr_oci_get_priv_level (argc, argv, &sub, &config) == 0);
	}

	/* make directory accessible once again */
	ck_assert (! g_chmod (tmpdir, 0755));

	/* specify a non-existing directory */
	config.root_dir = tmpdir_enoent;

	if (getuid ()) {
		/* parent directory does exist so no extra privs
		 * required.
		 */
		ck_assert (clr_oci_get_priv_level (argc, argv, &sub, &config) == 0);
	} else {
		/* root can write to any directory */
		ck_assert (clr_oci_get_priv_level (argc, argv, &sub, &config) == 0);
	}

	/* make parent directory inaccessible to non-root again */
	ck_assert (! g_chmod (tmpdir, 0));

	if (getuid ()) {
		ck_assert (clr_oci_get_priv_level (argc, argv, &sub, &config) == 1);
	} else {
		/* root can write to any directory */
		ck_assert (clr_oci_get_priv_level (argc, argv, &sub, &config) == 0);
	}

	/* make parent directory accessible once again */
	ck_assert (! g_chmod (tmpdir, 0755));

	if (getuid ()) {
		ck_assert (clr_oci_get_priv_level (argc, argv, &sub, &config) == 0);
	} else {
		/* root can write to any directory */
		ck_assert (clr_oci_get_priv_level (argc, argv, &sub, &config) == 0);
	}

	/* clean up */
	ck_assert (! g_remove (tmpdir));
	g_free (tmpdir);
	g_free (tmpdir_enoent);
} END_TEST

Suite* make_priv_suite(void) {
	Suite* s = suite_create(__FILE__);

	ADD_TEST(test_clr_oci_get_priv_level, s);

	return s;
}

gboolean enable_debug = true;

int main(void) {
	int number_failed;
	Suite* s;
	SRunner* sr;
	struct clr_log_options options = { 0 };

	options.use_json = false;
	options.filename = g_strdup ("priv_test_debug.log");
	(void)clr_oci_log_init(&options);

	s = make_priv_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	clr_oci_log_free (&options);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
