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
 * Ident:    $Id: ndmp4.x,v 1.1 2003/10/14 19:12:43 ern Exp $
 *
 * Description:
 *
 */



/*
 * ndmp4.x
 *
 * Description	 : NDMP protocol rpcgen file.
 *
 * Copyright (c) 1999 Intelliguard Software, Network Appliance.
 * All Rights Reserved.
 *
 * $Id: ndmp4.x,v 1.1 2003/10/14 19:12:43 ern Exp $
 */

%#ifndef NDMOS_OPTION_NO_NDMP4

const NDMP4VER = 4;
const NDMP4PORT = 10000;

%#define ndmp4_u_quad unsigned long long
%extern bool_t xdr_ndmp4_u_quad();

struct _ndmp4_u_quad
{
	u_long		high;
	u_long		low;
};

struct ndmp4_pval
{
	string		name<>;
	string		value<>;
};

typedef u_long ndmp4_error;

const NDMP4_NO_ERR                   = 0x00000000;
const NDMP4_NOT_SUPPORTED_ERR        = 0x00000001;
const NDMP4_DEVICE_BUSY_ERR          = 0x00000002;
const NDMP4_DEVICE_OPENED_ERR        = 0x00000003;
const NDMP4_NOT_AUTHORIZED_ERR       = 0x00000004;
const NDMP4_PERMISSION_ERR           = 0x00000005;
const NDMP4_DEV_NOT_OPEN_ERR         = 0x00000006;
const NDMP4_IO_ERR                   = 0x00000007;
const NDMP4_TIMEOUT_ERR              = 0x00000008;
const NDMP4_ILLEGAL_ARGS_ERR         = 0x00000009;
const NDMP4_NO_TAPE_LOADED_ERR       = 0x0000000A;
const NDMP4_WRITE_PROTECT_ERR        = 0x0000000B;
const NDMP4_EOF_ERR                  = 0x0000000C;
const NDMP4_EOM_ERR                  = 0x0000000D;
const NDMP4_FILE_NOT_FOUND_ERR       = 0x0000000E;
const NDMP4_BAD_FILE_ERR             = 0x0000000F;
const NDMP4_NO_DEVICE_ERR            = 0x00000010;
const NDMP4_NO_BUS_ERR               = 0x00000011;
const NDMP4_XDR_DECODE_ERR           = 0x00000012;
const NDMP4_ILLEGAL_STATE_ERR        = 0x00000013;
const NDMP4_UNDEFINED_ERR            = 0x00000014;
const NDMP4_XDR_ENCODE_ERR           = 0x00000015;
const NDMP4_NO_MEM_ERR               = 0x00000016;
const NDMP4_CONNECT_ERR              = 0x00000017;
const NDMP4_SEQUENCE_NUM_ERR         = 0x00000018;
const NDMP4_READ_IN_PROGRESS_ERR     = 0x00000019;


enum ndmp4_header_message_type
{
	NDMP4_MESSAGE_REQUEST,
	NDMP4_MESSAGE_REPLY
};

typedef u_long ndmp4_message;

/* Connect Interface */
const NDMP4_CONNECT_OPEN                   = 0x900;
const NDMP4_CONNECT_CLIENT_AUTH            = 0x901;
const NDMP4_CONNECT_CLOSE                  = 0x902;
const NDMP4_CONNECT_SERVER_AUTH            = 0x903;

/* Config Interface */
const NDMP4_CONFIG_GET_HOST_INFO           = 0x100;
const NDMP4_CONFIG_GET_CONNECTION_TYPE     = 0x102;
const NDMP4_CONFIG_GET_AUTH_ATTR           = 0x103;
const NDMP4_CONFIG_GET_BUTYPE_INFO         = 0x104;
const NDMP4_CONFIG_GET_FS_INFO             = 0x105;
const NDMP4_CONFIG_GET_TAPE_INFO           = 0x106;
const NDMP4_CONFIG_GET_SCSI_INFO           = 0x107;
const NDMP4_CONFIG_GET_SERVER_INFO         = 0x108;

/* SCSI Interface */
const NDMP4_SCSI_OPEN                      = 0x200;
const NDMP4_SCSI_CLOSE                     = 0x201;
const NDMP4_SCSI_GET_STATE                 = 0x202;
                                         /* 0x203 was NDMP4_SCSI_SET_TARGET */
