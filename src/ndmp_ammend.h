/*
 * Copyright (c) 1998,1999,2000
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
 * Ident:    $Id: ndmp_ammend.h,v 1.1 2004/01/12 18:06:31 ern Exp $
 *
 * Description:
 *
 */

/*
 * All invalid values are all 1s
 */
#define NDMP_INVALID_U_SHORT	(0xFFFFu)
#define NDMP_INVALID_U_LONG	(0xFFFFFFFFul)
#define NDMP_INVALID_U_QUAD	(0xFFFFFFFFFFFFFFFFull)

#define NDMP_INVALID_GENERAL	(-1)

#define NDMP_LENGTH_INFINITY	(~0ULL)


#define NDMP_TAPE_INVALID_FILE_NUM	NDMP_INVALID_U_LONG
#define NDMP_TAPE_INVALID_SOFT_ERRORS	NDMP_INVALID_U_LONG
#define NDMP_TAPE_INVALID_BLOCK_SIZE	NDMP_INVALID_U_LONG
#define NDMP_TAPE_INVALID_BLOCKNO	NDMP_INVALID_U_LONG
#define NDMP_TAPE_INVALID_TOTAL_SPACE	NDMP_INVALID_U_QUAD
#define NDMP_TAPE_INVALID_SPACE_REMAIN	NDMP_INVALID_U_QUAD
#define NDMP_TAPE_INVALID_PARTITION	NDMP_INVALID_U_LONG


#define NDMP_DATA_INVALID_EST_BYTES_REMAIN	NDMP_INVALID_U_QUAD
#define NDMP_DATA_INVALID_EST_TIME_REMAIN	NDMP_INVALID_U_QUAD


#define NDMP_FILE_STAT_INVALID_MTIME	NDMP_INVALID_U_LONG
#define NDMP_FILE_STAT_INVALID_ATIME	NDMP_INVALID_U_LONG
#define NDMP_FILE_STAT_INVALID_CTIME	NDMP_INVALID_U_LONG
#define NDMP_FILE_STAT_INVALID_UID	NDMP_INVALID_U_LONG
#define NDMP_FILE_STAT_INVALID_GID	NDMP_INVALID_U_LONG
#define NDMP_FILE_STAT_INVALID_LINKS	NDMP_INVALID_U_LONG
#define NDMP_FILE_STAT_INVALID_NODE	NDMP_INVALID_U_QUAD
#define NDMP_FILE_STAT_INVALID_SIZE	0


#define NDMP_FH_INVALID_FH_INFO		NDMP_INVALID_U_QUAD
