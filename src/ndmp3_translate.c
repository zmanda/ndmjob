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
 * Ident:    $Id: ndmp3_translate.c,v 1.1.1.1 2003/10/14 19:12:43 ern Exp $
 *
 * Description:
 *
 */


#include "ndmos.h"		/* rpc/rpc.h */
#include "ndmprotocol.h"
#include "ndmp_translate.h"


#ifndef NDMOS_OPTION_NO_NDMP3


/*
 * ndmp_error
 ****************************************************************
 */

struct enum_conversion ndmp_39_error[] = {
      { NDMP3_UNDEFINED_ERR,		NDMP9_UNDEFINED_ERR }, /* default */
      { NDMP3_NO_ERR,			NDMP9_NO_ERR },
      { NDMP3_NOT_SUPPORTED_ERR,	NDMP9_NOT_SUPPORTED_ERR },
      { NDMP3_DEVICE_BUSY_ERR,		NDMP9_DEVICE_BUSY_ERR },
      { NDMP3_DEVICE_OPENED_ERR,	NDMP9_DEVICE_OPENED_ERR },
      { NDMP3_NOT_AUTHORIZED_ERR,	NDMP9_NOT_AUTHORIZED_ERR },
      { NDMP3_PERMISSION_ERR,		NDMP9_PERMISSION_ERR },
      { NDMP3_DEV_NOT_OPEN_ERR,		NDMP9_DEV_NOT_OPEN_ERR },
      { NDMP3_IO_ERR,			NDMP9_IO_ERR },
      { NDMP3_TIMEOUT_ERR,		NDMP9_TIMEOUT_ERR },
      { NDMP3_ILLEGAL_ARGS_ERR,		NDMP9_ILLEGAL_ARGS_ERR },
      { NDMP3_NO_TAPE_LOADED_ERR,	NDMP9_NO_TAPE_LOADED_ERR },
      { NDMP3_WRITE_PROTECT_ERR,	NDMP9_WRITE_PROTECT_ERR },
      { NDMP3_EOF_ERR,			NDMP9_EOF_ERR },
      { NDMP3_EOM_ERR,			NDMP9_EOM_ERR },
      { NDMP3_FILE_NOT_FOUND_ERR,	NDMP9_FILE_NOT_FOUND_ERR },
      { NDMP3_BAD_FILE_ERR,		NDMP9_BAD_FILE_ERR },
      { NDMP3_NO_DEVICE_ERR,		NDMP9_NO_DEVICE_ERR },
      { NDMP3_NO_BUS_ERR,		NDMP9_NO_BUS_ERR },
      { NDMP3_XDR_DECODE_ERR,		NDMP9_XDR_DECODE_ERR },
      { NDMP3_ILLEGAL_STATE_ERR,	NDMP9_ILLEGAL_STATE_ERR },
      { NDMP3_UNDEFINED_ERR,		NDMP9_UNDEFINED_ERR },
      { NDMP3_XDR_ENCODE_ERR,		NDMP9_XDR_ENCODE_ERR },
      { NDMP3_NO_MEM_ERR,		NDMP9_NO_MEM_ERR },
      { NDMP3_CONNECT_ERR,		NDMP9_CONNECT_ERR },
	END_ENUM_CONVERSION_TABLE
};


extern int
ndmp_3to9_error (
  ndmp3_error *error3,
  ndmp9_error *error9)
{
	*error9 = convert_enum_to_9 (ndmp_39_error, *error3);
	return 0;
}

extern int
ndmp_9to3_error (
  ndmp9_error *error9,
  ndmp3_error *error3)
{
	*error3 = convert_enum_from_9 (ndmp_39_error, *error9);
	return 0;
}




/*
 * ndmp_data_get_state_reply
 ****************************************************************
 */

struct enum_conversion	ndmp_39_data_operation[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP3_DATA_OP_NOACTION,		NDMP9_DATA_OP_NOACTION },
      { NDMP3_DATA_OP_BACKUP,		NDMP9_DATA_OP_BACKUP },
      { NDMP3_DATA_OP_RESTORE,		NDMP9_DATA_OP_RESTORE },
      { NDMP3_DATA_OP_RESTORE_FILEHIST,	NDMP9_DATA_OP_RESTORE_FILEHIST },
	END_ENUM_CONVERSION_TABLE
};

