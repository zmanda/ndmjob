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
 * Ident:    $Id: ndmp2_translate.c,v 1.1.1.1 2003/10/14 19:12:42 ern Exp $
 *
 * Description:
 *
 */


#include "ndmos.h"		/* rpc/rpc.h */
#include "ndmprotocol.h"
#include "ndmp_translate.h"


#ifndef NDMOS_OPTION_NO_NDMP2


/*
 * ndmp_error
 ****************************************************************
 */


struct enum_conversion ndmp_29_error[] = {
      { NDMP2_UNDEFINED_ERR,		NDMP9_UNDEFINED_ERR }, /* default */
      { NDMP2_NO_ERR,			NDMP9_NO_ERR },
      { NDMP2_NOT_SUPPORTED_ERR,	NDMP9_NOT_SUPPORTED_ERR },
      { NDMP2_DEVICE_BUSY_ERR,		NDMP9_DEVICE_BUSY_ERR },
      { NDMP2_DEVICE_OPENED_ERR,	NDMP9_DEVICE_OPENED_ERR },
      { NDMP2_NOT_AUTHORIZED_ERR,	NDMP9_NOT_AUTHORIZED_ERR },
      { NDMP2_PERMISSION_ERR,		NDMP9_PERMISSION_ERR },
      { NDMP2_DEV_NOT_OPEN_ERR,		NDMP9_DEV_NOT_OPEN_ERR },
      { NDMP2_IO_ERR,			NDMP9_IO_ERR },
      { NDMP2_TIMEOUT_ERR,		NDMP9_TIMEOUT_ERR },
      { NDMP2_ILLEGAL_ARGS_ERR,		NDMP9_ILLEGAL_ARGS_ERR },
      { NDMP2_NO_TAPE_LOADED_ERR,	NDMP9_NO_TAPE_LOADED_ERR },
      { NDMP2_WRITE_PROTECT_ERR,	NDMP9_WRITE_PROTECT_ERR },
      { NDMP2_EOF_ERR,			NDMP9_EOF_ERR },
      { NDMP2_EOM_ERR,			NDMP9_EOM_ERR },
      { NDMP2_FILE_NOT_FOUND_ERR,	NDMP9_FILE_NOT_FOUND_ERR },
      { NDMP2_BAD_FILE_ERR,		NDMP9_BAD_FILE_ERR },
      { NDMP2_NO_DEVICE_ERR,		NDMP9_NO_DEVICE_ERR },
      { NDMP2_NO_BUS_ERR,		NDMP9_NO_BUS_ERR },
      { NDMP2_XDR_DECODE_ERR,		NDMP9_XDR_DECODE_ERR },
      { NDMP2_ILLEGAL_STATE_ERR,	NDMP9_ILLEGAL_STATE_ERR },
      { NDMP2_UNDEFINED_ERR,		NDMP9_UNDEFINED_ERR },
      { NDMP2_XDR_ENCODE_ERR,		NDMP9_XDR_ENCODE_ERR },
      { NDMP2_NO_MEM_ERR,		NDMP9_NO_MEM_ERR },
	END_ENUM_CONVERSION_TABLE
};


extern int
ndmp_2to9_error (
  ndmp2_error *error2,
  ndmp9_error *error9)
{
	*error9 = convert_enum_to_9 (ndmp_29_error, *error2);
	return 0;
}

extern int
ndmp_9to2_error (
  ndmp9_error *error9,
  ndmp2_error *error2)
{
	*error2 = convert_enum_from_9 (ndmp_29_error, *error9);
	return 0;
}




/*
 * ndmp_data_get_state_reply
 ****************************************************************
 */

struct enum_conversion	ndmp_29_data_operation[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP2_DATA_OP_NOACTION,		NDMP9_DATA_OP_NOACTION },
      { NDMP2_DATA_OP_BACKUP,		NDMP9_DATA_OP_BACKUP },
      { NDMP2_DATA_OP_RESTORE,		NDMP9_DATA_OP_RESTORE },
      { NDMP2_DATA_OP_RESTORE_FILEHIST,	NDMP9_DATA_OP_RESTORE_FILEHIST },
	END_ENUM_CONVERSION_TABLE
};

