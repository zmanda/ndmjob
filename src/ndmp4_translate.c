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
 * Ident:    $Id: ndmp4_translate.c,v 1.1.1.1 2003/10/14 19:12:44 ern Exp $
 *
 * Description:
 *
 */


#include "ndmos.h"		/* rpc/rpc.h */
#include "ndmprotocol.h"
#include "ndmp_translate.h"


#ifndef NDMOS_OPTION_NO_NDMP4


/*
 * ndmp_error
 ****************************************************************
 */

struct enum_conversion ndmp_49_error[] = {
      { NDMP4_UNDEFINED_ERR,		NDMP9_UNDEFINED_ERR }, /* default */
      { NDMP4_NO_ERR,			NDMP9_NO_ERR },
      { NDMP4_NOT_SUPPORTED_ERR,	NDMP9_NOT_SUPPORTED_ERR },
      { NDMP4_DEVICE_BUSY_ERR,		NDMP9_DEVICE_BUSY_ERR },
      { NDMP4_DEVICE_OPENED_ERR,	NDMP9_DEVICE_OPENED_ERR },
      { NDMP4_NOT_AUTHORIZED_ERR,	NDMP9_NOT_AUTHORIZED_ERR },
      { NDMP4_PERMISSION_ERR,		NDMP9_PERMISSION_ERR },
      { NDMP4_DEV_NOT_OPEN_ERR,		NDMP9_DEV_NOT_OPEN_ERR },
      { NDMP4_IO_ERR,			NDMP9_IO_ERR },
      { NDMP4_TIMEOUT_ERR,		NDMP9_TIMEOUT_ERR },
      { NDMP4_ILLEGAL_ARGS_ERR,		NDMP9_ILLEGAL_ARGS_ERR },
      { NDMP4_NO_TAPE_LOADED_ERR,	NDMP9_NO_TAPE_LOADED_ERR },
      { NDMP4_WRITE_PROTECT_ERR,	NDMP9_WRITE_PROTECT_ERR },
      { NDMP4_EOF_ERR,			NDMP9_EOF_ERR },
      { NDMP4_EOM_ERR,			NDMP9_EOM_ERR },
      { NDMP4_FILE_NOT_FOUND_ERR,	NDMP9_FILE_NOT_FOUND_ERR },
      { NDMP4_BAD_FILE_ERR,		NDMP9_BAD_FILE_ERR },
      { NDMP4_NO_DEVICE_ERR,		NDMP9_NO_DEVICE_ERR },
      { NDMP4_NO_BUS_ERR,		NDMP9_NO_BUS_ERR },
      { NDMP4_XDR_DECODE_ERR,		NDMP9_XDR_DECODE_ERR },
      { NDMP4_ILLEGAL_STATE_ERR,	NDMP9_ILLEGAL_STATE_ERR },
      { NDMP4_UNDEFINED_ERR,		NDMP9_UNDEFINED_ERR },
      { NDMP4_XDR_ENCODE_ERR,		NDMP9_XDR_ENCODE_ERR },
      { NDMP4_NO_MEM_ERR,		NDMP9_NO_MEM_ERR },
      { NDMP4_CONNECT_ERR,		NDMP9_CONNECT_ERR },
	END_ENUM_CONVERSION_TABLE
};


extern int
ndmp_4to9_error (
  ndmp4_error *error4,
  ndmp9_error *error9)
{
	*error9 = convert_enum_to_9 (ndmp_49_error, *error4);
	return 0;
}

extern int
ndmp_9to4_error (
  ndmp9_error *error9,
  ndmp4_error *error4)
{
	*error4 = convert_enum_from_9 (ndmp_49_error, *error9);
	return 0;
}




/*
 * ndmp_data_get_state_reply
 ****************************************************************
 */

struct enum_conversion	ndmp_49_data_operation[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP4_DATA_OP_NOACTION,		NDMP9_DATA_OP_NOACTION },
      { NDMP4_DATA_OP_BACKUP,		NDMP9_DATA_OP_BACKUP },
      { NDMP4_DATA_OP_RESTORE,		NDMP9_DATA_OP_RESTORE },
      { NDMP4_DATA_OP_RESTORE_FILEHIST,	NDMP9_DATA_OP_RESTORE_FILEHIST },
	END_ENUM_CONVERSION_TABLE
};