const NDMP4_SCSI_RESET_DEVICE              = 0x204;
                                         /* 0x205 was NDMP4_SCSI_RESET_BUS */
const NDMP4_SCSI_EXECUTE_CDB               = 0x206;

/* Tape Interface */
const NDMP4_TAPE_OPEN                      = 0x300;
const NDMP4_TAPE_CLOSE                     = 0x301;
const NDMP4_TAPE_GET_STATE                 = 0x302;
const NDMP4_TAPE_MTIO                      = 0x303;
const NDMP4_TAPE_WRITE                     = 0x304;
const NDMP4_TAPE_READ                      = 0x305;
const NDMP4_TAPE_EXECUTE_CDB               = 0x307;

/* Data Interface */
const NDMP4_DATA_GET_STATE                 = 0x400;
const NDMP4_DATA_START_BACKUP              = 0x401;
const NDMP4_DATA_START_RECOVER             = 0x402;
const NDMP4_DATA_ABORT                     = 0x403;
const NDMP4_DATA_GET_ENV                   = 0x404;
const NDMP4_DATA_STOP                      = 0x407;
const NDMP4_DATA_LISTEN                    = 0x409;
const NDMP4_DATA_CONNECT                   = 0x40a;
const NDMP4_DATA_START_RECOVER_FILEHIST    = 0x40b; /* non-standard */

/* Notify Interface */
const NDMP4_NOTIFY_DATA_HALTED             = 0x501;
const NDMP4_NOTIFY_CONNECTION_STATUS       = 0x502;
const NDMP4_NOTIFY_MOVER_HALTED            = 0x503;
const NDMP4_NOTIFY_MOVER_PAUSED            = 0x504;
const NDMP4_NOTIFY_DATA_READ               = 0x505;

/* Log Interface */
const NDMP4_LOG_FILE                       = 0x602;
const NDMP4_LOG_MESSAGE                    = 0x603;

/* File history interface */
const NDMP4_FH_ADD_FILE                    = 0x703;
const NDMP4_FH_ADD_DIR                     = 0x704;
const NDMP4_FH_ADD_NODE                    = 0x705;

/* Mover Interface */
const NDMP4_MOVER_GET_STATE                = 0xa00;
const NDMP4_MOVER_LISTEN                   = 0xa01;
const NDMP4_MOVER_CONTINUE                 = 0xa02;
const NDMP4_MOVER_ABORT                    = 0xa03;
const NDMP4_MOVER_STOP                     = 0xa04;
const NDMP4_MOVER_SET_WINDOW               = 0xa05;
const NDMP4_MOVER_READ                     = 0xa06;
const NDMP4_MOVER_CLOSE                    = 0xa07;
const NDMP4_MOVER_SET_RECORD_SIZE          = 0xa08;
const NDMP4_MOVER_CONNECT                  = 0xa09;

/* Reserved for class extensibility */
/* from 0xff00 thru 0xffffffff */
const NDMP4_RESERVED_BASE                  = 0x0000ff00;

struct ndmp4_header
{
	u_long			sequence;	/* monotonically increasing */
	u_long			time_stamp;	/* time stamp of message */
	ndmp4_header_message_type message_type;	/* what type of message */
	ndmp4_message		message;	/* message number */
	u_long			reply_sequence;	/* reply is in response to */
	ndmp4_error		error;		/* communications errors */
};

/**********************/
/*  CONNECT INTERFACE */
/**********************/

/* NDMP4_CONNECT_OPEN */
struct ndmp4_connect_open_request
{
	u_short	protocol_version;	/* the version of protocol supported */
};

struct ndmp4_connect_open_reply
{
	ndmp4_error	error;
};

/* NDMP4_CONNECT_CLIENT_AUTH */
enum ndmp4_auth_type
{
	NDMP4_AUTH_NONE,		/* no password is required */
	NDMP4_AUTH_TEXT,		/* the clear text password */
	NDMP4_AUTH_MD5			/* md5 */
};

struct ndmp4_auth_text
{
	string		auth_id<>;
	string		auth_password<>;

};

struct ndmp4_auth_md5
{
	string		auth_id<>;
	opaque		auth_digest[16];
};