struct enum_conversion	ndmp_29_data_state[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP2_DATA_STATE_IDLE,		NDMP9_DATA_STATE_IDLE },
      { NDMP2_DATA_STATE_ACTIVE,	NDMP9_DATA_STATE_ACTIVE },
      { NDMP2_DATA_STATE_HALTED,	NDMP9_DATA_STATE_HALTED },

	/* aliases */
      { NDMP2_DATA_STATE_ACTIVE,	NDMP9_DATA_STATE_CONNECTED },
      { NDMP2_DATA_STATE_ACTIVE,	NDMP9_DATA_STATE_LISTEN },

	END_ENUM_CONVERSION_TABLE
};

struct enum_conversion	ndmp_29_data_halt_reason[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP2_DATA_HALT_NA,		NDMP9_DATA_HALT_NA },
      { NDMP2_DATA_HALT_SUCCESSFUL,	NDMP9_DATA_HALT_SUCCESSFUL },
      { NDMP2_DATA_HALT_ABORTED,	NDMP9_DATA_HALT_ABORTED },
      { NDMP2_DATA_HALT_INTERNAL_ERROR,	NDMP9_DATA_HALT_INTERNAL_ERROR },
      { NDMP2_DATA_HALT_CONNECT_ERROR,	NDMP9_DATA_HALT_CONNECT_ERROR },
	END_ENUM_CONVERSION_TABLE
};


extern int
ndmp_2to9_data_get_state_reply (
  ndmp2_data_get_state_reply *reply2,
  ndmp9_data_get_state_reply *reply9)
{
	CNVT_E_TO_9 (reply2, reply9, error, ndmp_29_error);
	CNVT_E_TO_9 (reply2, reply9, operation, ndmp_29_data_operation);
	CNVT_E_TO_9 (reply2, reply9, state, ndmp_29_data_state);
	CNVT_E_TO_9 (reply2, reply9, halt_reason, ndmp_29_data_halt_reason);

	CNVT_TO_9 (reply2, reply9, bytes_processed);

	CNVT_VUQ_TO_9 (reply2, reply9, est_bytes_remain);
	CNVT_VUL_TO_9 (reply2, reply9, est_time_remain);

	ndmp_2to9_mover_addr (&reply2->mover, &reply9->data_connection_addr);

	CNVT_TO_9 (reply2, reply9, read_offset);
	CNVT_TO_9 (reply2, reply9, read_length);

	return 0;
}

extern int
ndmp_9to2_data_get_state_reply (
  ndmp9_data_get_state_reply *reply9,
  ndmp2_data_get_state_reply *reply2)
{
	CNVT_E_FROM_9 (reply2, reply9, error, ndmp_29_error);
	CNVT_E_FROM_9 (reply2, reply9, operation, ndmp_29_data_operation);
	CNVT_E_FROM_9 (reply2, reply9, state, ndmp_29_data_state);
	CNVT_E_FROM_9 (reply2, reply9, halt_reason, ndmp_29_data_halt_reason);

	CNVT_FROM_9 (reply2, reply9, bytes_processed);

	CNVT_VUQ_FROM_9 (reply2, reply9, est_bytes_remain);
	CNVT_VUL_FROM_9 (reply2, reply9, est_time_remain);

	ndmp_9to2_mover_addr (&reply9->data_connection_addr, &reply2->mover);

	CNVT_FROM_9 (reply2, reply9, read_offset);
	CNVT_FROM_9 (reply2, reply9, read_length);

	return 0;
}




/*
 * ndmp_tape_get_state_reply
 ****************************************************************
 */

extern int
ndmp_2to9_tape_get_state_reply (
  ndmp2_tape_get_state_reply *reply2,
  ndmp9_tape_get_state_reply *reply9)
{
	CNVT_E_TO_9 (reply2, reply9, error, ndmp_29_error);
	CNVT_TO_9 (reply2, reply9, flags);
	CNVT_VUL_TO_9 (reply2, reply9, file_num);
	CNVT_VUL_TO_9 (reply2, reply9, soft_errors);
	CNVT_VUL_TO_9 (reply2, reply9, block_size);
	CNVT_VUL_TO_9 (reply2, reply9, blockno);
	CNVT_VUQ_TO_9 (reply2, reply9, total_space);
	CNVT_VUQ_TO_9 (reply2, reply9, space_remain);

	return 0;
}