struct enum_conversion	ndmp_49_data_state[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP4_DATA_STATE_IDLE,		NDMP9_DATA_STATE_IDLE },
      { NDMP4_DATA_STATE_ACTIVE,	NDMP9_DATA_STATE_ACTIVE },
      { NDMP4_DATA_STATE_HALTED,	NDMP9_DATA_STATE_HALTED },
      { NDMP4_DATA_STATE_CONNECTED,	NDMP9_DATA_STATE_CONNECTED },
      { NDMP4_DATA_STATE_LISTEN,	NDMP9_DATA_STATE_LISTEN },

	END_ENUM_CONVERSION_TABLE
};

struct enum_conversion	ndmp_49_data_halt_reason[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP4_DATA_HALT_NA,		NDMP9_DATA_HALT_NA },
      { NDMP4_DATA_HALT_SUCCESSFUL,	NDMP9_DATA_HALT_SUCCESSFUL },
      { NDMP4_DATA_HALT_ABORTED,	NDMP9_DATA_HALT_ABORTED },
      { NDMP4_DATA_HALT_INTERNAL_ERROR,	NDMP9_DATA_HALT_INTERNAL_ERROR },
      { NDMP4_DATA_HALT_CONNECT_ERROR,	NDMP9_DATA_HALT_CONNECT_ERROR },
	END_ENUM_CONVERSION_TABLE
};

extern int
ndmp_4to9_data_get_state_reply (
  ndmp4_data_get_state_reply *reply4,
  ndmp9_data_get_state_reply *reply9)
{
	CNVT_E_TO_9 (reply4, reply9, error, ndmp_49_error);
	CNVT_E_TO_9 (reply4, reply9, operation, ndmp_49_data_operation);
	CNVT_E_TO_9 (reply4, reply9, state, ndmp_49_data_state);
	CNVT_E_TO_9 (reply4, reply9, halt_reason, ndmp_49_data_halt_reason);

	CNVT_TO_9 (reply4, reply9, bytes_processed);

	CNVT_VUQ_TO_9 (reply4, reply9, est_bytes_remain);
	CNVT_VUL_TO_9 (reply4, reply9, est_time_remain);

	ndmp_4to9_addr (&reply4->data_connection_addr,
					&reply9->data_connection_addr);

	CNVT_TO_9 (reply4, reply9, read_offset);
	CNVT_TO_9 (reply4, reply9, read_length);

	return 0;
}

extern int
ndmp_9to4_data_get_state_reply (
  ndmp9_data_get_state_reply *reply9,
  ndmp4_data_get_state_reply *reply4)
{
	CNVT_E_FROM_9 (reply4, reply9, error, ndmp_49_error);
	CNVT_E_FROM_9 (reply4, reply9, operation, ndmp_49_data_operation);
	CNVT_E_FROM_9 (reply4, reply9, state, ndmp_49_data_state);
	CNVT_E_FROM_9 (reply4, reply9, halt_reason, ndmp_49_data_halt_reason);

	CNVT_FROM_9 (reply4, reply9, bytes_processed);

	CNVT_VUQ_FROM_9 (reply4, reply9, est_bytes_remain);
	CNVT_VUL_FROM_9 (reply4, reply9, est_time_remain);

	ndmp_9to4_addr (&reply9->data_connection_addr,
					&reply4->data_connection_addr);

	CNVT_FROM_9 (reply4, reply9, read_offset);
	CNVT_FROM_9 (reply4, reply9, read_length);

	return 0;
}




/*
 * ndmp_tape_get_state_reply
 ****************************************************************
 */