union ndmp4_auth_data switch (enum ndmp4_auth_type auth_type)
{
    case NDMP4_AUTH_NONE:
	void;
    case NDMP4_AUTH_TEXT:
	struct ndmp4_auth_text	auth_text;
    case NDMP4_AUTH_MD5:
	struct ndmp4_auth_md5	auth_md5;
};

struct ndmp4_connect_client_auth_request
{
	ndmp4_auth_data	auth_data;
};

struct ndmp4_connect_client_auth_reply
{
	ndmp4_error	error;
};


/* NDMP4_CONNECT_CLOSE */
/* no request arguments */
/* no reply arguments */

/* NDMP4_CONNECT_SERVER_AUTH */
union ndmp4_auth_attr switch (enum ndmp4_auth_type auth_type)
{
    case NDMP4_AUTH_NONE:
	void;
    case NDMP4_AUTH_TEXT:
	void;
    case NDMP4_AUTH_MD5:
	opaque	challenge[64];
};

struct ndmp4_connect_server_auth_request
{
	ndmp4_auth_attr	client_attr;
};

struct ndmp4_connect_server_auth_reply
{
	ndmp4_error		error;
	ndmp4_auth_data		server_result;
};


/********************/
/* CONFIG INTERFACE */
/********************/

/* NDMP4_CONFIG_GET_HOST_INFO */
/* no request arguments */
struct ndmp4_config_get_host_info_reply
{
	ndmp4_error	error;
	string		hostname<>;	/* host name */
	string		os_type<>;	/* The O/S type (e.g. SOLARIS) */
	string		os_vers<>;	/* The O/S version (e.g. 2.5) */
	string		hostid<>;
};

enum ndmp4_addr_type
{
	NDMP4_ADDR_LOCAL,
	NDMP4_ADDR_TCP,
	NDMP4_ADDR_RESERVED,		/* was NDMP4_ADDR_FC */
	NDMP4_ADDR_IPC
};

/* NDMP4_CONFIG_GET_SERVER_INFO */
/* no requset arguments */
struct ndmp4_config_get_server_info_reply
{
	ndmp4_error	error;
	string		vendor_name<>;
	string		product_name<>;
	string		revision_number<>;
	ndmp4_auth_type	auth_type<>;
};

/* NDMP4_CONFIG_GET_CONNECTION_TYPE */
/* no request arguments */
struct ndmp4_config_get_connection_type_reply
{
	ndmp4_error	error;
	ndmp4_addr_type	addr_types<>;
};

/* NDMP4_CONFIG_GET_AUTH_ATTR */
struct ndmp4_config_get_auth_attr_request
{
	ndmp4_auth_type	auth_type;
};

struct ndmp4_config_get_auth_attr_reply
{
	ndmp4_error	error;
	ndmp4_auth_attr	server_attr;
};

/* backup type attributes */
/*    NDMP4_BUTYPE_BACKUP_FILE_HISTORY	= 0x0001 */
const NDMP4_BUTYPE_BACKUP_FILELIST	= 0x0002;
const NDMP4_BUTYPE_RECOVER_FILELIST	= 0x0004;
const NDMP4_BUTYPE_BACKUP_DIRECT	= 0x0008;
const NDMP4_BUTYPE_RECOVER_DIRECT	= 0x0010;
const NDMP4_BUTYPE_BACKUP_INCREMENTAL	= 0x0020;
const NDMP4_BUTYPE_RECOVER_INCREMENTAL	= 0x0040;
const NDMP4_BUTYPE_BACKUP_UTF8		= 0x0080;
const NDMP4_BUTYPE_RECOVER_UTF8		= 0x0100;
const NDMP4_BUTYPE_BACKUP_FH_FILE	= 0x0200;
const NDMP4_BUTYPE_BACKUP_FH_DIR	= 0x0400;

struct ndmp4_butype_info
{
	string		butype_name<>;
	ndmp4_pval	default_env<>;
	u_long		attrs;
};

/* NDMP4_CONFIG_GET_BUTYPE_INFO */
/* no request arguments */
struct ndmp4_config_get_butype_info_reply
{
	ndmp4_error	error;
	ndmp4_butype_info butype_info<>;
};

/* invalid bit */
const	NDMP4_FS_INFO_TOTAL_SIZE_UNS	 	= 0x00000001;
const	NDMP4_FS_INFO_USED_SIZE_UNS		= 0x00000002;
const	NDMP4_FS_INFO_AVAIL_SIZE_UNS		= 0x00000004;
const	NDMP4_FS_INFO_TOTAL_INODES_UNS		= 0x00000008;
const	NDMP4_FS_INFO_USED_INODES_UNS		= 0x00000010;