extern int
ndmp_9to2_tape_get_state_reply (
  ndmp9_tape_get_state_reply *reply9,
  ndmp2_tape_get_state_reply *reply2)
{
	CNVT_E_FROM_9 (reply2, reply9, error, ndmp_29_error);
	CNVT_FROM_9 (reply2, reply9, flags);
	CNVT_VUL_FROM_9 (reply2, reply9, file_num);
	CNVT_VUL_FROM_9 (reply2, reply9, soft_errors);
	CNVT_VUL_FROM_9 (reply2, reply9, block_size);
	CNVT_VUL_FROM_9 (reply2, reply9, blockno);
	CNVT_VUQ_FROM_9 (reply2, reply9, total_space);
	CNVT_VUQ_FROM_9 (reply2, reply9, space_remain);

	return 0;
}




/*
 * ndmp_mover_get_state_reply
 ****************************************************************
 */

struct enum_conversion	ndmp_29_mover_state[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP2_MOVER_STATE_IDLE,		NDMP9_MOVER_STATE_IDLE },
      { NDMP2_MOVER_STATE_LISTEN,	NDMP9_MOVER_STATE_LISTEN },
      { NDMP2_MOVER_STATE_ACTIVE,	NDMP9_MOVER_STATE_ACTIVE },
      { NDMP2_MOVER_STATE_PAUSED,	NDMP9_MOVER_STATE_PAUSED },
      { NDMP2_MOVER_STATE_HALTED,	NDMP9_MOVER_STATE_HALTED },

	/* alias */
      { NDMP2_MOVER_STATE_ACTIVE,	NDMP9_MOVER_STATE_STANDBY },

	END_ENUM_CONVERSION_TABLE
};

struct enum_conversion	ndmp_29_mover_pause_reason[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP2_MOVER_PAUSE_NA,		NDMP9_MOVER_PAUSE_NA },
      { NDMP2_MOVER_PAUSE_EOM,		NDMP9_MOVER_PAUSE_EOM },
      { NDMP2_MOVER_PAUSE_EOF,		NDMP9_MOVER_PAUSE_EOF },
      { NDMP2_MOVER_PAUSE_SEEK,		NDMP9_MOVER_PAUSE_SEEK },
      { NDMP2_MOVER_PAUSE_MEDIA_ERROR,	NDMP9_MOVER_PAUSE_MEDIA_ERROR },
	END_ENUM_CONVERSION_TABLE
};

struct enum_conversion	ndmp_29_mover_halt_reason[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP2_MOVER_HALT_NA,		NDMP9_MOVER_HALT_NA },
      { NDMP2_MOVER_HALT_CONNECT_CLOSED, NDMP9_MOVER_HALT_CONNECT_CLOSED },
      { NDMP2_MOVER_HALT_ABORTED,	NDMP9_MOVER_HALT_ABORTED },
      { NDMP2_MOVER_HALT_INTERNAL_ERROR, NDMP9_MOVER_HALT_INTERNAL_ERROR },
      { NDMP2_MOVER_HALT_CONNECT_ERROR,	NDMP9_MOVER_HALT_CONNECT_ERROR },
	END_ENUM_CONVERSION_TABLE
};


extern int
ndmp_2to9_mover_get_state_reply (
  ndmp2_mover_get_state_reply *reply2,
  ndmp9_mover_get_state_reply *reply9)
{
	CNVT_E_TO_9 (reply2, reply9, error, ndmp_29_error);
	CNVT_E_TO_9 (reply2, reply9, state, ndmp_29_mover_state);
	CNVT_E_TO_9 (reply2, reply9, pause_reason, ndmp_29_mover_pause_reason);
	CNVT_E_TO_9 (reply2, reply9, halt_reason, ndmp_29_mover_halt_reason);

	CNVT_TO_9 (reply2, reply9, record_size);
	CNVT_TO_9 (reply2, reply9, record_num);
	CNVT_TO_9x (reply2, reply9, data_written, bytes_moved);
	CNVT_TO_9 (reply2, reply9, seek_position);
	CNVT_TO_9 (reply2, reply9, bytes_left_to_read);
	CNVT_TO_9 (reply2, reply9, window_offset);
	CNVT_TO_9 (reply2, reply9, window_length);

	return 0;
}

