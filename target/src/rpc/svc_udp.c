/* svc_udp.c - server side for UDP/IP based RPC */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
modification history
--------------------
01p,05nov01,vvv  fixed compilation warnings
01o,18apr00,ham  fixed compilation warnings.
01n,15sep93,kdl  changed svcudp_create() to only take one parameter (SPR #2427).
01m,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01l,04jun92,wmd  added forward declarations required for gcc960 v2.0.
01k,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01j,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01i,07oct90,hjb   deleted declaration of mem_alloc() due to problems with
		  ISI cpp complaining about macro argument.  Added include
		  of "memLib.h" instead.
01h,19jul90,dnw  changed declaration of mem_alloc() from char* to void*
		 added coercions to (char*) where necessary
01g,10may90,dnw  changed to use rpcErrnoGet instead of errnoGet
01f,19apr90,hjb  de-linted.
01e,27oct89,hjb  upgraded to 4.0
01d,30may88,dnw  changed to v4 names.
01c,05apr88,gae  changed fprintf() to printErr().
01b,11nov87,jlf  added wrs copyright, title, mod history, etc.
01a,01nov87,rdc  first VxWorks version
*/

#ifndef lint
/* static char sccsid[] = "@(#)svc_udp.c 1.1 86/02/03 Copyr 1984 Sun Micro"; */
#endif

/*
 * svc_udp.c,
 * Server side for UDP/IP based RPC.  (Does some caching in the hopes of
 * achieving execute-at-most-once semantics.)
 *
 */

#include "rpc/rpctypes.h"
#include "netinet/in.h"
#include "sys/socket.h"
#include "errno.h"
#include "rpc/xdr.h"
#include "rpc/auth.h"
#include "rpc/clnt.h"
#include "rpc/rpc_msg.h"
#include "rpc/svc.h"
#include "vxWorks.h"
#include "memLib.h"
#include "sockLib.h"
#include "remLib.h"
#include "ioLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "rpcLib.h"

#ifdef BUFSIZ
#undef BUFSIZ
#endif
#define BUFSIZ 1024

#define rpc_buffer(xprt) ((xprt)->xp_p1)
#define MAX(a, b)     ((a > b) ? a : b)

LOCAL bool_t		svcudp_recv();
LOCAL bool_t		svcudp_reply();
LOCAL enum xprt_stat	svcudp_stat();
LOCAL bool_t		svcudp_getargs();
LOCAL bool_t		svcudp_freeargs();
LOCAL void		svcudp_destroy();
LOCAL int		cache_get();
LOCAL void		cache_set();

static struct xp_ops svcudp_op = {
	svcudp_recv,
	svcudp_stat,
	svcudp_getargs,
	svcudp_reply,
	svcudp_freeargs,
	svcudp_destroy
};

/*
 * kept in xprt->xp_p2
 */
struct svcudp_data {
	u_int   su_iosz;	/* byte size of send.recv buffer */
	u_long	su_xid;		/* transaction id */
	XDR	su_xdrs;	/* XDR handle */
	char	su_verfbody[MAX_AUTH_BYTES];	/* verifier body */
	char   *su_cache;	/* cached data, NULL if no cache - 4.0 */
};
#define	su_data(xprt)	((struct svcudp_data *)(xprt->xp_p2))

/*
 * Usage:
 *	xprt = svcudp_create(sock);
 *
 * If sock<0 then a socket is created, else sock is used.
 * If the socket, sock is not bound to a port then svcudp_create
 * binds it to an arbitrary port.  In any (successful) case,
 * xprt->xp_sock is the registered socket number and xprt->xp_port is the
 * associated port number.
 * Once *xprt is initialized, it is registered as a transporter;
 * see (svc.h, xprt_register).
 * The routines returns NULL if a problem occurred.
 */
SVCXPRT *
svcudp_bufcreate(sock, sendsz, recvsz)
	register int sock;
	u_int sendsz, recvsz;
{
	bool_t madesock = FALSE;
	register SVCXPRT *xprt;
	register struct svcudp_data *su;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);

	if (sock == RPC_ANYSOCK) {
		if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
			perror("svcudp_create: socket creation problem");
			return ((SVCXPRT *)NULL);
		}
		madesock = TRUE;
	}

	bzero ((char *) &addr, sizeof (addr));			/* 4.0 */
	addr.sin_family = AF_INET;
	if (bindresvport (sock, &addr))				/* 4.0 */
	    {							/* 4.0 */
	    addr.sin_port = 0;					/* 4.0 */
	    (void)bind(sock, (struct sockaddr *)&addr, len);	/* 4.0 */
	    }							/* 4.0 */

	if (getsockname(sock, (struct sockaddr *)&addr, &len) != 0) {
		perror("svcudp_create - cannot getsockname");
		if (madesock)
			(void)close(sock);
		return ((SVCXPRT *)NULL);
	}
	xprt = (SVCXPRT *)mem_alloc(sizeof(SVCXPRT));
	if (xprt == NULL) {
		printErr ("svcudp_create: out of memory\n");
		return (NULL);
	}
	su = (struct svcudp_data *)mem_alloc(sizeof(*su));
	if (su == NULL) {
		printErr ("svcudp_create: out of memory\n");
		return (NULL);
	}
	su->su_iosz = ((MAX(sendsz, recvsz) + 3) / 4) * 4;
	if ((rpc_buffer(xprt) = (char *) mem_alloc(su->su_iosz)) == NULL) {
		printErr ("svcudp_create: out of memory\n");
		return (NULL);
	}
	xdrmem_create(
	    &(su->su_xdrs), rpc_buffer(xprt), su->su_iosz, XDR_DECODE);
	su->su_cache = NULL;			/* 4.0 */
	xprt->xp_p2 = (caddr_t)su;
	xprt->xp_verf.oa_base = su->su_verfbody;
	xprt->xp_ops = &svcudp_op;
	xprt->xp_port = ntohs(addr.sin_port);
	xprt->xp_sock = sock;
	xprt_register(xprt);
	return (xprt);
}