struct ndmp4_fs_info
{
	u_long		unsupported;
	string		fs_type<>;
	string		fs_logical_device<>;
	string		fs_physical_device<>;
	ndmp4_u_quad	total_size;
	ndmp4_u_quad	used_size;
	ndmp4_u_quad	avail_size;
	ndmp4_u_quad	total_inodes;
	ndmp4_u_quad	used_inodes;
	ndmp4_pval	fs_env<>;
	string		fs_status<>;
};

/* NDMP4_CONFIG_GET_FS_INFO */
/* no request arguments */
struct ndmp4_config_get_fs_info_reply
{
	ndmp4_error	error;
	ndmp4_fs_info	fs_info<>;
};

/* NDMP4_CONFIG_GET_TAPE_INFO */
/* no request arguments */
/* tape attributes */
const NDMP4_TAPE_ATTR_REWIND	= 0x00000001;
const NDMP4_TAPE_ATTR_UNLOAD 	= 0x00000002;
const NDMP4_TAPE_ATTR_RAW 	= 0x00000004;

struct ndmp4_device_capability
{
	string		device<>;
	u_long		attr;
	ndmp4_pval	capability<>;
};

struct ndmp4_device_info
{
	string			model<>;
	ndmp4_device_capability	caplist<>;

};
struct ndmp4_config_get_tape_info_reply
{
	ndmp4_error		error;
	ndmp4_device_info	tape_info<>;

};

/* NDMP4_CONFIG_GET_SCSI_INFO */
/* jukebox attributes */
struct ndmp4_config_get_scsi_info_reply
{
	ndmp4_error		error;
	ndmp4_device_info	scsi_info<>;
};

/******************/
/* SCSI INTERFACE */
/******************/

/* NDMP4_SCSI_OPEN */
struct ndmp4_scsi_open_request
{
	string		device<>;
};

struct ndmp4_scsi_open_reply
{
	ndmp4_error	error;
};

/* NDMP4_SCSI_CLOSE */
/* no request arguments */
struct ndmp4_scsi_close_reply
{
	ndmp4_error	error;
};

/* NDMP4_SCSI_GET_STATE */
/* no request arguments */
struct ndmp4_scsi_get_state_reply
{
	ndmp4_error	error;
	short		target_controller;
	short		target_id;
	short		target_lun;
};

/* NDMP4_SCSI_RESET_DEVICE */
/* no request arguments */
struct ndmp4_scsi_reset_device_reply
{
	ndmp4_error	error;
};


/* NDMP4_SCSI_EXECUTE_CDB */
const NDMP4_SCSI_DATA_IN  = 0x00000001;	/* Expect data from SCSI device */
const NDMP4_SCSI_DATA_OUT = 0x00000002;	/* Transfer data to SCSI device */

struct ndmp4_execute_cdb_request
{
	u_long		flags;
	u_long		timeout;
	u_long		datain_len;	/* Set for expected datain */
	opaque		cdb<>;
	opaque		dataout<>;
};

struct ndmp4_execute_cdb_reply
{
	ndmp4_error	error;
	u_char		status;		/* SCSI status bytes */
	u_long		dataout_len;
	opaque		datain<>;	/* SCSI datain */
	opaque		ext_sense<>;	/* Extended sense data */
};
typedef ndmp4_execute_cdb_request	ndmp4_scsi_execute_cdb_request;
typedef ndmp4_execute_cdb_reply		ndmp4_scsi_execute_cdb_reply;

/******************/
/* TAPE INTERFACE */
/******************/
/* NDMP4_TAPE_OPEN */
enum ndmp4_tape_open_mode
{
	NDMP4_TAPE_READ_MODE,
	NDMP4_TAPE_RDWR_MODE,
	NDMP4_TAPE_RAW_MODE
};

struct ndmp4_tape_open_request
{
	string			device<>;
	ndmp4_tape_open_mode	mode;
};

struct ndmp4_tape_open_reply
{
	ndmp4_error	error;
};

/* NDMP4_TAPE_CLOSE */
/* no request arguments */
struct ndmp4_tape_close_reply
{
	ndmp4_error	error;
};