extern int
ndmp_9to2_mover_get_state_reply (
  ndmp9_mover_get_state_reply *reply9,
  ndmp2_mover_get_state_reply *reply2)
{
	CNVT_E_FROM_9 (reply2, reply9, error, ndmp_29_error);
	CNVT_E_FROM_9 (reply2, reply9, state, ndmp_29_mover_state);
	CNVT_E_FROM_9 (reply2, reply9, pause_reason,
						ndmp_29_mover_pause_reason);
	CNVT_E_FROM_9 (reply2, reply9, halt_reason,
						ndmp_29_mover_halt_reason);

	CNVT_FROM_9 (reply2, reply9, record_size);
	CNVT_FROM_9 (reply2, reply9, record_num);
	CNVT_FROM_9x (reply2, reply9, data_written, bytes_moved);
	CNVT_FROM_9 (reply2, reply9, seek_position);
	CNVT_FROM_9 (reply2, reply9, bytes_left_to_read);
	CNVT_FROM_9 (reply2, reply9, window_offset);
	CNVT_FROM_9 (reply2, reply9, window_length);

	return 0;
}




/*
 * ndmp[_mover]_addr
 ****************************************************************
 */

extern int
ndmp_2to9_mover_addr (
  ndmp2_mover_addr *addr2,
  ndmp9_addr *addr9)
{
	switch (addr2->addr_type) {
	case NDMP2_ADDR_LOCAL:
		addr9->addr_type = NDMP9_ADDR_LOCAL;
		break;

	case NDMP2_ADDR_TCP:
		addr9->addr_type = NDMP9_ADDR_TCP;
		addr9->ndmp9_addr_u.tcp_addr.ip_addr =
			addr2->ndmp2_mover_addr_u.addr.ip_addr;
		addr9->ndmp9_addr_u.tcp_addr.port =
			addr2->ndmp2_mover_addr_u.addr.port;
		break;

	default:
		NDMOS_MACRO_ZEROFILL (addr9);
		addr9->addr_type = -1;
		return -1;
	}

	return 0;
}

extern int
ndmp_9to2_mover_addr (
  ndmp9_addr *addr9,
  ndmp2_mover_addr *addr2)
{
	switch (addr9->addr_type) {
	case NDMP9_ADDR_LOCAL:
		addr2->addr_type = NDMP2_ADDR_LOCAL;
		break;

	case NDMP9_ADDR_TCP:
		addr2->addr_type = NDMP2_ADDR_TCP;
		addr2->ndmp2_mover_addr_u.addr.ip_addr =
			addr9->ndmp9_addr_u.tcp_addr.ip_addr;
		addr2->ndmp2_mover_addr_u.addr.port =
			addr9->ndmp9_addr_u.tcp_addr.port;
		break;

	default:
		NDMOS_MACRO_ZEROFILL (addr2);
		addr2->addr_type = -1;
		return -1;
	}

	return 0;
}




/*
 * ndmp[_unix]_file_stat
 ****************************************************************
 */

struct enum_conversion	ndmp_29_file_type[] = {
      { NDMP_INVALID_GENERAL,		NDMP_INVALID_GENERAL, }, /* default */
      { NDMP2_FILE_DIR,			NDMP9_FILE_DIR },
      { NDMP2_FILE_FIFO,		NDMP9_FILE_FIFO },
      { NDMP2_FILE_CSPEC,		NDMP9_FILE_CSPEC },
      { NDMP2_FILE_BSPEC,		NDMP9_FILE_BSPEC },
      { NDMP2_FILE_REG,			NDMP9_FILE_REG },
      { NDMP2_FILE_SLINK,		NDMP9_FILE_SLINK },
      { NDMP2_FILE_SOCK,		NDMP9_FILE_SOCK },
	END_ENUM_CONVERSION_TABLE
};