/* ARGSUSED1 */
SVCXPRT *
svcudp_create(sock)
	int sock;
{

	return(svcudp_bufcreate(sock, UDPMSGSIZE, UDPMSGSIZE));
}

/* ARGSUSED0 */
LOCAL enum xprt_stat				/* 4.0 */
svcudp_stat(xprt)
	SVCXPRT *xprt;
{

	return (XPRT_IDLE);
}

LOCAL bool_t					/* 4.0 */
svcudp_recv(xprt, msg)
	register SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	register struct svcudp_data *su = su_data(xprt);
	register XDR *xdrs = &(su->su_xdrs);
	register int rlen;
	char *reply;					/* 4.0 */
	u_long replylen;				/* 4.0 */

    again:
	xprt->xp_addrlen = sizeof(struct sockaddr_in);
	rlen = recvfrom(xprt->xp_sock, rpc_buffer(xprt), (int) su->su_iosz,
	    0, (struct sockaddr *)&(xprt->xp_raddr), &(xprt->xp_addrlen));
	if (rlen == -1 && (rpcErrnoGet () == EINTR))
		goto again;
	if (rlen < 4*sizeof(u_long))
		return (FALSE);
	xdrs->x_op = XDR_DECODE;
	XDR_SETPOS(xdrs, 0);
	if (! xdr_callmsg(xdrs, msg))
		return (FALSE);
	su->su_xid = msg->rm_xid;

	if (su->su_cache != NULL)				/* 4.0 */
	    {							/* 4.0 */
	    if (cache_get (xprt, msg, &reply, &replylen))	/* 4.0 */
		{						/* 4.0 */
/* 4.0 */	(void) sendto (xprt->xp_sock, reply, (int) replylen, 0,
			       (struct sockaddr *) &xprt->xp_raddr, /* 4.0 */
			       xprt->xp_addrlen);		/* 4.0 */
		}						/* 4.0 */
	    }							/* 4.0 */

	return (TRUE);
}

