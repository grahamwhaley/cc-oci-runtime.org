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
#include <signal.h>

#include "command.h"

static gboolean
handler_kill (const struct subcommand *sub,
		struct clr_oci_config *config,
		int argc, char *argv[])
{
	struct oci_state      *state = NULL;
	gchar                 *config_file = NULL;
	gboolean               ret = false;
	const gchar           *signame = NULL;
	int                    signum = SIGTERM;

	g_assert (sub);
	g_assert (config);

	if (argc && ((!g_strcmp0 (argv[0], "--help"))
			|| (!g_strcmp0 (argv[0], "-h")))) {
		g_print ("Usage: %s <container-id> [<signal>]\n",
				sub->name);
		return true;
	}

	if (argc < 1) {
		g_critical ("%s: need container id", sub->name);
		return false;
	}

	/* Used to allow us to find the state file */
	config->optarg_container_id = argv[0];

	if (argc == 2) {
		signame = argv[1];

		/* first, try to convert the string argument to a number */
		signum = atoi (signame);

		if (signum <= 0) {
			/* not a number, so try to convert the signame
			 * name to a number.
			 */
			signum = clr_oci_get_signum (signame);
		}

		if (signum < 0) {
			g_critical ("invalid signal specified: %s",
					signame);
			return false;
		}
	}

	ret = clr_oci_get_config_and_state (&config_file, config, &state);
	if (! ret) {
		goto out;
	}

	if (! clr_oci_config_update (config, state)) {
		goto out;
	}

	ret = clr_oci_kill (config, state, signum);

out:
	g_free_if_set (config_file);
	clr_oci_state_free (state);

	return ret;
}

struct subcommand command_kill =
{
	.name        = "kill",
	.handler     = handler_kill,
	.description = "send a signal to the container "
		       "(signal may be symbolic (\"SIGKILL\"/\"KILL\") "
		       "or numeric (\"9\"))"
};