extern int
ndmp_2to9_unix_file_stat (
  ndmp2_unix_file_stat *fstat2,
  ndmp9_file_stat *fstat9,
  ndmp9_u_quad node)
{
	CNVT_E_TO_9 (fstat2, fstat9, ftype, ndmp_29_file_type);

	CNVT_VUL_TO_9 (fstat2, fstat9, mtime);
	CNVT_VUL_TO_9 (fstat2, fstat9, atime);
	CNVT_VUL_TO_9 (fstat2, fstat9, ctime);
	CNVT_VUL_TO_9 (fstat2, fstat9, uid);
	CNVT_VUL_TO_9 (fstat2, fstat9, gid);

	CNVT_VUL_TO_9 (fstat2, fstat9, mode);

	CNVT_VUQ_TO_9 (fstat2, fstat9, size);
	CNVT_VUQ_TO_9 (fstat2, fstat9, fh_info);

	convert_valid_u_quad_to_9 (&node, &fstat9->node);

	return 0;
}

extern int
ndmp_9to2_unix_file_stat (
  ndmp9_file_stat *fstat9,
  ndmp2_unix_file_stat *fstat2)
{
	CNVT_E_FROM_9 (fstat2, fstat9, ftype, ndmp_29_file_type);

	CNVT_VUL_FROM_9 (fstat2, fstat9, mtime);
	CNVT_VUL_FROM_9 (fstat2, fstat9, atime);
	CNVT_VUL_FROM_9 (fstat2, fstat9, ctime);
	CNVT_VUL_FROM_9 (fstat2, fstat9, uid);
	CNVT_VUL_FROM_9 (fstat2, fstat9, gid);

	CNVT_VUL_FROM_9 (fstat2, fstat9, mode);

	CNVT_VUQ_FROM_9 (fstat2, fstat9, size);
	CNVT_VUQ_FROM_9 (fstat2, fstat9, fh_info);

	/* node ignored */

	return 0;
}




/*
 * ndmp_pval
 ****************************************************************
 */

extern int
ndmp_2to9_pval (
  ndmp2_pval *pval2,
  ndmp9_pval *pval9)
{
	CNVT_TO_9 (pval2, pval9, name);
	CNVT_TO_9 (pval2, pval9, value);

	return 0;
}

extern int
ndmp_9to2_pval (
  ndmp9_pval *pval9,
  ndmp2_pval *pval2)
{
	CNVT_FROM_9 (pval2, pval9, name);
	CNVT_FROM_9 (pval2, pval9, value);

	return 0;
}

extern int
ndmp_2to9_pval_vec (
  ndmp2_pval *pval2,
  ndmp9_pval *pval9,
  unsigned n_pval)
{
	int		i;

	for (i = 0; i < n_pval; i++)
		ndmp_2to9_pval (&pval2[i], &pval9[i]);

	return 0;
}

extern int
ndmp_9to2_pval_vec (
  ndmp9_pval *pval9,
  ndmp2_pval *pval2,
  unsigned n_pval)
{
	int		i;

	for (i = 0; i < n_pval; i++)
		ndmp_9to2_pval (&pval9[i], &pval2[i]);

	return 0;
}




/*
 * ndmp_name
 ****************************************************************
 */

extern int
ndmp_2to9_name (
  ndmp2_name *name2,
  ndmp9_name *name9)
{
	CNVT_TO_9 (name2, name9, name);
	CNVT_TO_9 (name2, name9, dest);
	CNVT_TO_9 (name2, name9, fh_info);

	return 0;
}

extern int
ndmp_9to2_name (
  ndmp9_name *name9,
  ndmp2_name *name2)
{
	CNVT_FROM_9 (name2, name9, name);
	CNVT_FROM_9 (name2, name9, dest);
	CNVT_FROM_9 (name2, name9, fh_info);

	name2->ssid = 0;

	return 0;
}

extern int
ndmp_2to9_name_vec (
  ndmp2_name *name2,
  ndmp9_name *name9,
  unsigned n_name)
{
	int		i;

	for (i = 0; i < n_name; i++)
		ndmp_2to9_name (&name2[i], &name9[i]);

	return 0;
}

extern int
ndmp_9to2_name_vec (
  ndmp9_name *name9,
  ndmp2_name *name2,
  unsigned n_name)
{
	int		i;

	for (i = 0; i < n_name; i++)
		ndmp_9to2_name (&name9[i], &name2[i]);

	return 0;
}

#endif /* !NDMOS_OPTION_NO_NDMP2 */
