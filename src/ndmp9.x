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
 * Ident:    $Id: ndmp9.x,v 1.1 2003/10/14 19:12:44 ern Exp $
 *
 * Description:
 *	NDMPv9, represented here, is a ficticious version
 *	used internally and never over-the-wire. This
 *	isolates higher-level routines from variations
 *	between NDMP protocol version. At this time,
 *	NDMPv2 and NDMPv3 are deployed, and NDMPv4 is
 *	being contemplated. NDMPv9 tends to be bits and
 *	pieces of all supported protocol versions mashed
 *	together.
 *
 *	While we want the higher-level routines	isolated,
 *	for clarity we still want them to use data structures
 *	and construct that resemble NDMP. Higher-level routines
 *	manipulate NDMPv9 data structures. Mid-level routines
 *	translate between NDMPv9 and the over-the-wire version
 *	in use. Low-level routines do the over-the-wire functions.
 *
 *	The approach of using the latest version internally
 *	and retrofiting earlier versions was rejected for
 *	two reasons. First, it means a tear-up of higher-level
 *	functions as new versions are deployed. Second,
 *	it makes building with selected version impossible.
 *	No matter what approach is taken, there will be
 *	some sort of retrofit between versions. NDMPv9
 *	is simply the internal version, and all bona-fide
 *	versions are retrofitted. v9 was chosen because
 *	it is unlikely the NDMP version will reach 9
 *	within the useful life of the NDMP architecture.
 *
 *	NDMPv9 could be implemented in a hand-crafted header (.h)
 *	file, yet we continue to use the ONC RPC (.x) description
 *	for convenvience. It's easy to cut-n-paste from the other
 *	NDMP.x files. It's important that ndmp9_xdr.c never be
 *	generated nor compiled.
 */


/*
 * (from ndmp3.x)
 * ndmp.x
 * 
 * Description	 : NDMP protocol rpcgen file.
 * 
 * Copyright (c) 1999 Intelliguard Software, Network Appliance.
 * All Rights Reserved.
 */
/*
 * (from ndmp2.x)
 * Copyright (c) 1997 Network Appliance. All Rights Reserved.
 *
 * Network Appliance makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 */

%#define ndmp9_u_quad unsigned long long


enum ndmp9_validity {
	NDMP9_VALIDITY_INVALID = 0,
	NDMP9_VALIDITY_VALID,
	NDMP9_VALIDITY_MAYBE_INVALID,
	NDMP9_VALIDITY_MAYBE_VALID
};

/*
 * These ndmp9_valid_TYPE's were originally unions,
 * but it made using them a real pain. We always
 * manipulate the value regardles of ->valid,
 * so they were turned into structs.
 */
%#define NDMP9_INVALID_U_LONG	0xFFFFFFFFul
struct ndmp9_valid_u_long {
	ndmp9_validity	valid;
	u_long		value;
};

%#define NDMP9_INVALID_U_QUAD	0xFFFFFFFFFFFFFFFFul
struct ndmp9_valid_u_quad {
	ndmp9_validity	valid;
	ndmp9_u_quad	value;
};


struct ndmp9_pval
{
	string		name<>;
	string		value<>;
};

enum ndmp9_error
{
	NDMP9_NO_ERR,			/* No error */
	NDMP9_NOT_SUPPORTED_ERR,	/* Call is not supported */
	NDMP9_DEVICE_BUSY_ERR,		/* The device is in use */
	NDMP9_DEVICE_OPENED_ERR,	/* Another tape or scsi device
					 * is already open */
	NDMP9_NOT_AUTHORIZED_ERR,	/* connection has not been authorized*/
	NDMP9_PERMISSION_ERR,		/* some sort of permission problem */
	NDMP9_DEV_NOT_OPEN_ERR,		/* SCSI device is not open */
	NDMP9_IO_ERR,			/* I/O error */
	NDMP9_TIMEOUT_ERR,		/* command timed out */
	NDMP9_ILLEGAL_ARGS_ERR,		/* illegal arguments in request */
	NDMP9_NO_TAPE_LOADED_ERR,	/* Cannot open because there is
					   no tape loaded */
	NDMP9_WRITE_PROTECT_ERR,	/* tape cannot be open for write */
	NDMP9_EOF_ERR,			/* Command encountered EOF */
	NDMP9_EOM_ERR,			/* Command encountered EOM */
	NDMP9_FILE_NOT_FOUND_ERR,	/* File not found during restore */
	NDMP9_BAD_FILE_ERR,		/* The file descriptor is invalid */
	NDMP9_NO_DEVICE_ERR,		/* The device is not at that target */
	NDMP9_NO_BUS_ERR,		/* Invalid controller */
	NDMP9_XDR_DECODE_ERR,		/* Can't decode the request argument */
	NDMP9_ILLEGAL_STATE_ERR,	/* Call can't be done at this state */
	NDMP9_UNDEFINED_ERR,		/* Undefined Error */
	NDMP9_XDR_ENCODE_ERR,		/* Can't encode the reply argument */
	NDMP9_NO_MEM_ERR,		/* no memory */
	NDMP9_CONNECT_ERR		/* Error connecting to another
					 * NDMP server */
};