/*NDMP4_TAPE_GET_STATE */
/* no request arguments */
const NDMP4_TAPE_STATE_NOREWIND	= 0x0008;	/* non-rewind device */
const NDMP4_TAPE_STATE_WR_PROT	= 0x0010;	/* write-protected */
const NDMP4_TAPE_STATE_ERROR	= 0x0020;	/* media error */
const NDMP4_TAPE_STATE_UNLOAD	= 0x0040;	/* tape unloaded upon close */

/* unsupported bit */
const NDMP4_TAPE_STATE_FILE_NUM_UNS		= 0x00000001;
const NDMP4_TAPE_STATE_SOFT_ERRORS_UNS		= 0x00000002;
const NDMP4_TAPE_STATE_BLOCK_SIZE_UNS		= 0x00000004;
const NDMP4_TAPE_STATE_BLOCKNO_UNS		= 0x00000008;
const NDMP4_TAPE_STATE_TOTAL_SPACE_UNS		= 0x00000010;
const NDMP4_TAPE_STATE_SPACE_REMAIN_UNS		= 0x00000020;
/* const NDMP4_TAPE_STATE_PARTITION_UNS		= 0x00000040; */

struct ndmp4_tape_get_state_reply
{
	u_long		unsupported;
	ndmp4_error	error;
	u_long		flags;
	u_long		file_num;
	u_long		soft_errors;
	u_long		block_size;
	u_long		blockno;
	ndmp4_u_quad	total_space;
	ndmp4_u_quad	space_remain;
/*	u_long		partition; */
};

/* NDMP4_TAPE_MTIO */
enum ndmp4_tape_mtio_op
{
	NDMP4_MTIO_FSF,
	NDMP4_MTIO_BSF,
	NDMP4_MTIO_FSR,
	NDMP4_MTIO_BSR,
	NDMP4_MTIO_REW,
	NDMP4_MTIO_EOF,
	NDMP4_MTIO_OFF
};

struct ndmp4_tape_mtio_request
{
	ndmp4_tape_mtio_op	tape_op;
	u_long			count;
};

struct ndmp4_tape_mtio_reply
{
	ndmp4_error	error;
	u_long		resid_count;
};

/* NDMP4_TAPE_WRITE */
struct ndmp4_tape_write_request
{
	opaque		data_out<>;
};

struct ndmp4_tape_write_reply
{
	ndmp4_error	error;
	u_long		count;
};

/* NDMP4_TAPE_READ */
struct ndmp4_tape_read_request
{
	u_long		count;
};

struct ndmp4_tape_read_reply
{
	ndmp4_error	error;
	opaque		data_in<>;
};


/* NDMP4_TAPE_EXECUTE_CDB */
typedef ndmp4_execute_cdb_request	ndmp4_tape_execute_cdb_request;
typedef ndmp4_execute_cdb_reply		ndmp4_tape_execute_cdb_reply;


/********************************/
/* MOVER INTERFACE              */
/********************************/
/* NDMP4_MOVER_GET_STATE */
enum ndmp4_mover_state
{
	NDMP4_MOVER_STATE_IDLE,
	NDMP4_MOVER_STATE_LISTEN,
	NDMP4_MOVER_STATE_ACTIVE,
	NDMP4_MOVER_STATE_PAUSED,
	NDMP4_MOVER_STATE_HALTED
};

enum ndmp4_mover_pause_reason
{
	NDMP4_MOVER_PAUSE_NA,
	NDMP4_MOVER_PAUSE_EOM,
	NDMP4_MOVER_PAUSE_EOF,
	NDMP4_MOVER_PAUSE_SEEK,
	NDMP4_MOVER_PAUSE_MEDIA_ERROR,
	NDMP4_MOVER_PAUSE_EOW
};

enum ndmp4_mover_halt_reason
{
	NDMP4_MOVER_HALT_NA,
	NDMP4_MOVER_HALT_CONNECT_CLOSED,
	NDMP4_MOVER_HALT_ABORTED,
	NDMP4_MOVER_HALT_INTERNAL_ERROR,
	NDMP4_MOVER_HALT_CONNECT_ERROR
};

/* mover address */
enum ndmp4_mover_mode
{
	NDMP4_MOVER_MODE_READ,	/* read from data connection; write to tape */
	NDMP4_MOVER_MODE_WRITE	/* write to data connection; read from tape */
};

