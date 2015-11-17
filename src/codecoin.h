// Copyright (c) 2014 Troy Benjegerdes, under AGPLv3
// Distributed under the Affero GNU General public license version 3
// file COPYING or http://www.gnu.org/licenses/agpl-3.0.html
#ifndef CODECOIN_codecoin_H
#define CODECOIN_codecoin_H

#include "bignum.h"
#include <inttypes.h>
#include <string>

/* TODO: make this a class */
extern const std::string strMessageMagic;
extern const CBigNum bnProofOfWorkLimit;
extern const CBigNum bnProofOfStakeLimit;
//extern const unsigned int nStakeMinAge;
//extern const unsigned int nStakeMaxAge;

#ifdef BRAND_solarcoin
#include "solarcoin.h"
#elif BRAND_catcoin
#include "catcoin.h"
#elif BRAND_kittycoin
#include "kittycoin.h"
#elif BRAND_uro
#include "uro.h"
#elif BRAND_grantcoin
#include "grantcoin.h"
#elif BRAND_grantstake
#include "grantstake.h"
#elif defined(BRAND_givecoin) || defined(BRAND_givestake)
#include "givecoin.h"
#elif defined(BRAND_hamburger)
#include "hamburger.h"
#else
#error "No BRAND_*COIN defined!!"
#endif

#ifndef BRAND_SYM
#define BRAND_SYM BRAND_CODE
#endif

/* main.h do we always CheckProofOfWork in CheckIndex */
/* Litecoin does not do so for performance reasons */
#if defined(BRAND_bluecoin) || defined(BRAND_uro) || defined(BRAND_givestake)
#define CHECKINDEX_SLOW
#endif

#endif
