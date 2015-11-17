// Copyright (c) 2014 Troy Benjegerdes, under AGPLv3
// Distributed under the Affero GNU General public license version 3
// file COPYING or http://www.gnu.org/licenses/agpl-3.0.html
#ifndef CODECOIN_solarcoin_H
#define CODECOIN_solarcoin_H

#include "util.h"

static const int RPC_PORT = 36347;
static const int RPC_PORT_TESTNET = 26347;
static const int P2P_PORT = 36348;
static const int P2P_PORT_TESTNET = 26348;

static const uint64_t COIN = 100000000;
static const int64_t CENT = 1000000;

static const int RETARGET_INTERVAL = 480; /* 24 hours */

/** Dust Soft Limit, allowed with additional fee per output */
static const int64_t DUST_SOFT_LIMIT = 100000; // 0.001 CAT
/** Dust Hard Limit, ignored as wallet inputs (mininput default) */
static const int64_t DUST_HARD_LIMIT = 1000;   // 0.00001 CAT mininput
/** No amount larger than this (in satoshi) is valid */
static const int64_t MAX_MONEY = 10000000 * COIN;
inline bool MoneyRange(int64_t nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }

/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 20; /* probably to small */

/** Minimum block time spacing (see also: AcceptBlockTimestamp) **/
static const int64_t MINIMUM_BLOCK_SPACING = 1;	// Absolute minimum spacing:

#define BRAND "Uro"
#define BRAND_upper "Uro"
#define BRAND_lower "uro"

#endif