struct enum_conversion	ndmp_39_data_state[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP3_DATA_STATE_IDLE,		NDMP9_DATA_STATE_IDLE },
      { NDMP3_DATA_STATE_ACTIVE,	NDMP9_DATA_STATE_ACTIVE },
      { NDMP3_DATA_STATE_HALTED,	NDMP9_DATA_STATE_HALTED },
      { NDMP3_DATA_STATE_CONNECTED,	NDMP9_DATA_STATE_CONNECTED },
      { NDMP3_DATA_STATE_LISTEN,	NDMP9_DATA_STATE_LISTEN },

	END_ENUM_CONVERSION_TABLE
};

struct enum_conversion	ndmp_39_data_halt_reason[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP3_DATA_HALT_NA,		NDMP9_DATA_HALT_NA },
      { NDMP3_DATA_HALT_SUCCESSFUL,	NDMP9_DATA_HALT_SUCCESSFUL },
      { NDMP3_DATA_HALT_ABORTED,	NDMP9_DATA_HALT_ABORTED },
      { NDMP3_DATA_HALT_INTERNAL_ERROR,	NDMP9_DATA_HALT_INTERNAL_ERROR },
      { NDMP3_DATA_HALT_CONNECT_ERROR,	NDMP9_DATA_HALT_CONNECT_ERROR },
	END_ENUM_CONVERSION_TABLE
};

extern int
ndmp_3to9_data_get_state_reply (
  ndmp3_data_get_state_reply *reply3,
  ndmp9_data_get_state_reply *reply9)
{
	CNVT_E_TO_9 (reply3, reply9, error, ndmp_39_error);
	CNVT_E_TO_9 (reply3, reply9, operation, ndmp_39_data_operation);
	CNVT_E_TO_9 (reply3, reply9, state, ndmp_39_data_state);
	CNVT_E_TO_9 (reply3, reply9, halt_reason, ndmp_39_data_halt_reason);

	CNVT_TO_9 (reply3, reply9, bytes_processed);

	CNVT_VUQ_TO_9 (reply3, reply9, est_bytes_remain);
	CNVT_VUL_TO_9 (reply3, reply9, est_time_remain);

	ndmp_3to9_addr (&reply3->data_connection_addr,
					&reply9->data_connection_addr);

	CNVT_TO_9 (reply3, reply9, read_offset);
	CNVT_TO_9 (reply3, reply9, read_length);

	return 0;
}

extern int
ndmp_9to3_data_get_state_reply (
  ndmp9_data_get_state_reply *reply9,
  ndmp3_data_get_state_reply *reply3)
{
	CNVT_E_FROM_9 (reply3, reply9, error, ndmp_39_error);
	CNVT_E_FROM_9 (reply3, reply9, operation, ndmp_39_data_operation);
	CNVT_E_FROM_9 (reply3, reply9, state, ndmp_39_data_state);
	CNVT_E_FROM_9 (reply3, reply9, halt_reason, ndmp_39_data_halt_reason);

	CNVT_FROM_9 (reply3, reply9, bytes_processed);

	CNVT_VUQ_FROM_9 (reply3, reply9, est_bytes_remain);
	CNVT_VUL_FROM_9 (reply3, reply9, est_time_remain);

	ndmp_9to3_addr (&reply9->data_connection_addr,
					&reply3->data_connection_addr);

	CNVT_FROM_9 (reply3, reply9, read_offset);
	CNVT_FROM_9 (reply3, reply9, read_length);

	return 0;
}




/*
 * ndmp_tape_get_state_reply
 ****************************************************************
 */