struct ndmp4_tcp_addr
{
	u_long	ip_addr;
	u_short	port;
};

struct ndmp4_fc_addr
{
	u_long	loop_id;
};

struct ndmp4_ipc_addr
{
	opaque	comm_data<>;
};
union ndmp4_addr switch (ndmp4_addr_type addr_type)
{
    case NDMP4_ADDR_LOCAL:
	void;
    case NDMP4_ADDR_TCP:
	ndmp4_tcp_addr	tcp_addr;
    case NDMP4_ADDR_IPC:
	ndmp4_ipc_addr	ipc_addr;
	
};

/* no request arguments */
struct ndmp4_mover_get_state_reply
{
	ndmp4_error		error;
	ndmp4_mover_state	state;
	ndmp4_mover_pause_reason pause_reason;
	ndmp4_mover_halt_reason	halt_reason;
	u_long			record_size;
	u_long			record_num;
	ndmp4_u_quad		bytes_moved;
	ndmp4_u_quad		seek_position;
	ndmp4_u_quad		bytes_left_to_read;
	ndmp4_u_quad		window_offset;
	ndmp4_u_quad		window_length;
	ndmp4_addr		data_connection_addr;
};

/* NDMP4_MOVER_LISTEN */
struct ndmp4_mover_listen_request
{
	ndmp4_mover_mode	mode;
	ndmp4_addr_type		addr_type;
};

struct ndmp4_mover_listen_reply
{
	ndmp4_error		error;
	ndmp4_addr		data_connection_addr;
};

/* NDMP4_MOVER_CONNECT */
struct ndmp4_mover_connect_request
{
	ndmp4_mover_mode	mode;
	ndmp4_addr		addr;
};

struct ndmp4_mover_connect_reply
{
	ndmp4_error	error;
};
/* NDMP4_MOVER_SET_RECORD_SIZE */
struct ndmp4_mover_set_record_size_request
{
	u_long		len;
};

struct ndmp4_mover_set_record_size_reply
{
	ndmp4_error	error;
};

/* NDMP4_MOVER_SET_WINDOW */
struct ndmp4_mover_set_window_request
{
	ndmp4_u_quad	offset;
	ndmp4_u_quad	length;
};

struct ndmp4_mover_set_window_reply
{
	ndmp4_error	error;
};

/* NDMP4_MOVER_CONTINUE */
/* no request arguments */
struct ndmp4_mover_continue_reply
{
	ndmp4_error	error;
};


/* NDMP4_MOVER_ABORT */
/* no request arguments */
struct ndmp4_mover_abort_reply
{
	ndmp4_error	error;
};

/* NDMP4_MOVER_STOP */
/* no request arguments */
struct ndmp4_mover_stop_reply
{
	ndmp4_error	error;
};

/* NDMP4_MOVER_READ */
struct ndmp4_mover_read_request
{
	ndmp4_u_quad	offset;
	ndmp4_u_quad	length;
};

struct ndmp4_mover_read_reply
{
	ndmp4_error	error;
};

/* NDMP4_MOVER_CLOSE */
/* no request arguments */
struct ndmp4_mover_close_reply
{
	ndmp4_error	error;
};

/********************************/
/* DATA INTERFACE		*/
/********************************/
/* NDMP4_DATA_GET_STATE */
/* no request arguments */
enum ndmp4_data_operation
{
	NDMP4_DATA_OP_NOACTION,
	NDMP4_DATA_OP_BACKUP,
	NDMP4_DATA_OP_RESTORE,
	NDMP4_DATA_OP_RESTORE_FILEHIST
};

enum ndmp4_data_state
{
	NDMP4_DATA_STATE_IDLE,
	NDMP4_DATA_STATE_ACTIVE,
	NDMP4_DATA_STATE_HALTED,
	NDMP4_DATA_STATE_LISTEN,
	NDMP4_DATA_STATE_CONNECTED
};

enum ndmp4_data_halt_reason
{
	NDMP4_DATA_HALT_NA,
	NDMP4_DATA_HALT_SUCCESSFUL,
	NDMP4_DATA_HALT_ABORTED,
	NDMP4_DATA_HALT_INTERNAL_ERROR,
	NDMP4_DATA_HALT_CONNECT_ERROR
};