extern int
ndmp_4to9_tape_get_state_reply (
  ndmp4_tape_get_state_reply *reply4,
  ndmp9_tape_get_state_reply *reply9)
{
	CNVT_E_TO_9 (reply4, reply9, error, ndmp_49_error);
	CNVT_TO_9 (reply4, reply9, flags);
	CNVT_VUL_TO_9 (reply4, reply9, file_num);
	CNVT_VUL_TO_9 (reply4, reply9, soft_errors);
	CNVT_VUL_TO_9 (reply4, reply9, block_size);
	CNVT_VUL_TO_9 (reply4, reply9, blockno);
	CNVT_VUQ_TO_9 (reply4, reply9, total_space);
	CNVT_VUQ_TO_9 (reply4, reply9, space_remain);
#if 0
	CNVT_VUL_TO_9 (reply4, reply9, partition);
#endif

	if (reply4->unsupported & NDMP4_TAPE_STATE_FILE_NUM_UNS)
		CNVT_IUL_TO_9 (reply9, file_num);

	if (reply4->unsupported & NDMP4_TAPE_STATE_SOFT_ERRORS_UNS)
		CNVT_IUL_TO_9 (reply9, soft_errors);

	if (reply4->unsupported & NDMP4_TAPE_STATE_BLOCK_SIZE_UNS)
		CNVT_IUL_TO_9 (reply9, block_size);

	if (reply4->unsupported & NDMP4_TAPE_STATE_BLOCKNO_UNS)
		CNVT_IUL_TO_9 (reply9, blockno);

	if (reply4->unsupported & NDMP4_TAPE_STATE_TOTAL_SPACE_UNS)
		CNVT_IUQ_TO_9 (reply9, total_space);

	if (reply4->unsupported & NDMP4_TAPE_STATE_SPACE_REMAIN_UNS)
		CNVT_IUQ_TO_9 (reply9, space_remain);

#if 0
	if (reply4->unsupported & NDMP4_TAPE_STATE_PARTITION_UNS)
		CNVT_IUL_TO_9 (reply9, partition);
#endif

	return 0;
}

extern int
ndmp_9to4_tape_get_state_reply (
  ndmp9_tape_get_state_reply *reply9,
  ndmp4_tape_get_state_reply *reply4)
{
	CNVT_E_FROM_9 (reply4, reply9, error, ndmp_49_error);
	CNVT_FROM_9 (reply4, reply9, flags);
	CNVT_VUL_FROM_9 (reply4, reply9, file_num);
	CNVT_VUL_FROM_9 (reply4, reply9, soft_errors);
	CNVT_VUL_FROM_9 (reply4, reply9, block_size);
	CNVT_VUL_FROM_9 (reply4, reply9, blockno);
	CNVT_VUQ_FROM_9 (reply4, reply9, total_space);
	CNVT_VUQ_FROM_9 (reply4, reply9, space_remain);
#if 0
	CNVT_VUL_FROM_9 (reply4, reply9, partition);
#endif

#if 0
	reply4->unsupported = 0;

	if (!reply9->file_num.valid)
		reply4->unsupported |= NDMP4_TAPE_STATE_FILE_NUM_UNS;

	if (!reply9->soft_errors.valid)
		reply4->unsupported |= NDMP4_TAPE_STATE_SOFT_ERRORS_UNS;

	if (!reply9->block_size.valid)
		reply4->unsupported |= NDMP4_TAPE_STATE_BLOCK_SIZE_UNS;

	if (!reply9->blockno.valid)
		reply4->unsupported |= NDMP4_TAPE_STATE_BLOCKNO_UNS;

	if (!reply9->total_space.valid)
		reply4->unsupported |= NDMP4_TAPE_STATE_TOTAL_SPACE_UNS;

	if (!reply9->space_remain.valid)
		reply4->unsupported |= NDMP4_TAPE_STATE_SPACE_REMAIN_UNS;

	if (!reply9->partition.valid)
		reply4->unsupported |= NDMP4_TAPE_STATE_PARTITION_UNS;
 /*#else*/
	reply4->unsupported |= NDMP4_TAPE_STATE_PARTITION_UNS;
#endif

	return 0;
}




/*
 * ndmp_mover_get_state_reply
 ****************************************************************
 */