extern int
ndmp_3to9_tape_get_state_reply (
  ndmp3_tape_get_state_reply *reply3,
  ndmp9_tape_get_state_reply *reply9)
{
	CNVT_E_TO_9 (reply3, reply9, error, ndmp_39_error);
	CNVT_TO_9 (reply3, reply9, flags);
	CNVT_VUL_TO_9 (reply3, reply9, file_num);
	CNVT_VUL_TO_9 (reply3, reply9, soft_errors);
	CNVT_VUL_TO_9 (reply3, reply9, block_size);
	CNVT_VUL_TO_9 (reply3, reply9, blockno);
	CNVT_VUQ_TO_9 (reply3, reply9, total_space);
	CNVT_VUQ_TO_9 (reply3, reply9, space_remain);
#if 0
	CNVT_VUL_TO_9 (reply3, reply9, partition);
#endif

	if (reply3->invalid & NDMP3_TAPE_STATE_FILE_NUM_INVALID)
		CNVT_IUL_TO_9 (reply9, file_num);

	if (reply3->invalid & NDMP3_TAPE_STATE_SOFT_ERRORS_INVALID)
		CNVT_IUL_TO_9 (reply9, soft_errors);

	if (reply3->invalid & NDMP3_TAPE_STATE_BLOCK_SIZE_INVALID)
		CNVT_IUL_TO_9 (reply9, block_size);

	if (reply3->invalid & NDMP3_TAPE_STATE_BLOCKNO_INVALID)
		CNVT_IUL_TO_9 (reply9, blockno);

	if (reply3->invalid & NDMP3_TAPE_STATE_TOTAL_SPACE_INVALID)
		CNVT_IUQ_TO_9 (reply9, total_space);

	if (reply3->invalid & NDMP3_TAPE_STATE_SPACE_REMAIN_INVALID)
		CNVT_IUQ_TO_9 (reply9, space_remain);

#if 0
	if (reply3->invalid & NDMP3_TAPE_STATE_PARTITION_INVALID)
		CNVT_IUL_TO_9 (reply9, partition);
#endif

	return 0;
}

extern int
ndmp_9to3_tape_get_state_reply (
  ndmp9_tape_get_state_reply *reply9,
  ndmp3_tape_get_state_reply *reply3)
{
	CNVT_E_FROM_9 (reply3, reply9, error, ndmp_39_error);
	CNVT_FROM_9 (reply3, reply9, flags);
	CNVT_VUL_FROM_9 (reply3, reply9, file_num);
	CNVT_VUL_FROM_9 (reply3, reply9, soft_errors);
	CNVT_VUL_FROM_9 (reply3, reply9, block_size);
	CNVT_VUL_FROM_9 (reply3, reply9, blockno);
	CNVT_VUQ_FROM_9 (reply3, reply9, total_space);
	CNVT_VUQ_FROM_9 (reply3, reply9, space_remain);
#if 0
	CNVT_VUL_FROM_9 (reply3, reply9, partition);
#endif

	reply3->invalid = 0;

	if (!reply9->file_num.valid)
		reply3->invalid |= NDMP3_TAPE_STATE_FILE_NUM_INVALID;

	if (!reply9->soft_errors.valid)
		reply3->invalid |= NDMP3_TAPE_STATE_SOFT_ERRORS_INVALID;

	if (!reply9->block_size.valid)
		reply3->invalid |= NDMP3_TAPE_STATE_BLOCK_SIZE_INVALID;

	if (!reply9->blockno.valid)
		reply3->invalid |= NDMP3_TAPE_STATE_BLOCKNO_INVALID;

	if (!reply9->total_space.valid)
		reply3->invalid |= NDMP3_TAPE_STATE_TOTAL_SPACE_INVALID;

	if (!reply9->space_remain.valid)
		reply3->invalid |= NDMP3_TAPE_STATE_SPACE_REMAIN_INVALID;

#if 0
	if (!reply9->partition.valid)
		reply3->invalid |= NDMP3_TAPE_STATE_PARTITION_INVALID;
#else
	reply3->invalid |= NDMP3_TAPE_STATE_PARTITION_INVALID;
#endif

	return 0;
}




/*
 * ndmp_mover_get_state_reply
 ****************************************************************
 */

