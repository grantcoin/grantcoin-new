// Copyright (c) 2014 Troy Benjegerdes, under AGPLv3
// Distributed under the Affero GNU General public license version 3
// file COPYING or http://www.gnu.org/licenses/agpl-3.0.html
#ifndef CODECOIN_hamburger_H
#define CODECOIN_hamburger_H

#include "util.h"

static const int RPC_PORT = 3123;
static const int P2P_PORT = 3234;
static const int RPC_PORT_TESTNET = 4123;
static const int P2P_PORT_TESTNET = 4234;


static const int64_t COIN = 100000000;
static const int64_t CENT = 1000000;

/** Dust Soft Limit, allowed with additional fee per output */
static const int64_t DUST_SOFT_LIMIT = 100000; // 0.001 GIVE
/** Dust Hard Limit, ignored as wallet inputs (mininput default) */
static const int64_t DUST_HARD_LIMIT = 1000;   // 0.00001 GIVE mininput
/** Minimum criteria for AllowFree */
static const int64_t MIN_FREE_PRIORITY = COIN * 576/250;
/** No amount larger than this (in catoshi) is valid */
static const int64_t MAX_MONEY = 500000000 * COIN;
inline bool MoneyRange(int64_t nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 100;

/** Minimum block time spacing (hard limit) **/
static const int64_t MINIMUM_BLOCK_SPACING = 15;	// Absolute minimum spacing

/** really only used in rpcmining.cpp **/
static const int RETARGET_INTERVAL = 15;

extern const unsigned int nStakeMinAge;
extern const unsigned int nStakeMaxAge;
extern const unsigned int nStakeTargetSpacing;
extern const unsigned int nMaxClockDrift;
//extern const unsigned int nMinTxOutAmount; // set to = CTransaction::nMinTxFee;

#define CUTOFF_POS_BLOCK nCutoff_Pos_Block
extern const int CUTOFF_POS_BLOCK;

#define BRAND "Hamburger"
#define BRAND_upper "Hamburger"
#define BRAND_lower "hamburger"
#define BRAND_domain "hamburger.org"
#define BRAND_CODE "HAM"

#define PPCOINSTAKE

#endif