/* invalid bit */
const NDMP4_DATA_STATE_EST_BYTES_REMAIN_UNS	= 0x00000001;
const NDMP4_DATA_STATE_EST_TIME_REMAIN_UNS	= 0x00000002;

struct ndmp4_data_get_state_reply
{
	u_long			unsupported;
	ndmp4_error		error;
	ndmp4_data_operation	operation;
	ndmp4_data_state	state;
	ndmp4_data_halt_reason	halt_reason;
	ndmp4_u_quad		bytes_processed;
	ndmp4_u_quad		est_bytes_remain;
	u_long			est_time_remain;
	ndmp4_addr		data_connection_addr;
	ndmp4_u_quad		read_offset;
	ndmp4_u_quad		read_length;
};

/* NDMP4_DATA_START_BACKUP */
struct ndmp4_data_start_backup_request
{
	string		bu_type<>;	/* backup method to use */
	ndmp4_pval	env<>;		/* Parameters that may modify backup */
};

struct ndmp4_data_start_backup_reply
{
	ndmp4_error	error;
};

/* NDMP4_DATA_START_RECOVER */
struct ndmp4_name
{
	string		original_path<>;
	string		destination_dir<>;
	string		new_name<>;	/* Direct access restore only */
	string		other_name<>;	/* Direct access restore only */
	ndmp4_u_quad	node;		/* Direct access restore only */
	ndmp4_u_quad	fh_info;	/* Direct access restore only */
};

struct ndmp4_data_start_recover_request
{
	ndmp4_pval	env<>;
	ndmp4_name	nlist<>;
	string		bu_type<>;
};

struct ndmp4_data_start_recover_reply
{
	ndmp4_error	error;
};

/* NDMP4_DATA_START_RECOVER_FILEHIST */
typedef ndmp4_data_start_recover_request ndmp4_data_start_recover_filehist_request;
typedef ndmp4_data_start_recover_reply ndmp4_data_start_recover_filehist_reply;


/* NDMP4_DATA_ABORT */
/* no request arguments */
struct ndmp4_data_abort_reply
{
	ndmp4_error	error;
};

/* NDMP4_DATA_STOP */
/* no request arguments */
struct ndmp4_data_stop_reply
{
	ndmp4_error	error;
};

/* NDMP4_DATA_GET_ENV */
/* no request arguments */
struct ndmp4_data_get_env_reply
{
	ndmp4_error	error;
	ndmp4_pval	env<>;
};

/* NDMP4_DATA_LISTEN */
struct ndmp4_data_listen_request
{
	ndmp4_addr_type	addr_type;
};

struct ndmp4_data_listen_reply
{
	ndmp4_error	error;
	ndmp4_addr	data_connection_addr;
};

/* NDMP4_DATA_CONNECT */
struct ndmp4_data_connect_request
{
	ndmp4_addr	addr;
};
struct ndmp4_data_connect_reply
{
	ndmp4_error	error;
};

/****************************************/
/* NOTIFY INTERFACE 			*/
/****************************************/
/* NDMP4_NOTIFY_DATA_HALTED */
struct ndmp4_notify_data_halted_post
{
	ndmp4_data_halt_reason	reason;
};

/* NDMP4_NOTIFY_CONNECTED */
enum ndmp4_connect_reason
{
	NDMP4_CONNECTED,	/* Connect sucessfully */
	NDMP4_SHUTDOWN,		/* Connection shutdown */
	NDMP4_REFUSED		/* reach the maximum number of connections */
};

struct ndmp4_notify_connection_status_post
{
	ndmp4_connect_reason	reason;
	u_short			protocol_version;
	string			text_reason<>;
};

/* NDMP4_NOTIFY_MOVER_PAUSED */
struct ndmp4_notify_mover_paused_post
{
	ndmp4_mover_pause_reason reason;
	ndmp4_u_quad		seek_position;
};
/* No reply */

/* NDMP4_NOTIFY_MOVER_HALTED */
struct ndmp4_notify_mover_halted_post
{
	ndmp4_mover_halt_reason	reason;
};
/* No reply */

/* NDMP4_NOTIFY_DATA_READ */
struct ndmp4_notify_data_read_post
{
	ndmp4_u_quad	offset;
	ndmp4_u_quad	length;
};
/* No reply */

