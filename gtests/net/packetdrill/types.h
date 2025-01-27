/*
 * Copyright 2013 Google Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
/*
 * Author: ncardwell@google.com (Neal Cardwell)
 *
 * Declarations for types used widely throughout this tool.
 */

#ifndef __TYPES_H__
#define __TYPES_H__

/* The files that include this file need to include it before
 * including stdio.h in order to ensure that the declaration of
 * asprintf is visible. So our .h files attempt to follow a
 * convention of including types.h first, before everything else.
 */
#define _GNU_SOURCE		/* for asprintf */
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#if defined(linux)
#include <linux/types.h>
#endif
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "assert.h"
#include "platforms.h"

/* We use some unconventional formatting here to avoid checkpatch.pl
 * warnings about having to use the __packed macro, which is typically
 * only available in the kernel.
 */
#ifndef __packed
#define __packed __attribute__ ((packed))
#endif
#ifndef __aligned
#define __aligned(x) __attribute__ ((aligned(x)))
#endif

/* Make sure we have the following constants on all platforms */
#ifndef IPPROTO_SCTP
#define IPPROTO_SCTP 132
#endif
#ifndef IPPROTO_UDPLITE
#define IPPROTO_UDPLITE 136
#endif

/* We use kernel-style names for standard integer types. */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

#if !defined(linux)
typedef u8 __u8;
typedef u16 __u16;
typedef u32 __u32;
typedef u64 __u64;

/* We also use kernel-style names for endian-specific unsigned types. */
typedef u16 __le16;
typedef u16 __be16;
typedef u32 __le32;
typedef u32 __be32;
typedef u64 __le64;
typedef u64 __be64;

typedef u16 __sum16;
typedef u32 __wsum;
#endif

typedef u8 bool;
enum bool_t {
	false = 0,
	true = 1,
};

#define ARRAY_SIZE(array_name)  (sizeof(array_name) / sizeof(array_name[0]))

/* Most functions in this codebase return one of these two values to let the
 * caller know whether there was a problem.
 */
enum status_t {
	STATUS_OK  = 0,
	STATUS_ERR = -1,
	STATUS_WARN = -2,	/* a non-fatal error or warning */
};

/* The directions in which a packet may flow. */
enum direction_t {
	DIRECTION_INVALID,
	DIRECTION_INBOUND,	/* packet coming into the kernel under test */
	DIRECTION_OUTBOUND,	/* packet leaving the kernel under test */
};

/* Return the opposite direction. */
static inline enum direction_t reverse_direction(enum direction_t direction)
{
	if (direction == DIRECTION_INBOUND)
		return DIRECTION_OUTBOUND;
	else if (direction == DIRECTION_OUTBOUND)
		return DIRECTION_INBOUND;
	else
		assert(!"bad direction");
}

/* How to treat the TOS byte of a packet. */
enum tos_chk_t {
	TOS_CHECK_NONE,
	TOS_CHECK_ECN,
	TOS_CHECK_ECN_ECT01,  /* for outbound packet, either ECT0/ECT1 is OK */
	TOS_CHECK_TOS,
	TOS_CHECK_DSCP,
	TOS_CHECK_DSCP_ECN_ECT01
};

struct tos_spec {
	enum tos_chk_t check;
	u8 value;
};

#define TTL_CHECK_NONE 255

struct ip_info {
	struct tos_spec tos;
	u32 flow_label;
	u8 ttl;
};

static inline struct ip_info ip_info_new() {
	struct ip_info info;

	info.tos.check = TOS_CHECK_NONE;
	info.tos.value = 0;
	info.flow_label = 0;
	info.ttl = 0;

	return info;
}

/* Type to handle <integer>! for specifying absolute vs adjusted values.
 * The <integer>! means absolute integer. Just a plain <integer> is relative.
 */
struct abs_integer {
	s64 integer;
	bool absolute;
};

/* Type to handle ... or <integer> for specifying an integer or to ignore one. */
struct ignore_integer {
	s64 integer;
	bool ignore;
};

/* Length of output buffer for inet_ntop, plus prefix length (e.g. "/128"). */
#define ADDR_STR_LEN ((INET_ADDRSTRLEN + INET6_ADDRSTRLEN)+5)

/* Flavors of IP versions we support. */
enum ip_version_t {
	/* Native IPv4, with AF_INET sockets and IPv4 addresses. */
	IP_VERSION_4		= 0,

	/* IPv4-Mapped IPv6 addresses: (see RFC 4291 sec. 2.5.5.2) we
	 * use AF_INET6 sockets but all connect(), bind(), and
	 * accept() calls are for IPv4 addresses mapped into IPv6
	 * address space. So all interface addresses and packets on
	 * the wire are IPv4.
	 */
	IP_VERSION_4_MAPPED_6	= 1,

	/* Native IPv6, with AF_INET6 sockets and IPv6 addresses. */
	IP_VERSION_6		= 2,
};

extern struct in_addr in4addr_any;

/* Comparing IPv4 addresses for equality in C, which has no == on structs. */
static inline bool is_equal_ipv4(struct in_addr a, struct in_addr b)
{
	return a.s_addr == b.s_addr;
}

/* For comparing ports, for consistency with is_equal_ipv4. */
static inline bool is_equal_port(u16 a, u16 b)
{
	return a == b;
}

/* Convert microseconds to a floating-point seconds value. */
static inline double usecs_to_secs(s64 usecs)
{
	return ((double)usecs) / 1.0e6;
}

/* Convert a timeval to microseconds. */
static inline s64 timeval_to_usecs(const struct timeval *tv)
{
	return ((s64)tv->tv_sec) * 1000000LL + (s64)tv->tv_usec;
}

/* Return a malloc-allocated hex dump of the given buffer of the given length */
extern void hex_dump(const u8 *buffer, int bytes, char **hex);

static inline bool is_valid_u6(s64 x)
{
	return (x >= 0) && (x <= 0x3f);
}

static inline bool is_valid_u8(s64 x)
{
	return (x >= 0) && (x <= UCHAR_MAX);
}

static inline bool is_valid_u11(s64 x)
{
	return (x >= 0) && (x <= 0x7ff);
}

static inline bool is_valid_u16(s64 x)
{
	return (x >= 0) && (x <= USHRT_MAX);
}

static inline bool is_valid_u20(s64 x)
{
	return (x >= 0) && (x <= 0xfffff);
}

static inline bool is_valid_u24(s64 x)
{
	return (x >= 0) && (x <= 0xffffff);
}

static inline bool is_valid_u32(s64 x)
{
	return (x >= 0) && (x <= UINT_MAX);
}

static inline bool is_valid_s8(s64 x)
{
	return (x >= CHAR_MIN) && (x <= CHAR_MAX);
}

static inline bool is_valid_s16(s64 x)
{
	return (x >= SHRT_MIN) && (x <= SHRT_MAX);
}

static inline bool is_valid_s32(s64 x)
{
	return (x >= INT_MIN) && (x <= INT_MAX);
}

static inline s64 max(s64 a, s64 b)
{
	return (a > b) ? a : b;
}

static inline s64 min(s64 a, s64 b)
{
	return (a < b) ? a : b;
}

#endif /* __TYPES_H__ */
