#
# Copyright (c) 1998,1999,2000
#	Traakan, Inc., Los Altos, CA
#	All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice unmodified, this list of conditions, and the following
#    disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

#
# Project:  NDMJOB
# Ident:    $Id: Makefile,v 1.1 2003/10/14 19:12:37 ern Exp $
#
# Description
#
#	This illustrates the strata (layers) of the
#	NDMJOB/NDMJOBLIB software, the scope of key
#	header (.h) files, and the source files
#	constituting each layer.
#
#  -  -  -  -  -	+---------------------------------------+
#  ^  ^  ^  ^  ndmjob.h	| NDMJOB Command	ndmjob_*.c	|
#  |  |  |  |  -	+---------------------------------------+
#  |  |  |  |		          NDMJOBLIB API "job"
#  |  |  |  |		+---------------------------------------+  \
#  |  |  |  |		| Rules       (NDMJLR)  ndmjr_*.[ch]	|   \
#  |  |  |  ndmagents.h	| Agents      (NDMJLA)  ndma_*.c	|    |
#  |  |  |  -		+---------------------------------------+    |
#  |  |  ndmlib.h	| Library     (NDMJLL)  ndml_*.c	|    |
#  |  |  -		+---------------------------------------+    |
#  |  ndmprotocol.h	| Protocol    (NDMJLP)  ndmp*.[chx]	| NDMJOBLIB
#  |  -			+---------------------------------------+    |
#  |			| SMC         (NDMJLS)  smc*.[ch]	|    |
#  |			| Formats     (NDMJLF)  snoop*.[ch]	|    |
#  |			| MD5         (NDMJL5)  md5*.[ch]	|    |
#  |			+---------------------------------------+    |
#  ndmos.h		| OS intf     (NDMJLO)  ndmos*.[ch]	|   /
#  -			+---------------------------------------+  /
#


#NDMOS_OPTIONS = -DNDMOS_OPTION_NO_CONTROL_AGENT
#NDMOS_OPTIONS = -DNDMOS_OPTION_NO_DATA_AGENT
#NDMOS_OPTIONS = -DNDMOS_OPTION_NO_TAPE_AGENT
#NDMOS_OPTIONS = -DNDMOS_OPTION_NO_ROBOT_AGENT
#NDMOS_OPTIONS = -DNDMOS_OPTION_NO_NDMP2
#NDMOS_OPTIONS = -DNDMOS_OPTION_NO_NDMP3
#NDMOS_OPTIONS = -DNDMOS_OPTION_NO_NDMP4

#NDMOS_OPTIONS = -DNDMOS_OPTION_NO_NDMP3

# Set this if the autorecognizer in ndmos.h doesn't work
#NDMOS_ID = -DNDMOS_ID=NDMOS_ID_FREEBSD
#NDMOS_ID = -DNDMOS_ID=NDMOS_ID_SOLARIS
#NDMOS_ID = -DNDMOS_ID=NDMOS_ID_LINUX
#NDMOS_ID = -DNDMOS_ID=NDMOS_ID_XXX


#Generic
NDMOS_OPTIONS = -DNDMOS_CONST_NDMJOBLIB_REVISION='"(pre-v4)"'

#FreeBSD
LIBS=
DEFS = $(NDMOS_ID) $(NDMOS_OPTIONS)
CFLAGS = -g $(DEFS) -Wall -O

#Solaris
#LIBS=-lsocket -lnsl
#DEFS = $(NDMOS_ID) $(NDMOS_OPTIONS)
#CFLAGS =  $(DEFS)


NDMJOB_OBJS = \
	ndmjob_main.o \
	ndmjob_args.o \
	ndmjob_job.o \
	ndmjob_rules.o

NDMJLR_OBJS = \
	ndmjr_none.o

NDMJLA_OBJS = \
	ndma_comm_job.o \
	ndma_comm_session.o \
	ndma_comm_dispatch.o \
	ndma_comm_subr.o \
	ndma_control.o \
	ndma_cops_backreco.o \
	ndma_cops_labels.o \
	ndma_cops_query.o \
	ndma_cops_robot.o \
	ndma_ctrl_calls.o \
	ndma_ctrl_conn.o \
	ndma_ctrl_media.o \
	ndma_ctrl_robot.o \
	ndma_ctst_tape.o \
	ndma_ctst_mover.o \
	ndma_ctst_data.o \
	ndma_ctst_subr.o \
	ndma_data.o \
	ndma_data_butype.o \
	ndma_data_fh.o \
	ndma_data_gtar.o \
	ndma_data_pfe.o \
	ndma_data_recover.o \
	ndma_image_stream.o \
	ndma_noti_calls.o \
	ndma_tape.o \
	ndma_tape_simulator.o \
	ndma_robot.o