struct enum_conversion	ndmp_49_mover_state[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP4_MOVER_STATE_IDLE,		NDMP9_MOVER_STATE_IDLE },
      { NDMP4_MOVER_STATE_LISTEN,	NDMP9_MOVER_STATE_LISTEN },
      { NDMP4_MOVER_STATE_ACTIVE,	NDMP9_MOVER_STATE_ACTIVE },
      { NDMP4_MOVER_STATE_PAUSED,	NDMP9_MOVER_STATE_PAUSED },
      { NDMP4_MOVER_STATE_HALTED,	NDMP9_MOVER_STATE_HALTED },

	/* alias */
      { NDMP4_MOVER_STATE_ACTIVE,	NDMP9_MOVER_STATE_STANDBY },

	END_ENUM_CONVERSION_TABLE
};

struct enum_conversion	ndmp_49_mover_pause_reason[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP4_MOVER_PAUSE_NA,		NDMP9_MOVER_PAUSE_NA },
      { NDMP4_MOVER_PAUSE_EOM,		NDMP9_MOVER_PAUSE_EOM },
      { NDMP4_MOVER_PAUSE_EOF,		NDMP9_MOVER_PAUSE_EOF },
      { NDMP4_MOVER_PAUSE_SEEK,		NDMP9_MOVER_PAUSE_SEEK },
      { NDMP4_MOVER_PAUSE_MEDIA_ERROR,	NDMP9_MOVER_PAUSE_MEDIA_ERROR },
      { NDMP4_MOVER_PAUSE_EOW,		NDMP9_MOVER_PAUSE_EOW },
	END_ENUM_CONVERSION_TABLE
};

struct enum_conversion	ndmp_49_mover_halt_reason[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP4_MOVER_HALT_NA,		NDMP9_MOVER_HALT_NA },
      { NDMP4_MOVER_HALT_CONNECT_CLOSED, NDMP9_MOVER_HALT_CONNECT_CLOSED },
      { NDMP4_MOVER_HALT_ABORTED,	NDMP9_MOVER_HALT_ABORTED },
      { NDMP4_MOVER_HALT_INTERNAL_ERROR, NDMP9_MOVER_HALT_INTERNAL_ERROR },
      { NDMP4_MOVER_HALT_CONNECT_ERROR,	NDMP9_MOVER_HALT_CONNECT_ERROR },
	END_ENUM_CONVERSION_TABLE
};


extern int
ndmp_4to9_mover_get_state_reply (
  ndmp4_mover_get_state_reply *reply4,
  ndmp9_mover_get_state_reply *reply9)
{
	CNVT_E_TO_9 (reply4, reply9, error, ndmp_49_error);
	CNVT_E_TO_9 (reply4, reply9, state, ndmp_49_mover_state);
	CNVT_E_TO_9 (reply4, reply9, pause_reason, ndmp_49_mover_pause_reason);
	CNVT_E_TO_9 (reply4, reply9, halt_reason, ndmp_49_mover_halt_reason);

	CNVT_TO_9 (reply4, reply9, record_size);
	CNVT_TO_9 (reply4, reply9, record_num);
	CNVT_TO_9 (reply4, reply9, bytes_moved);
	CNVT_TO_9 (reply4, reply9, seek_position);
	CNVT_TO_9 (reply4, reply9, bytes_left_to_read);
	CNVT_TO_9 (reply4, reply9, window_offset);
	CNVT_TO_9 (reply4, reply9, window_length);

	ndmp_4to9_addr (&reply4->data_connection_addr,
					&reply9->data_connection_addr);

	return 0;
}

extern int
ndmp_9to4_mover_get_state_reply (
  ndmp9_mover_get_state_reply *reply9,
  ndmp4_mover_get_state_reply *reply4)
{
	CNVT_E_FROM_9 (reply4, reply9, error, ndmp_49_error);
	CNVT_E_FROM_9 (reply4, reply9, state, ndmp_49_mover_state);
	CNVT_E_FROM_9 (reply4, reply9, pause_reason,
						ndmp_49_mover_pause_reason);
	CNVT_E_FROM_9 (reply4, reply9, halt_reason,
						ndmp_49_mover_halt_reason);

	CNVT_FROM_9 (reply4, reply9, record_size);
	CNVT_FROM_9 (reply4, reply9, record_num);
	CNVT_FROM_9 (reply4, reply9, bytes_moved);
	CNVT_FROM_9 (reply4, reply9, seek_position);
	CNVT_FROM_9 (reply4, reply9, bytes_left_to_read);
	CNVT_FROM_9 (reply4, reply9, window_offset);
	CNVT_FROM_9 (reply4, reply9, window_length);

	ndmp_9to4_addr (&reply9->data_connection_addr,
					&reply4->data_connection_addr);

	return 0;
}




