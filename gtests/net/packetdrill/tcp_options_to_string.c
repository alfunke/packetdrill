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
 * Implementation for generating human-readable representations of TCP options.
 */

#include "tcp_options_to_string.h"

#include "tcp_options_iterator.h"

static int tcp_fast_open_option_to_string(FILE *s, struct tcp_option *option)
{
	assert(option->kind == TCPOPT_FASTOPEN);
	if (option->length < TCPOLEN_FASTOPEN_BASE) {
		return STATUS_ERR;
	}

	fputs("FO", s);
	int cookie_bytes = option->length - TCPOLEN_FASTOPEN_BASE;
	assert(cookie_bytes >= 0);
	assert(cookie_bytes <= MAX_TCP_FAST_OPEN_COOKIE_BYTES);
	if (cookie_bytes > 0) {
		fputs(" ", s);
	}
	int i;
	for (i = 0; i < cookie_bytes; ++i)
		fprintf(s, "%02x", option->data.fast_open.cookie[i]);
	return STATUS_OK;
}

/* See if the given experimental option is a TFO option, and if so
 * then print the TFO option and return STATUS_OK. Otherwise, return
 * STATUS_ERR.
 */
static void tcp_exp_fast_open_option_to_string(FILE *s, struct tcp_option *option)
{
	int cookie_bytes, i;

	assert(option->kind == TCPOPT_EXP);
	assert(option->length >= TCPOLEN_EXP_FASTOPEN_BASE);
	assert(option->data.exp.magic == htons(TCPOPT_FASTOPEN_MAGIC));

	fputs("EXP-FO", s);
	cookie_bytes = option->length - TCPOLEN_EXP_FASTOPEN_BASE;
	assert(cookie_bytes <= MAX_TCP_EXP_FAST_OPEN_COOKIE_BYTES);
	if (cookie_bytes > 0) {
		fputs(" ", s);
	}
	for (i = 0; i < cookie_bytes; ++i)
		fprintf(s, "%02x", option->data.exp.contents.fast_open.cookie[i]);
}

static int tcp_acc_ecn_option_to_string(FILE *s, struct tcp_option *option)
{
	unsigned int order;

	assert(option->kind == TCPOPT_ACC_ECN_0 ||
	       option->kind == TCPOPT_ACC_ECN_1);
	if ((option->length != ACC_ECN_ZERO_COUNTER_LEN) &&
	    (option->length != ACC_ECN_ONE_COUNTER_LEN) &&
	    (option->length != ACC_ECN_TWO_COUNTER_LEN) &&
	    (option->length != ACC_ECN_THREE_COUNTER_LEN)) {
		return STATUS_ERR;
	}
	switch (option->kind) {
	case TCPOPT_ACC_ECN_0:
		order = 0;
		break;
	case TCPOPT_ACC_ECN_1:
		order = 1;
		break;
	}
	switch (option->length) {
	case ACC_ECN_ZERO_COUNTER_LEN:
		fprintf(s, "AccECN%u", order);
		break;
	case ACC_ECN_ONE_COUNTER_LEN:
		fprintf(s, "AccECN%u EE%uB %u",
		        order, order,
		        order == 0 ? acc_ecn_get_ee0b(option) : acc_ecn_get_ee1b(option));
		break;
	case ACC_ECN_TWO_COUNTER_LEN:
		fprintf(s, "AccECN%u EE%uB %u ECEB %u",
		        order, order,
		        order == 0 ? acc_ecn_get_ee0b(option) : acc_ecn_get_ee1b(option),
		        acc_ecn_get_eceb(option));
		break;
	case ACC_ECN_THREE_COUNTER_LEN:
		fprintf(s, "AccECN%u EE%uB %u ECEB %u EE%uB %u",
		        order, order,
		        order == 0 ? acc_ecn_get_ee0b(option) : acc_ecn_get_ee1b(option),
		        acc_ecn_get_eceb(option),
		        1 - order,
		        order == 0 ? acc_ecn_get_ee1b(option) : acc_ecn_get_ee0b(option));
		break;
	}
	return STATUS_OK;
}

static int tcp_exp_acc_ecn_option_to_string(FILE *s, u16 magic, struct tcp_option *option)
{
	unsigned int order;

	assert(option->kind == TCPOPT_EXP);
	assert(magic == TCPOPT_ACC_ECN_0_MAGIC || magic == TCPOPT_ACC_ECN_1_MAGIC);
	if ((option->length != EXP_ACC_ECN_ZERO_COUNTER_LEN) &&
	    (option->length != EXP_ACC_ECN_ONE_COUNTER_LEN) &&
	    (option->length != EXP_ACC_ECN_TWO_COUNTER_LEN) &&
	    (option->length != EXP_ACC_ECN_THREE_COUNTER_LEN)) {
		return STATUS_ERR;
	}
	switch (magic) {
	case TCPOPT_ACC_ECN_0_MAGIC:
		order = 0;
		break;
	case TCPOPT_ACC_ECN_1_MAGIC:
		order = 1;
		break;
	}
	switch (option->length) {
	case EXP_ACC_ECN_ZERO_COUNTER_LEN:
		fprintf(s, "exp-AccECN%u", order);
		break;
	case EXP_ACC_ECN_ONE_COUNTER_LEN:
		fprintf(s, "exp-AccECN%u EE%uB %u",
		        order, order,
		        order == 0 ? exp_acc_ecn_get_ee0b(option) : exp_acc_ecn_get_ee1b(option));
		break;
	case EXP_ACC_ECN_TWO_COUNTER_LEN:
		fprintf(s, "exp-AccECN%u EE%uB %u ECEB %u",
		        order, order,
		        order == 0 ? exp_acc_ecn_get_ee0b(option) : exp_acc_ecn_get_ee1b(option),
		        exp_acc_ecn_get_eceb(option));
		break;
	case EXP_ACC_ECN_THREE_COUNTER_LEN:
		fprintf(s, "exp-AccECN%u EE%uB %u ECEB %u EE%uB %u",
		        order, order,
		        order == 0 ? exp_acc_ecn_get_ee0b(option) : exp_acc_ecn_get_ee1b(option),
		        exp_acc_ecn_get_eceb(option),
		        1 - order,
		        order == 0 ? exp_acc_ecn_get_ee1b(option) : exp_acc_ecn_get_ee0b(option));
		break;
	}
	return STATUS_OK;
}

