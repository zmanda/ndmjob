/*
 * Copyright (c) 2002
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
 * Ident:    $Id: smccmd.c,v 1.1.1.1 2004/01/12 18:04:26 ern Exp $
 *
 * Description:
 *  This is a stand-alone command that accesses a SCSI
 *  Media Changer (robot) through a local SCSI pass-thru.
 *  The main purpose of this file is to help work out
 *  how to use the native O/S SCSI interfaces. Once those
 *  interfaces are understood it is a straight forward matter
 *  to implement the ndmos_scsi_....() functions for NDMJOB.
 */


#include "ndmos.h"
#include "smc.h"

/* forward */
int	os_scsi_api_open(char *scsi_pass_thru_dev);
int	os_scsi_api_close(void);
int	os_scsi_api_issue_scsi_req (struct smc_ctrl_block *smc);

struct smc_ctrl_block	the_smc;



int
main (int ac, char *av[])
{
	struct smc_ctrl_block *	smc = &the_smc;
	int			rc, lineno, nline, i;
	char			buf[100], lnbuf[30];


	if (ac != 2) {
		printf ("usage: %s SCSI-PASS-THRU-DEV\n", av[0]);
		return 1;
	}

	if (os_scsi_api_open (av[1]) != 0) {
		/* message already printed */
		exit (2);
	}

	smc->issue_scsi_req = os_scsi_api_issue_scsi_req;

	rc = smc_inquire (smc);
	if (rc == 0) {
		printf ("Ident = '%s'\n", smc->ident);
	} else {
		printf ("INQ = %d\n", rc);
	}

	rc = smc_test_unit_ready (smc);
	if (rc != 0) {
		printf ("TUR = %d\n", rc);
	}

	rc = smc_get_elem_aa (smc);
	if (rc == 0) {
		for (lineno = 0, nline = 1; lineno < nline; lineno++) {
			rc = smc_pp_element_address_assignments (&smc->elem_aa,
							lineno, buf);
			if (rc < 0) {
				strcpy (buf, "PP-ERROR");
			}
			nline = rc;
			printf ("    %s\n", buf);
		}
	} else {
		printf ("GetElemAA = %d\n", rc);
	}

#if 0
	rc = smc_init_elem_status (smc);
	printf ("InitElemStat = %d\n", rc);
#endif

	rc = smc_read_elem_status (smc);
	if (rc == 0) {
		printf ("    E#  Addr Type Status\n");
		printf ("    --  ---- ---- ---------------------\n");
		for (i = 0; i < smc->n_elem_desc; i++) {
			struct smc_element_descriptor *	edp;

			edp = &smc->elem_desc[i];

			for (lineno = 0, nline = 1; lineno < nline; lineno++) {
				rc = smc_pp_element_descriptor (edp,
								lineno, buf);

				if (lineno == 0)
					sprintf (lnbuf, "    %2d ", i+1);
				else
					sprintf (lnbuf, "       ");

				if (rc < 0) {
					strcpy (buf, "PP-ERROR");
				}
				nline = rc;
				printf ("%s %s\n", lnbuf, buf);
			}
		}
	} else {
		printf ("ReadElemStatus = %d\n", rc);
	}

	os_scsi_api_close();

	return 0;
}


#if NDMOS_ID == NDMOS_ID_FREEBSD

#include <camlib.h>
#include <cam/scsi/scsi_message.h>

int
os_scsi_api_open (char *scsi_pass_thru_dev)
{
	struct cam_device *	camdev = 0;
	struct smc_ctrl_block *	smc = &the_smc;

	camdev = cam_open_pass(scsi_pass_thru_dev, 2, 0);
	if (!camdev) {
		perror (scsi_pass_thru_dev);
		return -1;
	}

	smc->app_data = camdev;

	printf ("cam_open_pass(%s) c=%d t=%d l=%d\n",
		scsi_pass_thru_dev,
		camdev->path_id, camdev->target_id, camdev->target_lun);

	return 0;
}

