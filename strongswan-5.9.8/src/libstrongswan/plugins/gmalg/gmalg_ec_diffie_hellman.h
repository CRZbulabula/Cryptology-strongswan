/*
 * Copyright (C) 2008 Tobias Brunner
 * HSR Hochschule fuer Technik Rapperswil
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

/**
 * @defgroup gmalg_ec_diffie_hellman gmalg_ec_diffie_hellman
 * @{ @ingroup gmalg_p
 */

#ifndef GMALG_EC_DIFFIE_HELLMAN_H_
#define GMALG_EC_DIFFIE_HELLMAN_H_

typedef struct gmalg_ec_key_exchange_t gmalg_ec_key_exchange_t;

#include <library.h>

/**
 * Implementation of the EC Diffie-Hellman algorithm using OpenSSL.
 */
struct gmalg_ec_key_exchange_t {

	/**
	 * Implements key_exchange_t interface.
	 */
	key_exchange_t dh;
};

/**
 * Creates a new gmalg_ec_key_exchange_t object.
 *
 * @param group			EC Diffie Hellman group number to use
 * @return				gmalg_ec_key_exchange_t object, NULL if not supported
 */
gmalg_ec_key_exchange_t *gmalg_ec_diffie_hellman_create(key_exchange_method_t group);

#endif /** GMALG_EC_DIFFIE_HELLMAN_H_ @}*/