static int tcp_exp_tarr_option_to_string(FILE *s, u16 magic, struct tcp_option *option)
{
	u16 data;

	assert(option->kind == TCPOPT_EXP);
	assert(magic == TCPOPT_TARR_MAGIC);
	if ((option->length != TCPOLEN_EXP_TARR_WITHOUT_RATE_LEN) &&
	    (option->length != TCPOLEN_EXP_TARR_WITH_RATE_LEN)) {
		return STATUS_ERR;
	}
	switch (option->length) {
	case TCPOLEN_EXP_TARR_WITHOUT_RATE_LEN:
		fputs("exp-tarr", s);
		break;
	case TCPOLEN_EXP_TARR_WITH_RATE_LEN:
		data = get_unaligned_be16(&option->data.exp.contents.tarr.data);
		fprintf(s, "exp-tarr %u", data >> 5);
	}
	return STATUS_OK;
}

int tcp_options_to_string(struct packet *packet,
				  char **ascii_string, char **error)
{
	struct tcp_options_iterator iter;
	struct tcp_option *option;
	size_t size = 0;
	FILE *s = open_memstream(ascii_string, &size);  /* output string */
	int result = STATUS_ERR;	/* return value */
	u16 magic;
	int index = 0;	/* number of options seen so far */

	for (option = tcp_options_begin(packet, &iter);
	     option != NULL; option = tcp_options_next(&iter, error)) {
		if (index > 0)
			fputc(',', s);

		switch (option->kind) {
		case TCPOPT_EOL:
			fputs("eol", s);
			break;

		case TCPOPT_NOP:
			fputs("nop", s);
			break;

		case TCPOPT_MAXSEG:
			fprintf(s, "mss %u", get_unaligned_be16(&option->data.mss.bytes));
			break;

		case TCPOPT_WINDOW:
			fprintf(s, "wscale %u",
				option->data.window_scale.shift_count);
			break;

		case TCPOPT_SACK_PERMITTED:
			fputs("sackOK", s);
			break;

		case TCPOPT_SACK:
			fprintf(s, "sack ");
			int num_blocks = 0;
			if (num_sack_blocks(option->length,
						    &num_blocks, error))
				goto out;
			int i = 0;
			for (i = 0; i < num_blocks; ++i) {
				if (i > 0)
					fputc(' ', s);
				fprintf(s, "%u:%u",
					get_unaligned_be32(&option->data.sack.block[i].left),
					get_unaligned_be32(&option->data.sack.block[i].right));
			}
			break;

		case TCPOPT_TIMESTAMP:
			fprintf(s, "TS val %u ecr %u",
				get_unaligned_be32(&option->data.time_stamp.val),
				get_unaligned_be32(&option->data.time_stamp.ecr));
			break;

		case TCPOPT_FASTOPEN:
			if (tcp_fast_open_option_to_string(s, option)) {
				asprintf(error, "invalid length: %u",
					 option->length);
				goto out;
			}
			break;

		case TCPOPT_ACC_ECN_0:
		case TCPOPT_ACC_ECN_1:
			if (tcp_acc_ecn_option_to_string(s, option)) {
				asprintf(error, "AccECN invalid length: %u",
				         option->length);
				goto out;
			}
			break;

		case TCPOPT_EXP:
			if (option->length < MIN_EXP_OPTION_LEN) {
				asprintf(error,
				         "experimental option too short: %u",
				         option->length);
				goto out;
			}
			magic = get_unaligned_be16(&option->data.exp.magic);
			switch (magic) {
			case TCPOPT_FASTOPEN_MAGIC:
				tcp_exp_fast_open_option_to_string(s, option);
				break;
			case TCPOPT_ACC_ECN_0_MAGIC:
			case TCPOPT_ACC_ECN_1_MAGIC:
				if (tcp_exp_acc_ecn_option_to_string(s, magic, option)) {
					asprintf(error, "exp-AccECN invalid length: %u",
						 option->length);
					goto out;
				}
				break;
			case TCPOPT_TARR_MAGIC:
				if (tcp_exp_tarr_option_to_string(s, magic, option)) {
					asprintf(error, "exp-tarr invalid length: %u",
						 option->length);
					goto out;
				}
				break;
			default:
				asprintf(error,
				         "unexpected experimental option magic: %u",
				         magic);
				goto out;
			}
			break;

		default:
			asprintf(error, "unexpected TCP option kind: %u",
				 option->kind);
			goto out;
		}
		++index;
	}
	if (*error != NULL)  /* bogus TCP options prevented iteration */
		goto out;

	result = STATUS_OK;

out:
	fclose(s);
	return result;

}