struct enum_conversion	ndmp_39_mover_state[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP3_MOVER_STATE_IDLE,		NDMP9_MOVER_STATE_IDLE },
      { NDMP3_MOVER_STATE_LISTEN,	NDMP9_MOVER_STATE_LISTEN },
      { NDMP3_MOVER_STATE_ACTIVE,	NDMP9_MOVER_STATE_ACTIVE },
      { NDMP3_MOVER_STATE_PAUSED,	NDMP9_MOVER_STATE_PAUSED },
      { NDMP3_MOVER_STATE_HALTED,	NDMP9_MOVER_STATE_HALTED },

	/* alias */
      { NDMP3_MOVER_STATE_ACTIVE,	NDMP9_MOVER_STATE_STANDBY },

	END_ENUM_CONVERSION_TABLE
};

struct enum_conversion	ndmp_39_mover_pause_reason[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP3_MOVER_PAUSE_NA,		NDMP9_MOVER_PAUSE_NA },
      { NDMP3_MOVER_PAUSE_EOM,		NDMP9_MOVER_PAUSE_EOM },
      { NDMP3_MOVER_PAUSE_EOF,		NDMP9_MOVER_PAUSE_EOF },
      { NDMP3_MOVER_PAUSE_SEEK,		NDMP9_MOVER_PAUSE_SEEK },
      { NDMP3_MOVER_PAUSE_MEDIA_ERROR,	NDMP9_MOVER_PAUSE_MEDIA_ERROR },
      { NDMP3_MOVER_PAUSE_EOW,		NDMP9_MOVER_PAUSE_EOW },
	END_ENUM_CONVERSION_TABLE
};

struct enum_conversion	ndmp_39_mover_halt_reason[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP3_MOVER_HALT_NA,		NDMP9_MOVER_HALT_NA },
      { NDMP3_MOVER_HALT_CONNECT_CLOSED, NDMP9_MOVER_HALT_CONNECT_CLOSED },
      { NDMP3_MOVER_HALT_ABORTED,	NDMP9_MOVER_HALT_ABORTED },
      { NDMP3_MOVER_HALT_INTERNAL_ERROR, NDMP9_MOVER_HALT_INTERNAL_ERROR },
      { NDMP3_MOVER_HALT_CONNECT_ERROR,	NDMP9_MOVER_HALT_CONNECT_ERROR },
	END_ENUM_CONVERSION_TABLE
};


extern int
ndmp_3to9_mover_get_state_reply (
  ndmp3_mover_get_state_reply *reply3,
  ndmp9_mover_get_state_reply *reply9)
{
	CNVT_E_TO_9 (reply3, reply9, error, ndmp_39_error);
	CNVT_E_TO_9 (reply3, reply9, state, ndmp_39_mover_state);
	CNVT_E_TO_9 (reply3, reply9, pause_reason, ndmp_39_mover_pause_reason);
	CNVT_E_TO_9 (reply3, reply9, halt_reason, ndmp_39_mover_halt_reason);

	CNVT_TO_9 (reply3, reply9, record_size);
	CNVT_TO_9 (reply3, reply9, record_num);
	CNVT_TO_9x (reply3, reply9, data_written, bytes_moved);
	CNVT_TO_9 (reply3, reply9, seek_position);
	CNVT_TO_9 (reply3, reply9, bytes_left_to_read);
	CNVT_TO_9 (reply3, reply9, window_offset);
	CNVT_TO_9 (reply3, reply9, window_length);

	ndmp_3to9_addr (&reply3->data_connection_addr,
					&reply9->data_connection_addr);

	return 0;
}

extern int
ndmp_9to3_mover_get_state_reply (
  ndmp9_mover_get_state_reply *reply9,
  ndmp3_mover_get_state_reply *reply3)
{
	CNVT_E_FROM_9 (reply3, reply9, error, ndmp_39_error);
	CNVT_E_FROM_9 (reply3, reply9, state, ndmp_39_mover_state);
	CNVT_E_FROM_9 (reply3, reply9, pause_reason,
						ndmp_39_mover_pause_reason);
	CNVT_E_FROM_9 (reply3, reply9, halt_reason,
						ndmp_39_mover_halt_reason);

	CNVT_FROM_9 (reply3, reply9, record_size);
	CNVT_FROM_9 (reply3, reply9, record_num);
	CNVT_FROM_9x (reply3, reply9, data_written, bytes_moved);
	CNVT_FROM_9 (reply3, reply9, seek_position);
	CNVT_FROM_9 (reply3, reply9, bytes_left_to_read);
	CNVT_FROM_9 (reply3, reply9, window_offset);
	CNVT_FROM_9 (reply3, reply9, window_length);

	ndmp_9to3_addr (&reply9->data_connection_addr,
					&reply3->data_connection_addr);

	return 0;
}




