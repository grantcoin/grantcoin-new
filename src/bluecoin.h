// Copyright (c) 2014 Troy Benjegerdes, under AGPLv3
// Distributed under the Affero GNU General public license version 3
// file COPYING or http://www.gnu.org/licenses/agpl-3.0.html
#ifndef CODECOIN_bluecoin_H
#define CODECOIN_bluecoin_H

#include "util.h"

static const int RPC_PORT = 27105;
static const int RPC_PORT_TESTNET = 37105;
static const int P2P_PORT = 27104;
static const int P2P_PORT_TESTNET = 37104;

static const int64_t COIN = 100000000;
static const int64_t CENT = 1000000;

/** Dust Soft Limit, allowed with additional fee per output */
static const int64_t DUST_SOFT_LIMIT = 100000; // 0.001 BLU
/** Dust Hard Limit, ignored as wallet inputs (mininput default) */
static const int64_t DUST_HARD_LIMIT = 1000;   // 0.00001 BLU mininput
/** No amount larger than this (in catoshi) is valid */
static const int64_t MAX_MONEY = 500000000 * COIN;
inline bool MoneyRange(int64_t nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 100;

static const int64 MAX_MONEY2 = 1000000000 * COIN;			// 1 bil
static const int64 CIRCULATION_MONEY = MAX_MONEY2;
static const double TAX_PERCENTAGE = 0.03;
static const int64 MIN_MINT_PROOF_OF_STAKE = 0.01 * COIN;	// 3% annual interest
//static const int64 MIN_TXOUT_AMOUNT = MIN_TX_FEE; this is replaced with CTransaction::nMinTxFee
static const int X11_CUTOFF_TIME = 1403395200;

static const int STAKE_TARGET_SPACING = 10 * 60; // 10-minute block spacing 
static const int STAKE_MIN_AGE = 60 * 60 * 24 * 2; // minimum age for coin age
static const int STAKE_MAX_AGE = 60 * 60 * 24 * 90; // stake age of full weight

static const uint256 hashGenesisBlockOfficial("0x00000e5fcb52c612b989715cb5e9cad4c071c76e8263980b45d34e17cff6f732");
static const uint256 hashGenesisBlockTestNet ("0x");
static const int64 nMaxClockDrift = 2 * 60 * 60;        // two hours

#define BRAND "GiveCoin"
#define BRAND_upper "GiveCoin"
#define BRAND_lower "givecoin"
#define BRAND_SYM "ß"
#define BRAND_CODE "BLU"


#endif
