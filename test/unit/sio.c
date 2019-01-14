/*
 * Copyright 2010-2019, Tarantool AUTHORS, please see AUTHORS file.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "unit.h"
#include "memory.h"
#include "fiber.h"
#include <sys/un.h>
#include <arpa/inet.h>
#include "sio.h"

static void
check_uri_to_addr(void)
{
	header();
	plan(18);
	struct sockaddr_storage storage;
	struct sockaddr *addr = (struct sockaddr *) &storage;
	struct sockaddr_un *un = (struct sockaddr_un *) addr;
	struct sockaddr_in *in = (struct sockaddr_in *) addr;
	isnt(0, sio_uri_to_addr("invalid uri", addr),
	     "invalid uri is detected");

	char long_path[1000];
	char *pos = long_path + sprintf(long_path, "unix/:/");
	memset(pos, 'a', 900);
	pos[900] = 0;
	isnt(0, sio_uri_to_addr(long_path, addr), "too long UNIX path");

	is(0, sio_uri_to_addr("unix/:/normal_path", addr), "UNIX");
	is(0, strcmp(un->sun_path, "/normal_path"), "UNIX path");
	is(AF_UNIX, un->sun_family, "UNIX family");

	is(0, sio_uri_to_addr("localhost:1234", addr), "localhost");
	is(AF_INET, in->sin_family, "localhost family");
	is(htonl(INADDR_LOOPBACK), in->sin_addr.s_addr, "localhost address");
	is(htons(1234), in->sin_port, "localhost port");

	is(0, sio_uri_to_addr("5678", addr), "'any'");
	is(AF_INET, in->sin_family, "'any' family");
	is(htonl(INADDR_ANY), in->sin_addr.s_addr, "'any' address");
	is(htons(5678), in->sin_port, "'any' port");

	is(0, sio_uri_to_addr("192.168.0.1:9101", addr), "IP");
	is(AF_INET, in->sin_family, "IP family");
	is(inet_addr("192.168.0.1"), in->sin_addr.s_addr, "IP address");
	is(htons(9101), in->sin_port, "IP port");

	isnt(0, sio_uri_to_addr("192.168.0.300:1112", addr), "invalid IP");

	check_plan();
	footer();
}

int
main(void)
{
	memory_init();
	fiber_init(fiber_c_invoke);

	header();
	plan(1);
	check_uri_to_addr();
	int rc = check_plan();
	footer();

	fiber_free();
	memory_free();
	return rc;
}