LOCAL bool_t							/* 4.0 */
svcudp_reply(xprt, msg)
	register SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	register struct svcudp_data *su = su_data(xprt);
	register XDR *xdrs = &(su->su_xdrs);
	register int slen;
	register bool_t stat = FALSE;

	xdrs->x_op = XDR_ENCODE;
	XDR_SETPOS(xdrs, 0);
	msg->rm_xid = su->su_xid;
	if (xdr_replymsg(xdrs, msg)) {
		slen = (int)XDR_GETPOS(xdrs);
		if (sendto(xprt->xp_sock, rpc_buffer(xprt), slen, 0,
		    (struct sockaddr *)&(xprt->xp_raddr), xprt->xp_addrlen)
		    == slen)
		    {						/* 4.0 */
		    stat = TRUE;
		    if (su->su_cache && slen >= 0)		/* 4.0 */
			{					/* 4.0 */
			cache_set (xprt, (u_long) slen);	/* 4.0 */
			}					/* 4.0 */
		    }						/* 4.0 */
	}
	return (stat);
}

LOCAL bool_t						/* 4.0 */
svcudp_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{

	return ((*xdr_args)(&(su_data(xprt)->su_xdrs), args_ptr));
}

LOCAL bool_t						/* 4.0 */
svcudp_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	register XDR *xdrs = &(su_data(xprt)->su_xdrs);

	xdrs->x_op = XDR_FREE;
	return ((*xdr_args)(xdrs, args_ptr));
}

LOCAL void						/* 4.0 */
svcudp_destroy(xprt)
	register SVCXPRT *xprt;
{
	register struct svcudp_data *su = su_data(xprt);

	xprt_unregister(xprt);
	(void)close(xprt->xp_sock);
	XDR_DESTROY(&(su->su_xdrs));
	mem_free(rpc_buffer(xprt), su->su_iosz);
	mem_free((caddr_t)su, sizeof(struct svcudp_data));
	mem_free((caddr_t)xprt, sizeof(SVCXPRT));
}

/******************* 4.0 additions ***************************/
/*
 * Fifo cache for udp server:
 * Copies pointers to reply buffers into fifo cache.
 * Buffers are sent again if retransmissions are detected.
 */

#define SPARSENESS	4		/* 75 % sparse */

#define CACHE_PERROR(msg) 	(void) printErr ("%s\n", (msg))

#define ALLOC(type, size)	\
	(type *) mem_alloc ((unsigned) (sizeof (type) * (size)))

#define BZERO(addr, type, size)	 \
	bzero((char *) addr, sizeof(type) * (int) (size))

/*
 * An entry in the cache
 */
typedef struct cache_node *cache_ptr;
struct cache_node {
	/*
	 * Index into cache is xid, proc, vers, prog and address
	 */
	u_long cache_xid;
	u_long cache_proc;
	u_long cache_vers;
	u_long cache_prog;
	struct sockaddr_in cache_addr;
	/*
	 * The cached reply and length
	 */
	char * cache_reply;
	u_long cache_replylen;
	/*
 	 * Next node on the list, if there is a collision
	 */
	cache_ptr cache_next;
};



/*
 * The entire cache
 */
struct udp_cache {
	u_long uc_size;		/* size of cache */
	cache_ptr *uc_entries;	/* hash table of entries in cache */
	cache_ptr *uc_fifo;	/* fifo list of entries in cache */
	u_long uc_nextvictim;	/* points to next victim in fifo list */
	u_long uc_prog;		/* saved program number */
	u_long uc_vers;		/* saved version number */
	u_long uc_proc;		/* saved procedure number */
	struct sockaddr_in uc_addr; /* saved caller's address */
};


/*
 * the hashing function
 */
#define CACHE_LOC(transp, xid)	\
 (xid % (SPARSENESS*((struct udp_cache *) su_data(transp)->su_cache)->uc_size))


/*
 * Enable use of the cache.
 * Note: there is no disable.
 */
int
svcudp_enablecache(transp, size)
	SVCXPRT *transp;
	u_long size;
{
	struct svcudp_data *su = su_data(transp);
	struct udp_cache *uc;

	if (su->su_cache != NULL) {
		CACHE_PERROR("enablecache: cache already enabled");
		return(0);
	}
	uc = ALLOC(struct udp_cache, 1);
	if (uc == NULL) {
		CACHE_PERROR("enablecache: could not allocate cache");
		return(0);
	}
	uc->uc_size = size;
	uc->uc_nextvictim = 0;
	uc->uc_entries = ALLOC(cache_ptr, size * SPARSENESS);
	if (uc->uc_entries == NULL) {
		CACHE_PERROR("enablecache: could not allocate cache data");
		return(0);
	}
	BZERO(uc->uc_entries, cache_ptr, size * SPARSENESS);
	uc->uc_fifo = ALLOC(cache_ptr, size);
	if (uc->uc_fifo == NULL) {
		CACHE_PERROR("enablecache: could not allocate cache fifo");
		return(0);
	}
	BZERO(uc->uc_fifo, cache_ptr, size);
	su->su_cache = (char *) uc;
	return(1);
}