/*
 * ndmp[_mover]_addr
 ****************************************************************
 */


extern int
ndmp_4to9_addr (
  ndmp4_addr *addr4,
  ndmp9_addr *addr9)
{
	switch (addr4->addr_type) {
	case NDMP4_ADDR_LOCAL:
		addr9->addr_type = NDMP9_ADDR_LOCAL;
		break;

	case NDMP4_ADDR_TCP:
		addr9->addr_type = NDMP9_ADDR_TCP;
		addr9->ndmp9_addr_u.tcp_addr.ip_addr =
			addr4->ndmp4_addr_u.tcp_addr.ip_addr;
		addr9->ndmp9_addr_u.tcp_addr.port =
			addr4->ndmp4_addr_u.tcp_addr.port;
		break;

	default:
		NDMOS_MACRO_ZEROFILL (addr9);
		addr9->addr_type = -1;
		return -1;
	}

	return 0;
}

extern int
ndmp_9to4_addr (
  ndmp9_addr *addr9,
  ndmp4_addr *addr4)
{
	switch (addr9->addr_type) {
	case NDMP9_ADDR_LOCAL:
		addr4->addr_type = NDMP4_ADDR_LOCAL;
		break;

	case NDMP9_ADDR_TCP:
		addr4->addr_type = NDMP4_ADDR_TCP;
		addr4->ndmp4_addr_u.tcp_addr.ip_addr =
			addr9->ndmp9_addr_u.tcp_addr.ip_addr;
		addr4->ndmp4_addr_u.tcp_addr.port =
			addr9->ndmp9_addr_u.tcp_addr.port;
		break;

	default:
		NDMOS_MACRO_ZEROFILL (addr4);
		addr4->addr_type = -1;
		return -1;
	}

	return 0;
}




/*
 * ndmp[_unix]_file_stat
 ****************************************************************
 */

struct enum_conversion	ndmp_49_file_type[] = {
      { NDMP4_FILE_OTHER,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP4_FILE_DIR,			NDMP9_FILE_DIR },
      { NDMP4_FILE_FIFO,		NDMP9_FILE_FIFO },
      { NDMP4_FILE_CSPEC,		NDMP9_FILE_CSPEC },
      { NDMP4_FILE_BSPEC,		NDMP9_FILE_BSPEC },
      { NDMP4_FILE_REG,			NDMP9_FILE_REG },
      { NDMP4_FILE_SLINK,		NDMP9_FILE_SLINK },
      { NDMP4_FILE_SOCK,		NDMP9_FILE_SOCK },
      { NDMP4_FILE_REGISTRY,		NDMP9_FILE_REGISTRY },
      { NDMP4_FILE_OTHER,		NDMP9_FILE_OTHER },
	END_ENUM_CONVERSION_TABLE
};

extern int
ndmp_4to9_file_stat (
  ndmp4_file_stat *fstat4,
  ndmp9_file_stat *fstat9,
  ndmp9_u_quad node,
  ndmp9_u_quad fh_info)
{
	CNVT_E_TO_9 (fstat4, fstat9, ftype, ndmp_49_file_type);

	CNVT_VUL_TO_9 (fstat4, fstat9, mtime);
	CNVT_VUL_TO_9 (fstat4, fstat9, atime);
	CNVT_VUL_TO_9 (fstat4, fstat9, ctime);
	CNVT_VUL_TO_9x (fstat4, fstat9, owner, uid);
	CNVT_VUL_TO_9x (fstat4, fstat9, group, gid);

	CNVT_VUL_TO_9x (fstat4, fstat9, fattr, mode);

	CNVT_VUQ_TO_9 (fstat4, fstat9, size);

	CNVT_VUL_TO_9 (fstat4, fstat9, links);

	convert_valid_u_quad_to_9 (&node, &fstat9->node);
	convert_valid_u_quad_to_9 (&fh_info, &fstat9->fh_info);

	if (fstat4->unsupported & NDMP4_FILE_STAT_ATIME_UNS)
		CNVT_IUL_TO_9 (fstat9, atime);

	if (fstat4->unsupported & NDMP4_FILE_STAT_CTIME_UNS)
		CNVT_IUL_TO_9 (fstat9, ctime);

	if (fstat4->unsupported & NDMP4_FILE_STAT_GROUP_UNS)
		CNVT_IUL_TO_9 (fstat9, gid);

	return 0;
}