/*
 * ndmp[_mover]_addr
 ****************************************************************
 */


extern int
ndmp_3to9_addr (
  ndmp3_addr *addr3,
  ndmp9_addr *addr9)
{
	switch (addr3->addr_type) {
	case NDMP3_ADDR_LOCAL:
		addr9->addr_type = NDMP9_ADDR_LOCAL;
		break;

	case NDMP3_ADDR_TCP:
		addr9->addr_type = NDMP9_ADDR_TCP;
		addr9->ndmp9_addr_u.tcp_addr.ip_addr =
			addr3->ndmp3_addr_u.tcp_addr.ip_addr;
		addr9->ndmp9_addr_u.tcp_addr.port =
			addr3->ndmp3_addr_u.tcp_addr.port;
		break;

	default:
		NDMOS_MACRO_ZEROFILL (addr9);
		addr9->addr_type = -1;
		return -1;
	}

	return 0;
}

extern int
ndmp_9to3_addr (
  ndmp9_addr *addr9,
  ndmp3_addr *addr3)
{
	switch (addr9->addr_type) {
	case NDMP9_ADDR_LOCAL:
		addr3->addr_type = NDMP3_ADDR_LOCAL;
		break;

	case NDMP9_ADDR_TCP:
		addr3->addr_type = NDMP3_ADDR_TCP;
		addr3->ndmp3_addr_u.tcp_addr.ip_addr =
			addr9->ndmp9_addr_u.tcp_addr.ip_addr;
		addr3->ndmp3_addr_u.tcp_addr.port =
			addr9->ndmp9_addr_u.tcp_addr.port;
		break;

	default:
		NDMOS_MACRO_ZEROFILL (addr3);
		addr3->addr_type = -1;
		return -1;
	}

	return 0;
}




/*
 * ndmp[_unix]_file_stat
 ****************************************************************
 */

struct enum_conversion	ndmp_39_file_type[] = {
      { NDMP3_FILE_OTHER,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP3_FILE_DIR,			NDMP9_FILE_DIR },
      { NDMP3_FILE_FIFO,		NDMP9_FILE_FIFO },
      { NDMP3_FILE_CSPEC,		NDMP9_FILE_CSPEC },
      { NDMP3_FILE_BSPEC,		NDMP9_FILE_BSPEC },
      { NDMP3_FILE_REG,			NDMP9_FILE_REG },
      { NDMP3_FILE_SLINK,		NDMP9_FILE_SLINK },
      { NDMP3_FILE_SOCK,		NDMP9_FILE_SOCK },
      { NDMP3_FILE_REGISTRY,		NDMP9_FILE_REGISTRY },
      { NDMP3_FILE_OTHER,		NDMP9_FILE_OTHER },
	END_ENUM_CONVERSION_TABLE
};

extern int
ndmp_3to9_file_stat (
  ndmp3_file_stat *fstat3,
  ndmp9_file_stat *fstat9,
  ndmp9_u_quad node,
  ndmp9_u_quad fh_info)
{
	CNVT_E_TO_9 (fstat3, fstat9, ftype, ndmp_39_file_type);

	CNVT_VUL_TO_9 (fstat3, fstat9, mtime);
	CNVT_VUL_TO_9 (fstat3, fstat9, atime);
	CNVT_VUL_TO_9 (fstat3, fstat9, ctime);
	CNVT_VUL_TO_9x (fstat3, fstat9, owner, uid);
	CNVT_VUL_TO_9x (fstat3, fstat9, group, gid);

	CNVT_VUL_TO_9x (fstat3, fstat9, fattr, mode);

	CNVT_VUQ_TO_9 (fstat3, fstat9, size);

	CNVT_VUL_TO_9 (fstat3, fstat9, links);

	convert_valid_u_quad_to_9 (&node, &fstat9->node);
	convert_valid_u_quad_to_9 (&fh_info, &fstat9->fh_info);

	if (fstat3->invalid & NDMP3_FILE_STAT_ATIME_INVALID)
		CNVT_IUL_TO_9 (fstat9, atime);

	if (fstat3->invalid & NDMP3_FILE_STAT_CTIME_INVALID)
		CNVT_IUL_TO_9 (fstat9, ctime);

	if (fstat3->invalid & NDMP3_FILE_STAT_GROUP_INVALID)
		CNVT_IUL_TO_9 (fstat9, gid);

	return 0;
}