NDMJLL_OBJS = \
	ndml_agent.o \
	ndml_chan.o \
	ndml_conn.o \
	ndml_cstr.o \
	ndml_log.o \
	ndml_md5.o \
	ndml_fhh.o \
	ndml_media.o \
	ndml_nmb.o \
	ndml_scsi.o \
	ndml_util.o

NDMJLP_OBJS = \
	ndmprotocol.o \
	ndmp_translate.o \
	ndmp2_translate.o ndmp3_translate.o ndmp4_translate.o \
	ndmp0_enum_strs.o ndmp0_pp.o ndmp0_xdr.o ndmp0_xmt.o \
	ndmp2_enum_strs.o ndmp2_pp.o ndmp2_xdr.o ndmp2_xmt.o \
	ndmp3_enum_strs.o ndmp3_pp.o ndmp3_xdr.o ndmp3_xmt.o \
	ndmp4_enum_strs.o ndmp4_pp.o ndmp4_xdr.o ndmp4_xmt.o \
	ndmp9_enum_strs.o

NDMJLS_OBJS = \
	smc_api.o \
	smc_parse.o \
	smc_pp.o

NDMJLF_OBJS = \
	snoop_tar.o

NDMJL5_OBJS = \
	md5c.o

NDMJLO_SRCS = \
	ndmos.h ndmos.c \
	ndmos_solaris.h ndmos_solaris.c \
	ndmos_freebsd.h ndmos_freebsd.c \
	ndmos_linux.h ndmos_linux.c

NDMJLO_OBJS = ndmos.o

NDMJOBLIB = \
	ndmjla.rel \
	ndmjlr.rel \
	ndmjll.rel \
	ndmjlp.rel \
	ndmjlf.rel \
	ndmjls.rel \
	ndmjl5.rel \
	ndmjlo.rel


all: ndmjob

ndmjob: $(NDMJOB_OBJS) $(NDMJOBLIB)
	cc -o ndmjob  $(NDMJOB_OBJS) $(NDMJOBLIB) $(LIBS)

ndmjoblib: $(NDMJOBLIB)


ndmjla.rel : $(NDMJLA_OBJS)
	ld -r -o ndmjla.rel $(NDMJLA_OBJS)

ndmjlr.rel : $(NDMJLR_OBJS)
	ld -r -o ndmjlr.rel $(NDMJLR_OBJS)

ndmjll.rel : $(NDMJLL_OBJS)
	ld -r -o ndmjll.rel $(NDMJLL_OBJS)

ndmjlp.rel : $(NDMJLP_OBJS)
	ld -r -o ndmjlp.rel $(NDMJLP_OBJS)

ndmjlf.rel : $(NDMJLF_OBJS)
	ld -r -o ndmjlf.rel $(NDMJLF_OBJS)

ndmjls.rel : $(NDMJLS_OBJS)
	ld -r -o ndmjls.rel $(NDMJLS_OBJS)

ndmjl5.rel : $(NDMJL5_OBJS)
	ld -r -o ndmjl5.rel $(NDMJL5_OBJS)

ndmjlo.rel : $(NDMJLO_OBJS)
	ld -r -o ndmjlo.rel $(NDMJLO_OBJS)

$(NDMJLO_OBJS) : $(NDMJLO_SRCS)




#
# The NDMJOBLIB is dropped with rpcgen(1) results.
# You shouldn't have to re-run rpcgen(1) unless
# you change something, which is rarely a good idea.
#
ndmp_prot :
	$(MAKE) ndmp0.h ndmp2.h ndmp3.h ndmp4.h ndmp9.h

ndmp0.h ndmp0_xdr.c : ndmp0.x
	rpcgen ndmp0.x

ndmp2.h ndmp2_xdr.c : ndmp2.x
	rpcgen ndmp2.x

ndmp3.h ndmp3_xdr.c : ndmp3.x
	rpcgen ndmp3.x

ndmp4.h ndmp4_xdr.c : ndmp4.x
	rpcgen ndmp4.x

ndmp9.h ndmp9_xdr.c : ndmp9.x
	rpcgen ndmp9.x
	rm -f ndmp9_xdr.c




clean:
	rm -f *.o *.rel ndmjob *.core

veryclean:
	rm -f ndmp0.h ndmp0_xdr.c
	rm -f ndmp2.h ndmp2_xdr.c
	rm -f ndmp3.h ndmp3_xdr.c
	rm -f ndmp4.h ndmp4_xdr.c
	rm -f ndmp9.h
	rm -f *.o *.rel ndmjob *.core