extern int
ndmp_9to4_file_stat (
  ndmp9_file_stat *fstat9,
  ndmp4_file_stat *fstat4)
{
	CNVT_E_FROM_9 (fstat4, fstat9, ftype, ndmp_49_file_type);

	fstat4->fs_type = NDMP4_FS_UNIX;

	CNVT_VUL_FROM_9 (fstat4, fstat9, mtime);
	CNVT_VUL_FROM_9 (fstat4, fstat9, atime);
	CNVT_VUL_FROM_9 (fstat4, fstat9, ctime);
	CNVT_VUL_FROM_9x (fstat4, fstat9, owner, uid);
	CNVT_VUL_FROM_9x (fstat4, fstat9, group, gid);

	CNVT_VUL_FROM_9x (fstat4, fstat9, fattr, mode);

	CNVT_VUQ_FROM_9 (fstat4, fstat9, size);

	CNVT_VUL_FROM_9 (fstat4, fstat9, links);

	fstat4->unsupported = 0;

	if (!fstat9->atime.valid)
		fstat4->unsupported |= NDMP4_FILE_STAT_ATIME_UNS;

	if (!fstat9->ctime.valid)
		fstat4->unsupported |= NDMP4_FILE_STAT_CTIME_UNS;

	if (!fstat9->gid.valid)
		fstat4->unsupported |= NDMP4_FILE_STAT_GROUP_UNS;

	/* fh_info ignored */
	/* node ignored */

	return 0;
}




/*
 * ndmp_pval
 ****************************************************************
 */

extern int
ndmp_4to9_pval (
  ndmp4_pval *pval4,
  ndmp9_pval *pval9)
{
	CNVT_TO_9 (pval4, pval9, name);
	CNVT_TO_9 (pval4, pval9, value);

	return 0;
}

extern int
ndmp_9to4_pval (
  ndmp9_pval *pval9,
  ndmp4_pval *pval4)
{
	CNVT_FROM_9 (pval4, pval9, name);
	CNVT_FROM_9 (pval4, pval9, value);

	return 0;
}

extern int
ndmp_4to9_pval_vec (
  ndmp4_pval *pval4,
  ndmp9_pval *pval9,
  unsigned n_pval)
{
	int		i;

	for (i = 0; i < n_pval; i++)
		ndmp_4to9_pval (&pval4[i], &pval9[i]);

	return 0;
}

extern int
ndmp_9to4_pval_vec (
  ndmp9_pval *pval9,
  ndmp4_pval *pval4,
  unsigned n_pval)
{
	int		i;

	for (i = 0; i < n_pval; i++)
		ndmp_9to4_pval (&pval9[i], &pval4[i]);

	return 0;
}




/*
 * ndmp_name
 ****************************************************************
 */

extern int
ndmp_4to9_name (
  ndmp4_name *name4,
  ndmp9_name *name9)
{
	return 0;
}

extern int
ndmp_9to4_name (
  ndmp9_name *name9,
  ndmp4_name *name4)
{
	return 0;
}

extern int
ndmp_4to9_name_vec (
  ndmp4_name *name4,
  ndmp9_name *name9,
  unsigned n_name)
{
	int		i;

	for (i = 0; i < n_name; i++)
		ndmp_4to9_name (&name4[i], &name9[i]);

	return 0;
}

extern int
ndmp_9to4_name_vec (
  ndmp9_name *name9,
  ndmp4_name *name4,
  unsigned n_name)
{
	int		i;

	for (i = 0; i < n_name; i++)
		ndmp_9to4_name (&name9[i], &name4[i]);

	return 0;
}

#endif /* !NDMOS_OPTION_NO_NDMP4 */
