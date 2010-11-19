/* vim:ts=8:sts=8:sw=4:noai:noexpandtab
 *
 * unit tests for portable function to enumerate network names.
 *
 * Copyright (c) 2010 Miru Limited.
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

#ifndef _WIN32
#	include <sys/socket.h>
#	include <netdb.h>
#else
#	include <ws2tcpip.h>
#endif

#include <glib.h>
#include <check.h>


/* mock state */

#ifndef _WIN32
#	define COMPARE_GETNETENT
#endif

#define GETNETBYNAME_DEBUG
#include "getnetbyname.c"

static
void
mock_setup (void)
{
}

static
void
mock_teardown (void)
{
}

/* target:
 *	struct netent*
 *	pgm_getnetbyname (
 *		const char*	name
 *	)
 */

START_TEST (test_getnetbyname_pass_001)
{
	const char loopback[] = "loopback";

	fail_if (NULL == pgm_getnetbyname (loopback));
}
END_TEST

START_TEST (test_getnetbyname_fail_001)
{
	fail_unless (NULL == pgm_getnetbyname (NULL));
}
END_TEST

START_TEST (test_getnetbyname_fail_002)
{
	const char unknown[] = "qwertyuiop";

	fail_unless (NULL == pgm_getnetbyname (unknown));
}
END_TEST

/* target:
 *	struct pgm_netent_t*
 *	_pgm_compat_getnetent (void)
 */

START_TEST (test_getnetent_pass_001)
{
	int i = 1;
	struct pgm_netent_t* ne;
#ifdef COMPARE_GETNETENT
	struct netent* nne;
#endif

	_pgm_compat_setnetent();
#ifdef COMPARE_GETNETENT
	setnetent (0);
#endif
	while (ne = _pgm_compat_getnetent()) {
		char buffer[1024];
		char **p;

#ifdef COMPARE_GETNETENT
		nne = getnetent();
		if (NULL == nne)
			g_warning ("native ne = (null");
#endif

/* official network name */
		fail_if (NULL == ne->n_name);
		g_debug ("%-6dn_name = %s", i++, ne->n_name);
#ifdef COMPARE_GETNETENT
		if (NULL != nne)
			fail_unless (0 == strcmp (ne->n_name, nne->n_name));
#endif

/* alias list */
		fail_if (NULL == ne->n_aliases);
		p = ne->n_aliases;
		if (*p) {
			strcpy (buffer, *p++);
			while (*p) {
				strcat (buffer, ", ");
				strcat (buffer, *p++);
			}
		} else
			strcpy (buffer, "(nil)");
		g_debug ("      n_aliases = %s", buffer);
#ifdef COMPARE_GETNETENT
		if (NULL != nne) {
			char nbuffer[1024];

			fail_if (NULL == nne->n_aliases);
			p = nne->n_aliases;
			if (*p) {
				strcpy (nbuffer, *p++);
				while (*p) {
					strcat (nbuffer, ", ");
					strcat (nbuffer, *p++);
				}
			} else
				strcpy (nbuffer, "(nil)");
			fail_unless (0 == strcmp (buffer, nbuffer));
		}
#endif

/* net address type */
		fail_unless (AF_INET == ne->n_addrtype || AF_INET6 == ne->n_addrtype);
		if (AF_INET == ne->n_addrtype) {
			struct sockaddr_in sin;
			g_debug ("      n_addrtype = AF_INET");
#ifdef COMPARE_GETNETENT
			fail_unless (ne->n_addrtype == nne->n_addrtype);
#endif
			fail_unless (sizeof (struct in_addr) == ne->n_length);
			g_debug ("      n_length = %d", ne->n_length);
/* network number */
			memset (&sin, 0, sizeof (sin));
			sin.sin_family = ne->n_addrtype;
			sin.sin_addr.s_addr = ((struct in_addr*)ne->n_net)->s_addr;
			fail_unless (0 == getnameinfo ((struct sockaddr*)&sin, sizeof (sin),
						       buffer, sizeof (buffer),
						       NULL, 0,
						       NI_NUMERICHOST));
			g_debug ("      n_net = %s", buffer);
#ifdef COMPARE_GETNETENT
			fail_unless (0 == memcmp (ne->n_net, &nne->n_net, sizeof (struct in_addr)));
#endif
		} else {
			struct sockaddr_in6 sin6;
			g_debug ("      n_addrtype = AF_INET6");
#ifdef COMPARE_GETNETENT
			if (ne->n_addrtype != nne->n_addrtype)
				g_warning ("native address type not AF_INET6");
#endif
			fail_unless (sizeof (struct in6_addr) == ne->n_length);
			g_debug ("      n_length = %d", ne->n_length);
			memset (&sin6, 0, sizeof (sin6));
			sin6.sin6_family = ne->n_addrtype;
			sin6.sin6_addr = *(struct in6_addr*)ne->n_net;
			fail_unless (0 == getnameinfo ((struct sockaddr*)&sin6, sizeof (sin6),
						       buffer, sizeof (buffer),
						       NULL, 0,
						       NI_NUMERICHOST));
			g_debug ("      n_net = %s", buffer);
		}
	}
	_pgm_compat_endnetent();
#ifdef COMPARE_GETNETENT
	endnetent();
#endif
}
END_TEST


static
Suite*
make_test_suite (void)
{
	Suite* s;

	s = suite_create (__FILE__);
	TCase* tc_getnetbyname = tcase_create ("getnetbyname");
	suite_add_tcase (s, tc_getnetbyname);
	tcase_add_checked_fixture (tc_getnetbyname, mock_setup, mock_teardown);
	tcase_add_test (tc_getnetbyname, test_getnetbyname_pass_001);
	tcase_add_test (tc_getnetbyname, test_getnetbyname_fail_001);
	tcase_add_test (tc_getnetbyname, test_getnetbyname_fail_002);
	tcase_add_test (tc_getnetbyname, test_getnetent_pass_001);
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
#ifdef _WIN32
	WORD wVersionRequested = MAKEWORD (2, 2);
	WSADATA wsaData;
	g_assert (0 == WSAStartup (wVersionRequested, &wsaData));
	g_assert (LOBYTE (wsaData.wVersion) == 2 && HIBYTE (wsaData.wVersion) == 2);
#endif
	pgm_messages_init();
	SRunner* sr = srunner_create (make_master_suite ());
	srunner_add_suite (sr, make_test_suite ());
	srunner_run_all (sr, CK_ENV);
	int number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);
	pgm_messages_shutdown();
#ifdef _WIN32
	WSACleanup();
#endif
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* eof */