extern int
ndmp_9to3_file_stat (
  ndmp9_file_stat *fstat9,
  ndmp3_file_stat *fstat3)
{
	CNVT_E_FROM_9 (fstat3, fstat9, ftype, ndmp_39_file_type);

	fstat3->fs_type = NDMP3_FS_UNIX;

	CNVT_VUL_FROM_9 (fstat3, fstat9, mtime);
	CNVT_VUL_FROM_9 (fstat3, fstat9, atime);
	CNVT_VUL_FROM_9 (fstat3, fstat9, ctime);
	CNVT_VUL_FROM_9x (fstat3, fstat9, owner, uid);
	CNVT_VUL_FROM_9x (fstat3, fstat9, group, gid);

	CNVT_VUL_FROM_9x (fstat3, fstat9, fattr, mode);

	CNVT_VUQ_FROM_9 (fstat3, fstat9, size);

	CNVT_VUL_FROM_9 (fstat3, fstat9, links);

	fstat3->invalid = 0;

	if (!fstat9->atime.valid)
		fstat3->invalid |= NDMP3_FILE_STAT_ATIME_INVALID;

	if (!fstat9->ctime.valid)
		fstat3->invalid |= NDMP3_FILE_STAT_CTIME_INVALID;

	if (!fstat9->gid.valid)
		fstat3->invalid |= NDMP3_FILE_STAT_GROUP_INVALID;

	/* fh_info ignored */
	/* node ignored */

	return 0;
}




/*
 * ndmp_pval
 ****************************************************************
 */

extern int
ndmp_3to9_pval (
  ndmp3_pval *pval3,
  ndmp9_pval *pval9)
{
	CNVT_TO_9 (pval3, pval9, name);
	CNVT_TO_9 (pval3, pval9, value);

	return 0;
}

extern int
ndmp_9to3_pval (
  ndmp9_pval *pval9,
  ndmp3_pval *pval3)
{
	CNVT_FROM_9 (pval3, pval9, name);
	CNVT_FROM_9 (pval3, pval9, value);

	return 0;
}

extern int
ndmp_3to9_pval_vec (
  ndmp3_pval *pval3,
  ndmp9_pval *pval9,
  unsigned n_pval)
{
	int		i;

	for (i = 0; i < n_pval; i++)
		ndmp_3to9_pval (&pval3[i], &pval9[i]);

	return 0;
}

extern int
ndmp_9to3_pval_vec (
  ndmp9_pval *pval9,
  ndmp3_pval *pval3,
  unsigned n_pval)
{
	int		i;

	for (i = 0; i < n_pval; i++)
		ndmp_9to3_pval (&pval9[i], &pval3[i]);

	return 0;
}




/*
 * ndmp_name
 ****************************************************************
 */

extern int
ndmp_3to9_name (
  ndmp3_name *name3,
  ndmp9_name *name9)
{
	return 0;
}

extern int
ndmp_9to3_name (
  ndmp9_name *name9,
  ndmp3_name *name3)
{
	return 0;
}

extern int
ndmp_3to9_name_vec (
  ndmp3_name *name3,
  ndmp9_name *name9,
  unsigned n_name)
{
	int		i;

	for (i = 0; i < n_name; i++)
		ndmp_3to9_name (&name3[i], &name9[i]);

	return 0;
}

extern int
ndmp_9to3_name_vec (
  ndmp9_name *name9,
  ndmp3_name *name3,
  unsigned n_name)
{
	int		i;

	for (i = 0; i < n_name; i++)
		ndmp_9to3_name (&name9[i], &name3[i]);

	return 0;
}

#endif /* !NDMOS_OPTION_NO_NDMP3 */