/* NDMP9_CONNECT_CLIENT_AUTH */
enum ndmp9_auth_type
{
	NDMP9_AUTH_NONE,		/* no password is required */
	NDMP9_AUTH_TEXT,		/* the clear text password */
	NDMP9_AUTH_MD5			/* md5 */
};


/********************/
/* CONFIG INTERFACE */
/********************/
struct ndmp9_local_info
{
	/* ndmp[23]_config_get_host_info_reply */
	string		hostname<>;	/* host name */
	string		os_type<>;	/* The O/S type (e.g. SOLARIS) */
	string		os_vers<>;	/* The O/S version (e.g. 2.5) */
	string		hostid<>;

	/* ndmp3_config_get_server_info_reply */
	string		vendor_name<>;
	string		product_name<>;
	string		revision_number<>;
};

/* NDMP9_CONFIG_GET_BUTYPE_ATTR */
const NDMP9_NO_BACKUP_FILELIST	= 0x0001;
const NDMP9_NO_BACKUP_FHINFO	= 0x0002;
const NDMP9_NO_RECOVER_FILELIST	= 0x0004;
const NDMP9_NO_RECOVER_FHINFO	= 0x0008;
const NDMP9_NO_RECOVER_RESVD	= 0x0010;
const NDMP9_NO_RECOVER_INC_ONLY	= 0x0020;

struct ndmp9_config_get_butype_attr_request
{
	string		name<>;		/* backup type name */
};

struct ndmp9_config_get_butype_attr_reply
{
	ndmp9_error	error;
	u_long		attrs;
};

enum ndmp9_addr_type
{
	NDMP9_ADDR_LOCAL,
	NDMP9_ADDR_TCP
};

struct ndmp9_tcp_addr
{
	u_long		ip_addr;
	u_short		port;
};

union ndmp9_addr switch (ndmp9_addr_type addr_type)
{
    case NDMP9_ADDR_LOCAL:
	void;
    case NDMP9_ADDR_TCP:
	ndmp9_tcp_addr	tcp_addr;
};

struct ndmp9_device_capability
{
	string		device<>;
	u_long		attr;
	ndmp9_pval	capability<>;
};

struct ndmp9_device_info
{
	string			model<>;
	ndmp9_device_capability	caplist<>;
};


/******************/
/* SCSI INTERFACE */
/******************/
struct ndmp9_scsi_get_state_reply
{
	ndmp9_error	error;
	short		target_controller;
	short		target_id;
	short		target_lun;
};



/******************/
/* TAPE INTERFACE */
/******************/
enum ndmp9_tape_open_mode
{
	NDMP9_TAPE_READ_MODE,
	NDMP9_TAPE_RDWR_MODE
};

enum ndmp9_tape_state
{
	NDMP9_TAPE_STATE_IDLE,		/* not doing anything */
	NDMP9_TAPE_STATE_OPEN,		/* open, tape operations OK */
	NDMP9_TAPE_STATE_MOVER		/* mover active, tape ops locked out */
					/* ie read, write, mtio, close, cdb */
};


const NDMP9_TAPE_NOREWIND = 0x0008;	/* non-rewind device */
const NDMP9_TAPE_WR_PROT  = 0x0010;	/* write-protected */
const NDMP9_TAPE_ERROR    = 0x0020;	/* media error */
const NDMP9_TAPE_UNLOAD   = 0x0040;	/* tape will be unloaded when
					 * the device is closed */
struct ndmp9_tape_get_state_reply
{
	ndmp9_error		error;
	ndmp9_tape_state	state;
	ndmp9_tape_open_mode 	open_mode;
	u_long			flags;
	ndmp9_valid_u_long	file_num;
	ndmp9_valid_u_long	soft_errors;
	ndmp9_valid_u_long	block_size;
	ndmp9_valid_u_long	blockno;
	ndmp9_valid_u_quad	total_space;
	ndmp9_valid_u_quad	space_remain;
};

/* NDMP9_TAPE_MTIO */
enum ndmp9_tape_mtio_op
{
	NDMP9_MTIO_FSF,
	NDMP9_MTIO_BSF,
	NDMP9_MTIO_FSR,
	NDMP9_MTIO_BSR,
	NDMP9_MTIO_REW,
	NDMP9_MTIO_EOF,
	NDMP9_MTIO_OFF
};


/********************************/
/* MOVER INTERFACE              */
/********************************/
enum ndmp9_mover_state
{
	NDMP9_MOVER_STATE_IDLE,
	NDMP9_MOVER_STATE_LISTEN,
	NDMP9_MOVER_STATE_ACTIVE,
	NDMP9_MOVER_STATE_PAUSED,
	NDMP9_MOVER_STATE_HALTED,
	NDMP9_MOVER_STATE_STANDBY	/* awaiting mover_read_request */
};