int
os_scsi_api_close (void)
{
	struct smc_ctrl_block *	smc = &the_smc;
	struct cam_device *	camdev = smc->app_data;

	smc->app_data = 0;
	cam_close_device (camdev);

	return 0;
}

int
os_scsi_api_issue_scsi_req (struct smc_ctrl_block *smc)
{
	struct cam_device *	camdev = smc->app_data;
	u_int8_t *		data_ptr = smc->scsi_req.data;
	u_int32_t		data_len = smc->scsi_req.n_data_avail;
	union ccb *		ccb;
	u_int32_t		flags;
	int			rc;

	smc->scsi_req.completion_status = SMCSR_CS_FAIL;

	ccb = cam_getccb(camdev);

	if (!ccb) {
		perror ("cam_getccb()");
		return -1;
	}


	switch (smc->scsi_req.data_dir) {
	case SMCSR_DD_NONE:
		flags = CAM_DIR_NONE;
		break;

	case SMCSR_DD_IN:
		flags = CAM_DIR_IN;
		break;

	case SMCSR_DD_OUT:
		flags = CAM_DIR_OUT;
		break;

	default:
		return -1;	/* illegal arg */
		break;
	}

	bcopy(smc->scsi_req.cmd, &ccb->csio.cdb_io.cdb_bytes,
		smc->scsi_req.n_cmd);

	cam_fill_csio(&ccb->csio,
		      /*retries*/ 1,
		      /*cbfcnp*/ NULL,
		      /*flags*/ flags,
		      /*tag_action*/ MSG_SIMPLE_Q_TAG,
		      /*data_ptr*/ data_ptr,
		      /*dxfer_len*/ data_len,
		      /*sense_len*/ SSD_FULL_SIZE,
		      /*cdb_len*/ smc->scsi_req.n_cmd,
		      /*timeout*/ 0);

	rc = cam_send_ccb (camdev, ccb);
	if (rc != 0) {
#if 0
		printf ("cam_send_ccb() = %d\n", rc);
#endif
		rc = -1;	/* EIO */
		goto out;
	}

	smc->scsi_req.status_byte = ccb->csio.scsi_status;

	smc->scsi_req.n_data_done =
		smc->scsi_req.n_data_avail - ccb->csio.resid;

	bcopy (&ccb->csio.sense_data, &smc->scsi_req.sense_data,
		sizeof ccb->csio.sense_data);

	smc->scsi_req.n_sense_data = 0;

	switch (ccb->csio.ccb_h.status & CAM_STATUS_MASK) {
	case CAM_REQ_CMP:
		/* completed */
		smc->scsi_req.completion_status = SMCSR_CS_GOOD;
		rc = 0;
		break;

	case CAM_SEL_TIMEOUT:
	case CAM_CMD_TIMEOUT:
		rc = -1;	/* NDMP9_TIMEOUT_ERR */

	case CAM_SCSI_STATUS_ERROR:
		if (ccb->csio.ccb_h.status & CAM_AUTOSNS_VALID) {
			smc->scsi_req.n_sense_data =
				ccb->csio.sense_len - ccb->csio.sense_resid;
		}
		smc->scsi_req.completion_status = SMCSR_CS_GOOD;
		rc = 0;
		break;

	default:
		rc = -1;	/* NDMP9_IO_ERR */
		break;
	}

  out:
#if 0
	printf ("scsi_status  = %d\n", ccb->csio.scsi_status);
	printf ("sense_len    = %d\n", ccb->csio.sense_len);
	printf ("sense_resid  = %d\n", ccb->csio.sense_resid);
	printf ("dxfer_len    = %d\n", ccb->csio.dxfer_len);
	printf ("resid        = %d\n", ccb->csio.resid);
#endif

	cam_freeccb (ccb);

	return rc;
}

#endif /* NDMOS_ID == NDMOS_ID_FREEBSD */
