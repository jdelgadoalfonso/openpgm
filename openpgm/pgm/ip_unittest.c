/* vim:ts=8:sts=8:sw=4:noai:noexpandtab
 *
 * unit tests for ip stack.
 *
 * Copyright (c) 2009-2010 Miru Limited.
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

#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef _WIN32
#	include <arpa/inet.h>
#	include <netinet/in.h>
#	include <sys/types.h>
#	include <sys/socket.h>
#else
#	include <ws2tcpip.h>
#	include <mswsock.h>
#endif
#include <glib.h>
#include <check.h>


/* getsockopt(3SOCKET)
 * level is the protocol number of the protocl that controls the option.
 */
#ifndef SOL_IP
#	define SOL_IP		IPPROTO_IP
#endif
#ifndef SOL_IPV6
#	define SOL_IPV6		IPPROTO_IPV6
#endif

/* mock state */

size_t
pgm_transport_pkt_offset2 (
        const bool                      can_fragment,
        const bool                      use_pgmcc
        )
{
        return 0;
}


#define PGM_COMPILATION
#include "impl/sockaddr.h"
#include "impl/indextoaddr.h"
#include "impl/ip.h"


/* target:
 *   testing platform capability to loop send multicast packets to a listening
 * receive socket.
 */

START_TEST (test_multicast_loop_pass_001)
{
	struct sockaddr_in addr;
	memset (&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr ("239.192.0.1");

	int recv_sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	fail_if (-1 == recv_sock, "socket failed");
	struct sockaddr_in recv_addr;
	memcpy (&recv_addr, &addr, sizeof(addr));
	recv_addr.sin_port = 7500;
	fail_unless (0 == bind (recv_sock, (struct sockaddr*)&recv_addr, pgm_sockaddr_len ((struct sockaddr*)&recv_addr)), "bind failed");
	struct group_req gr;
	memset (&gr, 0, sizeof(gr));
	((struct sockaddr*)&gr.gr_group)->sa_family = addr.sin_family;
	((struct sockaddr_in*)&gr.gr_group)->sin_addr.s_addr = addr.sin_addr.s_addr;
	fail_unless (0 == setsockopt (recv_sock, SOL_IP, MCAST_JOIN_GROUP, (const char*)&gr, sizeof(gr)), "setsockopt failed");
	fail_unless (0 == pgm_sockaddr_multicast_loop (recv_sock, AF_INET, FALSE), "multicast_loop failed");

	int send_sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	fail_if (-1 == send_sock, "socket failed");
	struct sockaddr_in send_addr;
	memcpy (&send_addr, &addr, sizeof(addr));
	fail_unless (0 == bind (send_sock, (struct sockaddr*)&send_addr, pgm_sockaddr_len ((struct sockaddr*)&send_addr)), "bind failed");
        struct sockaddr_in if_addr;
	fail_unless (TRUE == pgm_if_indextoaddr (0, AF_INET, 0, (struct sockaddr*)&if_addr, NULL), "if_indextoaddr failed");
	fail_unless (0 == pgm_sockaddr_multicast_if (send_sock, (struct sockaddr*)&if_addr, 0), "multicast_if failed");
	fail_unless (0 == pgm_sockaddr_multicast_loop (send_sock, AF_INET, TRUE), "multicast_loop failed");

	const char data[] = "apple pie";
	addr.sin_port = 7500;
	ssize_t bytes_sent = sendto (send_sock, data, sizeof(data), 0, (struct sockaddr*)&addr, pgm_sockaddr_len ((struct sockaddr*)&addr));
	if (-1 == bytes_sent)
		g_message ("sendto: %s", strerror (errno));
	fail_unless (sizeof(data) == bytes_sent, "sendto underrun");

	char recv_data[1024];
	ssize_t bytes_read = recv (recv_sock, recv_data, sizeof(recv_data), MSG_DONTWAIT);
	if (-1 == bytes_read)
		g_message ("sendto: %s", strerror (errno));
	fail_unless (sizeof(data) == bytes_read, "recv underrun");

	fail_unless (0 == close (recv_sock), "close failed");
	fail_unless (0 == close (send_sock), "close failed");
}
END_TEST

/* target:
 *   testing whether unicast bind accepts packets to multicast join on a
 * different port.
 */

START_TEST (test_port_bind_pass_001)
{
	struct sockaddr_in addr;
	memset (&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr ("239.192.0.1");

	int recv_sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	fail_if (-1 == recv_sock, "socket failed");
	struct sockaddr_in recv_addr;
	memcpy (&recv_addr, &addr, sizeof(addr));
	recv_addr.sin_port = 3056;
	fail_unless (0 == bind (recv_sock, (struct sockaddr*)&recv_addr, pgm_sockaddr_len ((struct sockaddr*)&recv_addr)), "bind failed");
	struct group_req gr;
	memset (&gr, 0, sizeof(gr));
	((struct sockaddr*)&gr.gr_group)->sa_family = addr.sin_family;
	((struct sockaddr_in*)&gr.gr_group)->sin_addr.s_addr = addr.sin_addr.s_addr;
	((struct sockaddr_in*)&gr.gr_group)->sin_port = 3055;
	fail_unless (0 == setsockopt (recv_sock, SOL_IP, MCAST_JOIN_GROUP, (const char*)&gr, sizeof(gr)), "setsockopt failed");
	fail_unless (0 == pgm_sockaddr_multicast_loop (recv_sock, AF_INET, FALSE), "multicast_loop failed");

	int send_sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	fail_if (-1 == send_sock, "socket failed");
	struct sockaddr_in send_addr;
	memcpy (&send_addr, &addr, sizeof(addr));
	fail_unless (0 == bind (send_sock, (struct sockaddr*)&send_addr, pgm_sockaddr_len ((struct sockaddr*)&send_addr)), "bind failed");
        struct sockaddr_in if_addr;
	fail_unless (TRUE == pgm_if_indextoaddr (0, AF_INET, 0, (struct sockaddr*)&if_addr, NULL), "if_indextoaddr failed");
	fail_unless (0 == pgm_sockaddr_multicast_if (send_sock, (struct sockaddr*)&if_addr, 0), "multicast_if failed");
	fail_unless (0 == pgm_sockaddr_multicast_loop (send_sock, AF_INET, TRUE), "multicast_loop failed");

	const char data[] = "apple pie";
	addr.sin_port = 3056;
	ssize_t bytes_sent = sendto (send_sock, data, sizeof(data), 0, (struct sockaddr*)&addr, pgm_sockaddr_len ((struct sockaddr*)&addr));
	if (-1 == bytes_sent)
		g_message ("sendto: %s", strerror (errno));
	fail_unless (sizeof(data) == bytes_sent, "sendto underrun");

	char recv_data[1024];
	ssize_t bytes_read = recv (recv_sock, recv_data, sizeof(recv_data), MSG_DONTWAIT);
	if (-1 == bytes_read)
		g_message ("recv: %s", strerror (errno));
	if (sizeof(data) != bytes_read)
		g_message ("recv returned %d bytes expected %d.", bytes_read, sizeof(data));
	fail_unless (sizeof(data) == bytes_read, "recv underrun");

	fail_unless (0 == close (recv_sock), "close failed");
	fail_unless (0 == close (send_sock), "close failed");
}
END_TEST

/* target:
 *   test setting hop limit, aka time-to-live.
 *
 *   NB: whilst convenient, we cannot use SOCK_RAW & IPPROTO_UDP on Solaris 10
 *   as it crashes the IP stack.
 */

START_TEST (test_hop_limit_pass_001)
{
	struct sockaddr_in addr;
	memset (&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr ("239.192.0.1");

	int recv_sock = socket (AF_INET, SOCK_RAW, 113);
	fail_if (-1 == recv_sock, "socket failed");
	struct sockaddr_in recv_addr;
	memcpy (&recv_addr, &addr, sizeof(addr));
	recv_addr.sin_port = 7500;
	fail_unless (0 == bind (recv_sock, (struct sockaddr*)&recv_addr, pgm_sockaddr_len ((struct sockaddr*)&recv_addr)), "bind failed");
	struct group_req gr;
	memset (&gr, 0, sizeof(gr));
	((struct sockaddr*)&gr.gr_group)->sa_family = addr.sin_family;
	((struct sockaddr_in*)&gr.gr_group)->sin_addr.s_addr = addr.sin_addr.s_addr;
	fail_unless (0 == setsockopt (recv_sock, SOL_IP, MCAST_JOIN_GROUP, (const char*)&gr, sizeof(gr)), "setsockopt failed");
	fail_unless (0 == pgm_sockaddr_multicast_loop (recv_sock, AF_INET, FALSE), "multicast_loop failed");
	fail_unless (0 == pgm_sockaddr_hdrincl (recv_sock, AF_INET, TRUE), "hdrincl failed");

	int send_sock = socket (AF_INET, SOCK_RAW, 113);
	fail_if (-1 == send_sock, "socket failed");
	struct sockaddr_in send_addr;
	memcpy (&send_addr, &addr, sizeof(addr));
	fail_unless (0 == bind (send_sock, (struct sockaddr*)&send_addr, pgm_sockaddr_len ((struct sockaddr*)&send_addr)), "bind failed");
        struct sockaddr_in if_addr;
	fail_unless (TRUE == pgm_if_indextoaddr (0, AF_INET, 0, (struct sockaddr*)&if_addr, NULL), "if_indextoaddr failed");
	fail_unless (0 == pgm_sockaddr_multicast_if (send_sock, (struct sockaddr*)&if_addr, 0), "multicast_if failed");
	fail_unless (0 == pgm_sockaddr_multicast_loop (send_sock, AF_INET, TRUE), "multicast_loop failed");
	fail_unless (0 == pgm_sockaddr_multicast_hops (send_sock, AF_INET, 16), "multicast_hops failed");

	const char data[] = "apple pie";
	addr.sin_port = 7500;
	ssize_t bytes_sent = sendto (send_sock, data, sizeof(data), 0, (struct sockaddr*)&addr, pgm_sockaddr_len ((struct sockaddr*)&addr));
	if (-1 == bytes_sent)
		g_message ("sendto: %s", strerror (errno));
	fail_unless (sizeof(data) == bytes_sent, "sendto underrun");

	char recv_data[1024];
	ssize_t bytes_read = recv (recv_sock, recv_data, sizeof(recv_data), MSG_DONTWAIT);
	if (-1 == bytes_read)
		g_message ("recv: %s", strerror (errno));
	const size_t pkt_len = sizeof(struct pgm_ip) + sizeof(data);
	if (pkt_len != bytes_read)
#ifndef _MSC_VER
		g_message ("recv returned %zd bytes expected %zu.", bytes_read, pkt_len);
#else
		g_message ("recv returned %ld bytes expected %lu.", (long)bytes_read, (unsigned long)pkt_len);
#endif
	fail_unless (pkt_len == bytes_read, "recv underrun");
	const struct pgm_ip* iphdr = (void*)recv_data;
	fail_unless (4 == iphdr->ip_v, "Incorrect IP version, found %u expecting 4.", iphdr->ip_v);
	fail_unless (16 == iphdr->ip_ttl, "hop count mismatch, found %u expecting 16.", iphdr->ip_ttl);

	fail_unless (0 == close (recv_sock), "close failed");
	fail_unless (0 == close (send_sock), "close failed");
}
END_TEST

/* target:
 *   router alert.
 */

START_TEST (test_router_alert_pass_001)
{
	struct sockaddr_in addr;
	memset (&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr ("239.192.0.1");

	int recv_sock = socket (AF_INET, SOCK_RAW, 113);
	fail_if (-1 == recv_sock, "socket failed");
	struct sockaddr_in recv_addr;
	memcpy (&recv_addr, &addr, sizeof(addr));
	recv_addr.sin_port = 7500;
	fail_unless (0 == bind (recv_sock, (struct sockaddr*)&recv_addr, pgm_sockaddr_len ((struct sockaddr*)&recv_addr)), "bind failed");
	struct group_req gr;
	memset (&gr, 0, sizeof(gr));
	((struct sockaddr*)&gr.gr_group)->sa_family = addr.sin_family;
	((struct sockaddr_in*)&gr.gr_group)->sin_addr.s_addr = addr.sin_addr.s_addr;
	fail_unless (0 == setsockopt (recv_sock, SOL_IP, MCAST_JOIN_GROUP, (const char*)&gr, sizeof(gr)), "setsockopt failed");
	fail_unless (0 == pgm_sockaddr_multicast_loop (recv_sock, AF_INET, FALSE), "multicast_loop failed");
	fail_unless (0 == pgm_sockaddr_hdrincl (recv_sock, AF_INET, TRUE), "hdrincl failed");

	int send_sock = socket (AF_INET, SOCK_RAW, 113);
	fail_if (-1 == send_sock, "socket failed");
	struct sockaddr_in send_addr;
	memcpy (&send_addr, &addr, sizeof(addr));
	fail_unless (0 == bind (send_sock, (struct sockaddr*)&send_addr, pgm_sockaddr_len ((struct sockaddr*)&send_addr)), "bind failed");
        struct sockaddr_in if_addr;
	fail_unless (TRUE == pgm_if_indextoaddr (0, AF_INET, 0, (struct sockaddr*)&if_addr, NULL), "if_indextoaddr failed");
	fail_unless (0 == pgm_sockaddr_multicast_if (send_sock, (struct sockaddr*)&if_addr, 0), "multicast_if failed");
	fail_unless (0 == pgm_sockaddr_multicast_loop (send_sock, AF_INET, TRUE), "multicast_loop failed");
	fail_unless (0 == pgm_sockaddr_router_alert (send_sock, AF_INET, TRUE), "router_alert failed");

	const char data[] = "apple pie";
	addr.sin_port = 7500;
	ssize_t bytes_sent = sendto (send_sock, data, sizeof(data), 0, (struct sockaddr*)&addr, pgm_sockaddr_len ((struct sockaddr*)&addr));
	if (-1 == bytes_sent)
		g_message ("sendto: %s", strerror (errno));
	fail_unless (sizeof(data) == bytes_sent, "sendto underrun");

	char recv_data[1024];
	ssize_t bytes_read = recv (recv_sock, recv_data, sizeof(recv_data), MSG_DONTWAIT);
	if (-1 == bytes_read)
		g_message ("recv: %s", strerror (errno));
	const size_t ra_iphdr_len = sizeof(uint32_t) + sizeof(struct pgm_ip);
	const size_t ra_pkt_len = ra_iphdr_len + sizeof(data);
	if (ra_pkt_len != bytes_read)
#ifndef _MSC_VER
		g_message ("recv returned %zd bytes expected %zu.", bytes_read, ra_pkt_len);
#else
		g_message ("recv returned %ld bytes expected %lu.", (long)bytes_read, (unsigned long)ra_pkt_len);
#endif
	fail_unless (ra_pkt_len == bytes_read, "recv underrun");
	const struct pgm_ip* iphdr = (void*)recv_data;
	fail_unless (4 == iphdr->ip_v, "Incorrect IP version, found %u expecting 4.", iphdr->ip_v);
	if (ra_iphdr_len != (iphdr->ip_hl << 2)) {
#ifndef _MSC_VER
		g_message ("IP header length mismatch, found %zu expecting %zu.",
			(size_t)(iphdr->ip_hl << 2), ra_iphdr_len);
#else
		g_message ("IP header length mismatch, found %lu expecting %lu.",
			(long)(iphdr->ip_hl << 2), (unsigned long)ra_iphdr_len);
#endif
	}
#ifndef _MSC_VER
	g_message ("IP header length = %zu", (size_t)(iphdr->ip_hl << 2));
#else
	g_message ("IP header length = %lu", (unsigned long)(iphdr->ip_hl << 2));
#endif
	const uint32_t* ipopt = (const void*)&recv_data[ iphdr->ip_hl << 2 ];
	const uint32_t ipopt_ra = ((uint32_t)PGM_IPOPT_RA << 24) | (0x04 << 16);
	const uint32_t router_alert = htonl(ipopt_ra);
	if (router_alert == *ipopt) {
		g_message ("IP option router alert found after IP header length.");
		ipopt += sizeof(uint32_t);
	} else {
		ipopt = (const void*)&recv_data[ sizeof(struct pgm_ip) ];
		fail_unless (router_alert == *ipopt, "IP router alert option not found.");
		g_message ("IP option router alert found before end of IP header length.");
	}
#ifndef _MSC_VER
	g_message ("Final IP header length = %zu", (size_t)((const char*)ipopt - (const char*)recv_data));
#else
	g_message ("Final IP header length = %lu", (unsigned long)((const char*)ipopt - (const char*)recv_data));
#endif

	fail_unless (0 == close (recv_sock), "close failed");
	fail_unless (0 == close (send_sock), "close failed");
}
END_TEST


static
Suite*
make_test_suite (void)
{
	Suite* s;

	s = suite_create (__FILE__);

	TCase* tc_multicast_loop = tcase_create ("multicast loop");
	suite_add_tcase (s, tc_multicast_loop);
	tcase_add_test (tc_multicast_loop, test_multicast_loop_pass_001);

	TCase* tc_port_bind = tcase_create ("port bind");
	suite_add_tcase (s, tc_port_bind);
	tcase_add_test (tc_port_bind, test_port_bind_pass_001);

	TCase* tc_hop_limit = tcase_create ("hop limit");
	suite_add_tcase (s, tc_hop_limit);
	tcase_add_test (tc_hop_limit, test_hop_limit_pass_001);

	TCase* tc_router_alert = tcase_create ("router alert");
	suite_add_tcase (s, tc_router_alert);
	tcase_add_test (tc_router_alert, test_router_alert_pass_001);
	return s;
}

static
Suite*
make_master_suite (void)
{
	Suite* s = suite_create ("Master");
	return s;
}

int
main (void)
{
#ifndef _WIN32
	if (0 != getuid()) {
		fprintf (stderr, "This test requires super-user privileges to run.\n");
		return EXIT_FAILURE;
	}
#endif

	SRunner* sr = srunner_create (make_master_suite ());
	srunner_add_suite (sr, make_test_suite ());
	srunner_run_all (sr, CK_ENV);
	int number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* eof */
