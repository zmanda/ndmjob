/*
 * Copyright (c) 2000
 *	Traakan, Inc., Los Altos, CA
 *	All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Project:  NDMJOB
 * Ident:    $Id: ndma_data_butype.c,v 1.1 2003/10/14 19:12:39 ern Exp $
 *
 * Description:
 *
 */

#define NDMOS_OPTION_NO_DUMP_BUTYPE

#include "ndmagents.h"

#ifndef NDMOS_OPTION_NO_DATA_AGENT


struct ndm_data_butype	ndmda_data_butype_table[] = {
#ifndef NDMOS_OPTION_NO_GTAR_BUTYPE
	{
		"tar",
		ndmda_butype_gtar_config_get_attrs,
		ndmda_butype_gtar_config_get_default_env,
		ndmda_butype_gtar_attach
	},
	{
		"gtar",
		ndmda_butype_gtar_config_get_attrs,
		ndmda_butype_gtar_config_get_default_env,
		ndmda_butype_gtar_attach
	},
#endif /* NDMOS_OPTION_NO_TAR_BUTYPE */
#ifndef NDMOS_OPTION_NO_DUMP_BUTYPE
	{
		"dump",
		ndmda_butype_dump_config_get_attrs,
		ndmda_butype_dump_config_get_default_env,
		ndmda_butype_dump_attach
	},
#endif /* NDMOS_OPTION_NO_DUMP_BUTYPE */
#ifdef NDMOS_MACRO_BUTYPE_TABLE_ADDITIONS
	NDMOS_MACRO_BUTYPE_TABLE_ADDITIONS
#endif /* NDMOS_MACRO_BUTYPE_TABLE_ADDITIONS */
	{ 0 },
};

#define N_BUTYPE \
    ((sizeof ndmda_data_butype_table / sizeof ndmda_data_butype_table[0]) - 1)

struct ndm_data_butype *
ndmda_find_butype_by_name (char *name)
{
	int				i;
	struct ndm_data_butype *	bt;

	for (i = 0; i < N_BUTYPE; i++) {
		bt = &ndmda_data_butype_table[i];

		if (strcmp (bt->name, name) == 0)
			return bt;
	}

	return 0;
}

struct ndm_data_butype *
ndmda_find_butype_by_index (int index)
{
	if (index < 0 || index >= N_BUTYPE)
		return 0;

	return &ndmda_data_butype_table[index];
}

#endif /* !NDMOS_OPTION_NO_DATA_AGENT */
