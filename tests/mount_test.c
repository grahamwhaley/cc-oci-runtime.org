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

#include <check.h>
#include <glib.h>

#include "../src/mount.h"
#include "../src/logging.h"
#include "../src/json.h"
#include "../src/util.h"
#include "test_common.h"

gboolean clr_oci_mount_ignore (struct clr_oci_mount *m);
gboolean clr_oci_perform_mount (const struct clr_oci_mount *m, gboolean dry_run);
gboolean clr_oci_perform_unmount (const struct clr_oci_mount *m);

extern struct spec_handler mounts_spec_handler;

START_TEST(test_clr_oci_mount_ignore) {
	struct clr_oci_mount m = { 0 };

	ck_assert(! clr_oci_mount_ignore(NULL));

	ck_assert(! clr_oci_mount_ignore(&m));

	m.mnt.mnt_dir = "/";
	ck_assert(! clr_oci_mount_ignore(&m));

	m.mnt.mnt_dir = "/proc";
	ck_assert(clr_oci_mount_ignore(&m));
} END_TEST

START_TEST(test_clr_oci_perform_mount) {
	struct clr_oci_mount m = { 0 };

	ck_assert(! clr_oci_perform_mount(NULL, false));

	g_snprintf(m.dest, PATH_MAX, "/tmp");
	m.mnt.mnt_fsname = "/tmp";
	m.mnt.mnt_type = "tmpfs";

	if (getuid ()) {
		/* don't run this test if we are root */
		ck_assert(! clr_oci_perform_mount(&m, false));
	}

	ck_assert(clr_oci_perform_mount(&m, true));
} END_TEST

START_TEST(test_clr_oci_handle_mounts) {
	struct clr_oci_config config = { { 0 } };
	GNode* node = NULL;

	ck_assert(! clr_oci_handle_mounts(NULL));

	clr_oci_json_parse(&node, TEST_DATA_DIR "/mounts.json");
	mounts_spec_handler.handle_section(
	    node_find_child(node, mounts_spec_handler.name), &config);

	ck_assert(! clr_oci_handle_mounts(&config));

	config.dry_run_mode = true;
	ck_assert(clr_oci_handle_mounts(&config));

	clr_oci_config_free(&config);
	g_free_node(node);
} END_TEST

START_TEST(test_clr_oci_perform_unmount) {
	struct clr_oci_mount m = { 0 };
	ck_assert(! clr_oci_perform_unmount(NULL));

	ck_assert(! clr_oci_perform_unmount(&m));
} END_TEST

START_TEST(test_clr_oci_handle_umounts) {
	struct clr_oci_config config = { { 0 } };
	GNode* node = NULL;

	ck_assert(! clr_oci_handle_unmounts(NULL));

	clr_oci_json_parse(&node, TEST_DATA_DIR "/mounts.json");
	mounts_spec_handler.handle_section(
	    node_find_child(node, mounts_spec_handler.name), &config);

	ck_assert(! clr_oci_handle_unmounts(&config));

	clr_oci_config_free(&config);
	g_free_node(node);
} END_TEST

Suite* make_mount_suite(void) {
	Suite* s = suite_create(__FILE__);

	ADD_TEST(test_clr_oci_mount_ignore, s);
	ADD_TEST(test_clr_oci_perform_mount, s);
	ADD_TEST(test_clr_oci_handle_mounts, s);
	ADD_TEST(test_clr_oci_perform_unmount, s);
	ADD_TEST(test_clr_oci_handle_umounts, s);

	return s;
}

gboolean enable_debug = true;

int main(void) {
	int number_failed;
	Suite* s;
	SRunner* sr;
	struct clr_log_options options = { 0 };

	options.use_json = false;
	options.filename = g_strdup ("mount_test_debug.log");
	(void)clr_oci_log_init(&options);

	s = make_mount_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	clr_oci_log_free (&options);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
