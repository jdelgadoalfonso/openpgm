/* vim:ts=8:sts=4:sw=4:noai:noexpandtab
 * 
 * basic receive window.
 *
 * Copyright (c) 2006-2008 Miru Limited.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __PGM_RXW_H__
#define __PGM_RXW_H__

#include <glib.h>

#ifndef __PGM_TIMER_H__
#   include <pgm/timer.h>
#endif

#ifndef __PGM_MSGV_H__
#   include <pgm/msgv.h>
#endif

#ifndef __PGM_PACKET_H__
#   include <pgm/packet.h>
#endif

#ifndef __PGM_ERR_H__
#   include <pgm/err.h>
#endif

#ifndef __PGM_SKBUFF_H__
#	include <pgm/skbuff.h>
#endif


G_BEGIN_DECLS

typedef enum
{
    PGM_PKT_BACK_OFF_STATE,	    /* PGM protocol recovery states */
    PGM_PKT_WAIT_NCF_STATE,
    PGM_PKT_WAIT_DATA_STATE,

    PGM_PKT_HAVE_DATA_STATE,	    /* data received waiting to commit to application layer */

    PGM_PKT_HAVE_PARITY_STATE,	    /* contains parity information not original data */
    PGM_PKT_COMMIT_DATA_STATE,	    /* packet data at application layer */
    PGM_PKT_PARITY_DATA_STATE,	    /* packet available for parity calculation */
    PGM_PKT_LOST_DATA_STATE,	    /* if recovery fails, but packet has not yet been commited */

    PGM_PKT_ERROR_STATE
} pgm_pkt_state_e;

typedef enum
{
    PGM_RXW_OK = 0,
    PGM_RXW_CREATED_PLACEHOLDER,
    PGM_RXW_FILLED_PLACEHOLDER,
    PGM_RXW_ADVANCED_WINDOW,
    PGM_RXW_NOT_IN_TXW,
    PGM_RXW_WINDOW_UNDEFINED,
    PGM_RXW_DUPLICATE,
    PGM_RXW_APDU_LOST,
    PGM_RXW_MALFORMED_APDU,
    PGM_RXW_UNKNOWN,

    PGM_RXW_NEW_APDU = 0x100,
    PGM_RXW_CONSUMED_SKB = 0x200,
    PGM_RXW_RETURNS_MASK = 0xff
} pgm_rxw_returns_e;

const char* pgm_rxw_state_string (pgm_pkt_state_e);
const char* pgm_rxw_returns_string (pgm_rxw_returns_e);

/* callback for commiting contiguous pgm packets */
typedef int (*pgm_rxw_commitfn_t)(guint32, gpointer, guint, gpointer);

struct pgm_rxw_packet_t {
	pgm_time_t	t0;
	pgm_time_t	nak_rb_expiry;
	pgm_time_t	nak_rpt_expiry;
	pgm_time_t	nak_rdata_expiry;

        pgm_pkt_state_e state;

	guint8		nak_transmit_count;
        guint8          ncf_retry_count;
        guint8          data_retry_count;
};

typedef struct pgm_rxw_packet_t pgm_rxw_packet_t;

struct pgm_rxw_t {
	struct {				/* GPtrArray */
		gpointer*	pdata;
		guint		len;
		guint		alloc;
	} pdata;
	const void*	identifier;
	pgm_sock_err_t	pgm_sock_err;

	GSList		waiting_link;
	gboolean	is_waiting;
	GSList		commit_link;

        GQueue          backoff_queue;
        GQueue          wait_ncf_queue;
        GQueue          wait_data_queue;
/* window context counters */
	guint32		lost_count;		/* failed to repair */
	guint32		fragment_count;		/* incomplete apdu */
	guint32		parity_count;		/* parity for repairs */
	guint32		committed_count;	/* but still in window */
	guint32		parity_data_count;	/* to re-construct missing packets */

        guint16         max_tpdu;               /* maximum packet size */
	guint32		tg_size;		/* transmission group size for parity recovery */
	guint		tg_sqn_shift;

        guint32         lead, trail;
        guint32         rxw_trail, rxw_trail_init;
	guint32		commit_lead;
	guint32		commit_trail;
        gboolean        is_rxw_constrained;
        gboolean        is_window_defined;

	guint32		min_fill_time;
	guint32		max_fill_time;
	guint32		min_nak_transmit_count;
	guint32		max_nak_transmit_count;

/* runtime context counters */
	guint32		cumulative_losses;
	guint32		ack_cumulative_losses;
	guint32		bytes_delivered;
	guint32		msgs_delivered;
};

typedef struct pgm_rxw_t pgm_rxw_t;


pgm_rxw_t* pgm_rxw_init (const void*, guint16, guint32, guint, guint);
int pgm_rxw_shutdown (pgm_rxw_t*);

int pgm_rxw_push (pgm_rxw_t*, struct pgm_sk_buff_t*, pgm_time_t);

gssize pgm_rxw_readv (pgm_rxw_t*, pgm_msgv_t**, guint, struct pgm_iovec**, guint, gboolean);

/* from state checking */
int pgm_rxw_mark_lost (pgm_rxw_t*, guint32);

/* from SPM */
int pgm_rxw_window_update (pgm_rxw_t*, guint32, guint32, guint32, guint32, pgm_time_t);

/* from NCF */
int pgm_rxw_ncf (pgm_rxw_t*, guint32, pgm_time_t, pgm_time_t);


/* type determined by garray.h */
static inline guint pgm_rxw_len (pgm_rxw_t* r)
{
    return r->pdata.len;
}

static inline guint32 pgm_rxw_sqns (pgm_rxw_t* r)
{
    return ( 1 + r->lead ) - r->trail;
}

static inline gboolean pgm_rxw_empty (pgm_rxw_t* r)
{
    return pgm_rxw_sqns (r) == 0;
}

static inline gboolean pgm_rxw_full (pgm_rxw_t* r)
{
    return pgm_rxw_len (r) == pgm_rxw_sqns (r);
}

int pgm_rxw_pkt_state_unlink (pgm_rxw_t*, struct pgm_sk_buff_t*);
int pgm_rxw_peek (pgm_rxw_t*, guint32, struct pgm_opt_fragment** ,gpointer*, guint16*, gboolean*);
int pgm_rxw_push_nth_parity (pgm_rxw_t*, struct pgm_sk_buff_t*, pgm_time_t);
int pgm_rxw_push_nth_repair (pgm_rxw_t*, struct pgm_sk_buff_t*, pgm_time_t);

int pgm_rxw_release_committed (pgm_rxw_t*);
int pgm_rxw_free_committed (pgm_rxw_t*);

G_END_DECLS

#endif /* __PGM_RXW_H__ */