/********************************/
/* LOG INTERFACE		*/
/********************************/
/* NDMP4_LOG_MESSAGE */
enum ndmp4_log_type
{
	NDMP4_LOG_NORMAL,
	NDMP4_LOG_DEBUG,
	NDMP4_LOG_ERROR,
	NDMP4_LOG_WARNING
};


struct ndmp4_log_message_post
{
	ndmp4_log_type	log_type;
	u_long		message_id;
	string		entry<>;
	bool		associated_message_valid;
	u_long		associated_message_sequence;
};
/* No reply */


enum ndmp4_recovery_status
{
	NDMP4_FILE_RECOVERY_SUCCESSFUL             = 0,
	NDMP4_FILE_RECOVERY_FAILED_PERMISSION      = 1,
	NDMP4_FILE_RECOVERY_FAILED_NOT_FOUND       = 2,
	NDMP4_FILE_RECOVERY_FAILED_NO_DIRECTORY    = 3,
	NDMP4_FILE_RECOVERY_FAILED_OUT_OF_MEMORY   = 4,
	NDMP4_FILE_RECOVERY_FAILED_IO_ERROR        = 5,
	NDMP4_FILE_RECOVERY_FAILED_UNDEFINED_ERROR = 6
};


/* NDMP4_LOG_FILE */
struct ndmp4_log_file_post
{
	string			name<>;
	ndmp4_recovery_status	recovery_status;
};
/* No reply */


/*****************************/
/* File History INTERFACE    */
/*****************************/
/* NDMP4_FH_ADD_FILE */
enum ndmp4_fs_type
{
	NDMP4_FS_UNIX,	/* UNIX */
	NDMP4_FS_NT,	/* NT */
	NDMP4_FS_OTHER
};

typedef string ndmp4_path<>;
struct ndmp4_nt_path
{
	ndmp4_path	nt_path;
	ndmp4_path	dos_path;
};

union ndmp4_file_name switch (ndmp4_fs_type fs_type)
{
    case NDMP4_FS_UNIX:
	ndmp4_path	unix_name;
    case NDMP4_FS_NT:
	ndmp4_nt_path	nt_name;
    default:
	ndmp4_path	other_name;
};

enum ndmp4_file_type
{
	NDMP4_FILE_DIR,
	NDMP4_FILE_FIFO,
	NDMP4_FILE_CSPEC,
	NDMP4_FILE_BSPEC,
	NDMP4_FILE_REG,
	NDMP4_FILE_SLINK,
	NDMP4_FILE_SOCK,
	NDMP4_FILE_REGISTRY,
	NDMP4_FILE_OTHER
};

/* invalid bit */
const NDMP4_FILE_STAT_ATIME_UNS	= 0x00000001;
const NDMP4_FILE_STAT_CTIME_UNS	= 0x00000002;
const NDMP4_FILE_STAT_GROUP_UNS	= 0x00000004;

struct ndmp4_file_stat
{
	u_long			unsupported;
	ndmp4_fs_type		fs_type;
	ndmp4_file_type		ftype;
	u_long			mtime;
	u_long			atime;
	u_long			ctime;
	u_long			owner; /* uid for UNIX, owner for NT */
	u_long			group; /* gid for UNIX, NA for NT */
	u_long			fattr; /* mode for UNIX, fattr for NT */
	ndmp4_u_quad		size;
	u_long			links;
};


/* one file could have both UNIX and NT name and attributes */
struct ndmp4_file
{
	ndmp4_file_name		names<>;
	ndmp4_file_stat		stats<>;
	ndmp4_u_quad		node;	/* used for the direct access */
	ndmp4_u_quad		fh_info;/* used for the direct access */
};

struct ndmp4_fh_add_file_post
{
	ndmp4_file		files<>;
};

/* No reply */
/* NDMP4_FH_ADD_DIR */

struct ndmp4_dir
{
	ndmp4_file_name		names<>;
	ndmp4_u_quad		node;
	ndmp4_u_quad		parent;
};

struct ndmp4_fh_add_dir_post
{
	ndmp4_dir		dirs<>;
};
/* No reply *;

/* NDMP4_FH_ADD_NODE */
struct ndmp4_node
{
	ndmp4_file_stat		stats<>;
	ndmp4_u_quad		node;
	ndmp4_u_quad		fh_info;
};

struct ndmp4_fh_add_node_post
{
	ndmp4_node		nodes<>;
};
/* No reply */

%#endif /* !NDMOS_OPTION_NO_NDMP4 */
