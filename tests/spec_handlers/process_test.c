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
#include <stdlib.h>

#include <check.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "../test_common.h"
#include "../../src/logging.h"

extern struct spec_handler process_spec_handler;

static struct spec_handler_test tests[] = {
	{ TEST_DATA_DIR "/process-invalid-relative-cwd.json" , false },
	{ TEST_DATA_DIR "/process-no-cwd.json"               , false },
	{ TEST_DATA_DIR "/process-no-args-cwd.json"          , false },
	{ TEST_DATA_DIR "/process.json"                      , true  },
	{ NULL                                               , false },
};

START_TEST(test_process_handle_section) {

	test_spec_handler(&process_spec_handler, tests);

} END_TEST

Suite* make_process_suite(void) {
	Suite* s = suite_create(__FILE__);

	ADD_TEST(test_process_handle_section, s);

	return s;
}

gboolean enable_debug = true;

int main(void) {
	int number_failed;
	Suite* s;
	SRunner* sr;
	struct clr_log_options options = { 0 };
	gchar *cwd = g_get_current_dir();

	options.use_json = false;
	options.filename = g_build_path ("/",
			cwd,
			"process_spechandler_test_debug.log", NULL);
	(void)clr_oci_log_init(&options);

	s = make_process_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	clr_oci_log_free (&options);

	g_free (cwd);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