enum ndmp9_mover_mode
{
	NDMP9_MOVER_MODE_READ,	/* read from data conn; write to tape */
	NDMP9_MOVER_MODE_WRITE	/* write to data conn; read from tape */
};

enum ndmp9_mover_pause_reason
{
	NDMP9_MOVER_PAUSE_NA,
	NDMP9_MOVER_PAUSE_EOM,
	NDMP9_MOVER_PAUSE_EOF,
	NDMP9_MOVER_PAUSE_SEEK,
	NDMP9_MOVER_PAUSE_MEDIA_ERROR,
	NDMP9_MOVER_PAUSE_EOW
};

enum ndmp9_mover_halt_reason
{
	NDMP9_MOVER_HALT_NA,
	NDMP9_MOVER_HALT_CONNECT_CLOSED,
	NDMP9_MOVER_HALT_ABORTED,
	NDMP9_MOVER_HALT_INTERNAL_ERROR,
	NDMP9_MOVER_HALT_CONNECT_ERROR
};

struct ndmp9_mover_get_state_reply
{
	ndmp9_error		error;
	ndmp9_mover_state	state;
	ndmp9_mover_mode	mode;
	ndmp9_mover_pause_reason pause_reason;
	ndmp9_mover_halt_reason	halt_reason;
	u_long			record_size;
	u_long			record_num;
	ndmp9_u_quad		bytes_moved;
	ndmp9_u_quad		seek_position;
	ndmp9_u_quad		bytes_left_to_read;
	ndmp9_u_quad		window_offset;
	ndmp9_u_quad		window_length;
	ndmp9_addr		data_connection_addr;
};




/****************************/
/* DATA INTERFACE	    */
/****************************/

enum ndmp9_data_operation
{
	NDMP9_DATA_OP_NOACTION,
	NDMP9_DATA_OP_BACKUP,
	NDMP9_DATA_OP_RESTORE,
	NDMP9_DATA_OP_RESTORE_FILEHIST
};

enum ndmp9_data_state
{
	NDMP9_DATA_STATE_IDLE,
	NDMP9_DATA_STATE_ACTIVE,
	NDMP9_DATA_STATE_HALTED,
	NDMP9_DATA_STATE_LISTEN,
	NDMP9_DATA_STATE_CONNECTED
};

enum ndmp9_data_halt_reason
{
	NDMP9_DATA_HALT_NA,
	NDMP9_DATA_HALT_SUCCESSFUL,
	NDMP9_DATA_HALT_ABORTED,
	NDMP9_DATA_HALT_INTERNAL_ERROR,
	NDMP9_DATA_HALT_CONNECT_ERROR
};

struct ndmp9_data_get_state_reply
{
	ndmp9_error		error;
	ndmp9_data_operation	operation;
	ndmp9_data_state	state;
	ndmp9_data_halt_reason	halt_reason;
	ndmp9_u_quad		bytes_processed;
	ndmp9_valid_u_quad	est_bytes_remain;
	ndmp9_valid_u_long	est_time_remain;
	ndmp9_addr		data_connection_addr;
	ndmp9_u_quad		read_offset;
	ndmp9_u_quad		read_length;
};

struct ndmp9_name
{
	string		name<>;
	string		dest<>;
	ndmp9_u_quad	fh_info;
};


/****************************/
/* NOTIFY INTERFACE	    */
/****************************/

struct ndmp9_notify_mover_paused_request
{
	ndmp9_mover_pause_reason reason;
	ndmp9_u_quad		seek_position;
};

struct ndmp9_notify_data_read_request
{
	ndmp9_u_quad	offset;
	ndmp9_u_quad	length;
};

/********************************/
/* File History INTERFACE	*/
/********************************/


enum ndmp9_file_type
{
	NDMP9_FILE_DIR,
	NDMP9_FILE_FIFO,
	NDMP9_FILE_CSPEC,
	NDMP9_FILE_BSPEC,
	NDMP9_FILE_REG,
	NDMP9_FILE_SLINK,
	NDMP9_FILE_SOCK,
	NDMP9_FILE_REGISTRY,
	NDMP9_FILE_OTHER
};

struct ndmp9_file_stat
{
	ndmp9_file_type		ftype;
	ndmp9_valid_u_long	mtime;
	ndmp9_valid_u_long	atime;
	ndmp9_valid_u_long	ctime;
	ndmp9_valid_u_long	uid;
	ndmp9_valid_u_long	gid;
	ndmp9_valid_u_long	mode;
	ndmp9_valid_u_quad	size;
	ndmp9_valid_u_long	links;

	ndmp9_valid_u_quad	node;	    /* id on disk at backup time */
	ndmp9_valid_u_quad	fh_info;    /* id on tape at backup time */
};