/*
 * Set an entry in the cache
 */
static void
cache_set(xprt, replylen)
	SVCXPRT *xprt;
	u_long replylen;
{
	register cache_ptr victim;
	register cache_ptr *vicp;
	register struct svcudp_data *su = su_data(xprt);
	struct udp_cache *uc = (struct udp_cache *) su->su_cache;
	u_int loc;
	char *newbuf;

	/*
 	 * Find space for the new entry, either by
	 * reusing an old entry, or by mallocing a new one
	 */
	victim = uc->uc_fifo[uc->uc_nextvictim];
	if (victim != NULL) {
		loc = CACHE_LOC(xprt, victim->cache_xid);
		for (vicp = &uc->uc_entries[loc];
		  *vicp != NULL && *vicp != victim;
		  vicp = &(*vicp)->cache_next)
				;
		if (*vicp == NULL) {
			CACHE_PERROR("cache_set: victim not found");
			return;
		}
		*vicp = victim->cache_next;	/* remote from cache */
		newbuf = victim->cache_reply;
	} else {
		victim = ALLOC(struct cache_node, 1);
		if (victim == NULL) {
			CACHE_PERROR("cache_set: victim alloc failed");
			return;
		}
		newbuf = (char *) mem_alloc(su->su_iosz);
		if (newbuf == NULL) {
			CACHE_PERROR("cache_set: could not allocate new rpc_buffer");
			return;
		}
	}

	/*
	 * Store it away
	 */
	victim->cache_replylen = replylen;
	victim->cache_reply = rpc_buffer(xprt);
	rpc_buffer(xprt) = newbuf;
	xdrmem_create(&(su->su_xdrs), rpc_buffer(xprt), su->su_iosz, XDR_ENCODE);
	victim->cache_xid = su->su_xid;
	victim->cache_proc = uc->uc_proc;
	victim->cache_vers = uc->uc_vers;
	victim->cache_prog = uc->uc_prog;
	victim->cache_addr = uc->uc_addr;
	loc = CACHE_LOC(xprt, victim->cache_xid);
	victim->cache_next = uc->uc_entries[loc];
	uc->uc_entries[loc] = victim;
	uc->uc_fifo[uc->uc_nextvictim++] = victim;
	uc->uc_nextvictim %= uc->uc_size;
}

/*
 * Try to get an entry from the cache
 * return 1 if found, 0 if not found
 */
static int
cache_get(xprt, msg, replyp, replylenp)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
	char **replyp;
	u_long *replylenp;
{
	u_int loc;
	register cache_ptr ent;
	register struct svcudp_data *su = su_data(xprt);
	register struct udp_cache *uc = (struct udp_cache *) su->su_cache;

#	define EQADDR(a1, a2)	(bcmp((char*)&a1, (char*)&a2, sizeof(a1)) == 0)

	loc = CACHE_LOC(xprt, su->su_xid);
	for (ent = uc->uc_entries[loc]; ent != NULL; ent = ent->cache_next) {
		if (ent->cache_xid == su->su_xid &&
		  ent->cache_proc == uc->uc_proc &&
		  ent->cache_vers == uc->uc_vers &&
		  ent->cache_prog == uc->uc_prog &&
		  EQADDR(ent->cache_addr, uc->uc_addr)) {
			*replyp = ent->cache_reply;
			*replylenp = ent->cache_replylen;
			return(1);
		}
	}
	/*
	 * Failed to find entry
	 * Remember a few things so we can do a set later
	 */
	uc->uc_proc = msg->rm_call.cb_proc;
	uc->uc_vers = msg->rm_call.cb_vers;
	uc->uc_prog = msg->rm_call.cb_prog;
	uc->uc_addr = xprt->xp_raddr;
	return(0);
}
/************************ 4.0 additions **********************************/