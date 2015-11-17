// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2013-2014 The Catcoin developers
// Copyright (c) 2014 Troy Benjegerdes, under AGPLv3
// Distributed under the Affero GNU General public license version 3
// file COPYING or http://www.gnu.org/licenses/agpl-3.0.html

#include "catcoin.h"
#include "alert.h"
#include "checkpoints.h"
#include "db.h"
#include "txdb.h"
#include "net.h"
#include "init.h"
#include "ui_interface.h"
#include "checkqueue.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

using namespace std;
using namespace boost;

uint256 hashGenesisBlock = 0;			// TODO: objectize this for multicoin support
CBigNum bnProofOfWorkLimit(~uint256(0) >> 12);	// *coin: starting difficulty is 1 / 2^12

const string strMessageMagic = "Catcoin Signed Message:\n";
/* value, in percent of what difficulty value we'll accept for orphans */
const int ORPHAN_WORK_THRESHOLD = 1; // FIXME WAY TOO WIDE right now

// DNS seeds
// Each pair gives a source name and a seed name.
// The first name is used as information source for addrman.
// The second name should resolve to a list of seed addresses.
// FIXME use a single string and/or objectify this
/*
  Catcoin policy for getting on this list:
   1) have a catcoind with port 9933 open
   2) be avaible for #catcoin-dev people to contact you for debug and testing
   3) be willing to take a haircut on generates if we determine on irc and on
	  (a future) mailing list that we need to do it to fix the network
 */
const char *strMainNetDNSSeed[][2] = {
	{"thepeeps.net", "p2pool.thepeeps.net"},
	{"catstat.info", "seed.catstat.info"},
	{"catcoinwallets.com", "seed.catcoinwallets.com"},
	{"geekhash.org", "cat.geekhash.org"},
	{NULL, NULL}
};

const char *strTestNetDNSSeed[][2] = {
	{"catstat.info", "testnet-seed.catstat.info"},
	{"catcoinwallets.com", "seed.catcoinwallets.com"},
	{NULL, NULL}
};

int64_t GetBlockValue(CBlockIndex *block, int64_t nFees)
{
	int64_t nSubsidy = 50 * COIN;

	// Making some sort of 'promise' of 21 million coins is like Monsanto trying to patent
	// any roundup-resistant plant, or insisting on only running the 'Genuine IBM PC'
	// Sure you can try to do that, but weeds evolve resistance, China makes clones,
	// and copycatcoins print money faster than Zimbabwe after they got rid of the farmers.
	//
	// Sound currency will come from a robust community that values something in common.
	// like Cat pictures.  -- Farmer Troy
	//
	// FIXME: add Depurrage based on last year of Cat food prices in the blockchain
	// FIXME2: come up with a way to GET cat food prices in the blockchain
	// FIXME3: do this more elegantly while maintaining readability
	//
	// further economic analysis supporting this at:
	// http://cryptonomics.org/2014/01/15/the-marginal-cost-of-cryptocurrency/
	// http://www.ezonomics.com/videos/can_bitcoin_and_other_virtual_currencies_ever_replace_real_money

	return nSubsidy + nFees;
}

static const int64_t nTargetTimespan = 6 * 60 * 60; // 6 hours
static const int64_t nTargetSpacing = 10 * 60;
static const int64_t nInterval = nTargetTimespan / nTargetSpacing;

static const int64_t nTargetTimespanOld = 14 * 24 * 60 * 60; // two weeks
static const int64_t nIntervalOld = nTargetTimespanOld / nTargetSpacing;

//
// minimum amount of work that could possibly be required nTime after
// minimum work required was nBase
//
unsigned int ComputeMinWork(unsigned int nBase, int64_t nTime)
{
	// Testnet has min-difficulty blocks
	// after nTargetSpacing*2 time between blocks:
	if (fTestNet && nTime > nTargetSpacing*2)
		return bnProofOfWorkLimit.GetCompact();

	CBigNum bnResult;
	bnResult.SetCompact(nBase);
	while (nTime > 0 && bnResult < bnProofOfWorkLimit)
	{
	// should technically be 112/100 * 36 .. ~40
		bnResult *= 40;
		// and if we have long blocks, max 40 x, as well
		nTime -= nTargetTimespan*40;
	}
	if (bnResult > bnProofOfWorkLimit)
		bnResult = bnProofOfWorkLimit;
	return bnResult.GetCompact();
}

static int fork3Block = 27260; // FIXME move to top...
static int fork4Block = 27680; // Acceptblock needs this
// static int fork1min = 31919;
static int fork1min = 210000;	// kittycoin fork block

//Checks for 'hardcoded' block timestamps
bool AcceptBlockTimestamp(CValidationState &state, CBlockIndex* pindexPrev, const CBlockHeader *pblock)
{
	int64_t time_allow = -30;
	int64_t time_warn = MINIMUM_BLOCK_SPACING;
	int64_t delta = pblock->GetBlockTime() - pindexPrev->GetBlockTime();
	int nHeight = pindexPrev->nHeight + 1;

	if (nHeight > fork4Block){
		time_allow = 30;
	}
	
	if (delta < time_warn){
		printf("WARNING blocktime nHeight %d time_allow %" PRId64" time_warn %" PRId64" time delta %" PRId64"\n", nHeight, time_allow, time_warn, delta);
	}

	if (nHeight >= fork3Block) {
		if (delta <= time_allow) // see above, from first hard limit
			return state.Invalid(error("AcceptBlock(height=%d) : block time delta %" PRId64" too short", nHeight, delta));
	}
	if (nHeight >= fork1min) { /* don't forward these */
		if (delta <= MINIMUM_BLOCK_SPACING)
			return state.DoS(10, (error("AcceptBlock(height=%d) : block time delta %" PRId64" too short", nHeight, delta)));
	}
	return true;	
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
	int64_t nTargetTimespanLocal = 0;
	int64_t nIntervalLocal = 0;
	int forkBlock = 20290 - 1;
	int fork2Block = 21346;

	 // moved variable inits to the top where they belong
	 
	unsigned int nProofOfWorkLimit = bnProofOfWorkLimit.GetCompact();
	int64_t nActualTimespan;
	const CBlockIndex* pindexFirst = pindexLast;

	 int64_t error;	 
	 //int64_t diffcalc;
	double pGainUp=-0.005125;	// Theses values can be changed to tune the PID formula
	double iGainUp=-0.0225;	// Theses values can be changed to tune the PID formula
	double dGainUp=-0.0075;		// Theses values can be changed to tune the PID formula

	double pGainDn=-0.005125;	// Theses values can be changed to tune the PID formula
	double iGainDn=-0.0525;	// Theses values can be changed to tune the PID formula
	double dGainDn=-0.0075;		// Theses values can be changed to tune the PID formula

	double pCalc;
	double iCalc;
	double dCalc;
	double dResult;
	int64_t result;
	CBigNum bResult;
	CBigNum bnNew;
	int i;
	//CBigNum bLowLimit; // Limit for PID calc to never go below this
	
	if(fTestNet){
		forkBlock = -1;
		fork2Block = 36;
	}

	// Genesis block
	if (pindexLast == NULL)
		return nProofOfWorkLimit;

	// Starting from block 20,290 the network diff was set to 16
	// and the retarget interval was changed to 36
	if(pindexLast->nHeight < forkBlock && !fTestNet) 
	{
		nTargetTimespanLocal = nTargetTimespanOld;
		nIntervalLocal = nIntervalOld;
	} 
	else if(pindexLast->nHeight == forkBlock && !fTestNet) 
	{
		bnNew.SetCompact(0x1c0ffff0); // Difficulty 16
		return bnNew.GetCompact();
	} 
	else // Keep in for a resync
	{
		nTargetTimespanLocal = nTargetTimespan;
		nIntervalLocal = nInterval;
	}

	// after fork2Block we retarget every block   
	if(pindexLast->nHeight < fork2Block && !fTestNet)
	{
		// Only change once per interval
		if ((pindexLast->nHeight+1) % nIntervalLocal != 0 && !fTestNet)
		{
			// Special difficulty rule for testnet:
			if (fTestNet)
			{
				// If the new block's timestamp is more than 2* 10 minutes
				// then allow mining of a min-difficulty block.
				if (pblock->nTime > pindexLast->nTime + nTargetSpacing*2)
					return nProofOfWorkLimit;
				else
				{
					// Return the last non-special-min-difficulty-rules-block
					const CBlockIndex* pindex = pindexLast;
					while (pindex->pprev && pindex->nHeight % nIntervalLocal != 0 && pindex->nBits == nProofOfWorkLimit)
						pindex = pindex->pprev;
					return pindex->nBits;
				}
			}

			return pindexLast->nBits;
		}
	}


	if(pindexLast->nHeight < fork3Block && !fTestNet) // let it walk through 2nd fork stuff if below fork3Block, and ignore if on testnet
	{
	// Catcoin: This fixes an issue where a 51% attack can change difficulty at will.
	// Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
		int blockstogoback = nIntervalLocal-1;
		if ((pindexLast->nHeight+1) != nIntervalLocal)
			blockstogoback = nIntervalLocal;

		// Go back by what we want to be 14 days worth of blocks
		const CBlockIndex* pindexFirst = pindexLast;
		for (i = 0; pindexFirst && i < blockstogoback; i++)
			pindexFirst = pindexFirst->pprev;
		assert(pindexFirst);

		// Limit adjustment step
		int numerator = 4;
		int denominator = 1;
		if(pindexLast->nHeight >= fork2Block){
			numerator = 112;
			denominator = 100;
		}
		int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
		int64_t lowLimit = nTargetTimespanLocal*denominator/numerator;
		int64_t highLimit = nTargetTimespanLocal*numerator/denominator;
		printf("  nActualTimespan = %" PRId64"  before bounds\n", nActualTimespan);
		if (nActualTimespan < lowLimit)
			nActualTimespan = lowLimit;
		if (nActualTimespan > highLimit)
			nActualTimespan = highLimit;

		// Retarget
		bnNew.SetCompact(pindexLast->nBits);
		bnNew *= nActualTimespan;
		bnNew /= nTargetTimespanLocal;
	
		if (bnNew > bnProofOfWorkLimit)
			bnNew = bnProofOfWorkLimit;

		/// debug print
		if(fTestNet) printf("GetNextWorkRequired RETARGET\n");
		if(fTestNet) printf("nTargetTimespan = %" PRId64"    nActualTimespan = %" PRId64"\n", nTargetTimespanLocal, nActualTimespan);
		if(fTestNet) printf("Before: %08x  %s\n", pindexLast->nBits, CBigNum().SetCompact(pindexLast->nBits).getuint256().ToString().c_str());
		if(fTestNet) printf("After:	%08x  %s\n", bnNew.GetCompact(), bnNew.getuint256().ToString().c_str());
	}
/*
PID formula
Error = Actual Time - Desired time
P Calc = pGain * Error
I Calc = iGain * Error * (Desired Time / Actual Time) 
D Calc = dGain * (Error / Actual Time) * I Calc

New Diff = (Current Diff + P Calc + I Calc + D Calc)

If New diff < 0, then set static value of 0.0001 or so.
*/	

	int nMinSpacing = 30;
	if(pindexLast->nHeight >= fork1min || fTestNet)
		nMinSpacing = MINIMUM_BLOCK_SPACING;
	
	if(pindexLast->nHeight >= fork3Block || fTestNet)
	// Fork 3 to use a PID routine instead of the other 2 forks 
	{
		pindexFirst = pindexLast->pprev;	// Set previous block
		for(i=0;i<7;i++) pindexFirst = pindexFirst->pprev; // Set 4th previous block for 8 block filtering 
		nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();		// Get last X blocks time
		nActualTimespan = nActualTimespan / 8;	// Calculate average for last 8 blocks
		if(pindexLast->nHeight > fork4Block || fTestNet){
			if (nMinSpacing > nActualTimespan){
				printf("WARNING: SANITY CHECK FAILED: PID nActualTimespan %" PRId64" too small! increased to %d\n",
					nActualTimespan, nMinSpacing );
				nActualTimespan = nMinSpacing;
			}
		}
		bnNew.SetCompact(pindexLast->nBits);	// Get current difficulty
		i=0;					// Zero bit-shift counter
		while(bnNew>0)				// Loop while bnNew > 0
		{
			i++;				// Increment bit-shift counter
			bnNew = bnNew >> 1;		// shift bnNew lower by 1 bit
			if(i>256) bnNew = 0;		// overflow test, just to make sure that it never stays in this loop
		}
		bnNew.SetCompact(pindexLast->nBits);	// Get current difficulty again
		

		error = nActualTimespan - nTargetSpacing;	// Calculate the error to be fed into the PID Calculation
		if(error >= -450 && error <= 450) // Slower gains for when the average time is within 2.5 min and 7.5 min 
		{
			// Calculate P ... pGainUp defined at beginning of routine
			pCalc = pGainUp * (double)error;
			// Calculate I ... iGainUp defined at beginning of routine
			iCalc = iGainUp * (double)error * (double)((double)nTargetSpacing / (double)nActualTimespan);
			// Calculate D ... dGainUp defined at beginning of routine
			dCalc = dGainUp * ((double)error / (double)nActualTimespan) * iCalc;
		}
		else // Faster gains for block averages faster than 2.5 min and greater than 7.5 min 
		{
			// Calculate P ... pGainDn defined at beginning of routine
			pCalc = pGainDn * (double)error;
			// Calculate I ... iGainDn defined at beginning of routine
			iCalc = iGainDn * (double)error * (double)((double)nTargetSpacing / (double)nActualTimespan);
			// Calculate D ... dGainDn defined at beginning of routine
			dCalc = dGainDn * ((double)error / (double)nActualTimespan) * iCalc;
		}

		if(error > -10 && error < 10)
		{
			if(fTestNet) printf("Within dead zone. No change!  error: %" PRId64"\n", error);
			return(bnNew.GetCompact());
		}		
		
		dResult = pCalc + iCalc + dCalc;	// Sum the PID calculations
		
		result = (int64_t)(dResult * 65536);	// Adjust for scrypt calcuation
		// Bring the result within max range to avoid overflow condition 
		while(result >	8388607) result = result / 2; 
		bResult = result;			// Set the bignum value
		if(i>24) bResult = bResult << (i - 24);	// bit-shift integer value of result to be subtracted from current diff

		//if(fTestNet)
		printf("pCalc: %f, iCalc: %f, dCalc: %f, Result: %" PRId64" (%f)\n", pCalc, iCalc, dCalc, result, dResult);
		//if(fTestNet) // TODO: make this key on a 'debugPID' or something
		printf("PID Actual Time: %" PRId64", error: %" PRId64"\n", nActualTimespan, error); 
		if(fTestNet)
			printf("Result: %08x %s\n",bResult.GetCompact(), bResult.getuint256().ToString().c_str()); 
		if(fTestNet)
			printf("Before: %08x %s\n",bnNew.GetCompact(), bnNew.getuint256().ToString().c_str()); 
		bnNew = bnNew - bResult;	// Subtract the result to set the current diff
		
		// Make sure that diff is not set too low, ever
		if (bnNew.GetCompact() > 0x1e0fffff) bnNew.SetCompact(0x1e0fffff);
		if(fTestNet) 
			printf("After:  %08x %s\n",bnNew.GetCompact(), bnNew.getuint256().ToString().c_str()); 
		
	} // End Fork 3 to use a PID routine instead of the other 2 forks routine

	return bnNew.GetCompact();
}


const char *pchCatMain	= "\xfc\xc1\xb7\xdc";
const char *pchCatTest	= "\xfd\xcb\xb8\xdd";

unsigned char pchMessageStart[4];

bool LoadBlockIndex()
{
	if (fTestNet)
	{	/* add 1 to litecoin values (3 to bitcoins) */
		pchMessageStart[0] = pchCatTest[0];
		pchMessageStart[1] = pchCatTest[1];
		pchMessageStart[2] = pchCatTest[2];
		pchMessageStart[3] = pchCatTest[3];
		hashGenesisBlock = uint256("0xec7987a2ab5225246c5cf9b8d93b4b75bcef383a4a65d5a265bc09ed54006188");
	} else {
		pchMessageStart[0] = pchCatMain[0];
		pchMessageStart[1] = pchCatMain[1];
		pchMessageStart[2] = pchCatMain[2];
		pchMessageStart[3] = pchCatMain[3];
		hashGenesisBlock = uint256("0xbc3b4ec43c4ebb2fef49e6240812549e61ffa623d9418608aa90eaad26c96296");
	}

	//
	// Load block index from databases
	//
	if (!fReindex && !LoadBlockIndexDB())
		return false;

	return true;
}


bool InitBlockIndex() {
	// Check whether we're already initialized
	if (pindexGenesisBlock != NULL)
		return true;

	// Use the provided setting for -txindex in the new database
	fTxIndex = GetBoolArg("-txindex", false);
	pblocktree->WriteFlag("txindex", fTxIndex);
	printf("Initializing databases...\n");

	// Only add the genesis block if not reindexing (in which case we reuse the one already on disk)
	if (!fReindex) {
		// Genesis Block:
		// CBlock(hash=12a765e31ffd4059bada, PoW=0000050c34a64b415b6b, ver=1, hashPrevBlock=00000000000000000000, hashMerkleRoot=97ddfbbae6, nTime=1317972665, nBits=1e0ffff0, nNonce=2084524493, vtx=1)
		//	 CTransaction(hash=97ddfbbae6, ver=1, vin.size=1, vout.size=1, nLockTime=0)
		//	   CTxIn(COutPoint(0000000000, -1), coinbase 04ffff001d0104404e592054696d65732030352f4f63742f32303131205374657665204a6f62732c204170706c65e280997320566973696f6e6172792c2044696573206174203536)
		//	   CTxOut(nValue=50.00000000, scriptPubKey=040184710fa689ad5023690c80f3a4)
		//	 vMerkleTree: 97ddfbbae6

		// Genesis block
		const char* pszTimestamp = "NY Times - December 23, 2013 - For Today's Babes, Toyland Is Digital";
		CTransaction txNew;
		txNew.vin.resize(1);
		txNew.vout.resize(1);
		txNew.vin[0].scriptSig = CScript() << 486604799 << CBigNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
		txNew.vout[0].nValue = 50 * COIN;
		txNew.vout[0].scriptPubKey = CScript() << ParseHex("040184710fa689ad5023690c80f3a49c8f13f8d45b8c857fbcbc8bc4a8e4d3eb4b10f4d4604fa08dce601aaf0f470216fe1b51850b4acf21b179c45070ac7b03a9") << OP_CHECKSIG;
		CBlock block;
		block.vtx.push_back(txNew);
		block.hashPrevBlock = 0;
		block.hashMerkleRoot = block.BuildMerkleTree();
		block.nVersion = 1;
		block.nTime    = 1387838302;
		block.nBits    = 0x1e0ffff0;
		block.nNonce   = 588050;

		if (fTestNet)
		{
			block.nTime    = 1387838303; //FIXME testnet0.1
			block.nNonce   = 608937;
		}

		//// debug print
		uint256 hash = block.GetHash();
		printf("%s\n", hash.ToString().c_str());
		printf("%s\n", hashGenesisBlock.ToString().c_str());
		printf("%s\n", block.hashMerkleRoot.ToString().c_str());
		assert(block.hashMerkleRoot == uint256("0x4007a33db5d9cdf2aab117335eb8431c8d13fb86e0214031fdaebe69a0f29cf7"));


		block.print();
		assert(hash == hashGenesisBlock);

		// Start new block file
		try {
			unsigned int nBlockSize = ::GetSerializeSize(block, SER_DISK, CLIENT_VERSION);
			CDiskBlockPos blockPos;
			CValidationState state;
			if (!FindBlockPos(state, blockPos, nBlockSize+8, 0, block.nTime))
				return error("LoadBlockIndex() : FindBlockPos failed");
			if (!block.WriteToDisk(blockPos))
				return error("LoadBlockIndex() : writing genesis block to disk failed");
			if (!block.AddToBlockIndex(state, blockPos))
				return error("LoadBlockIndex() : genesis block not accepted");
		} catch(std::runtime_error &e) {
			return error("LoadBlockIndex() : failed to initialize block database: %s", e.what());
		}
	}

	return true;
}

namespace Checkpoints
{
	// What makes a good checkpoint block?
	// + Is surrounded by blocks with reasonable timestamps
	//	 (no blocks before with a timestamp after, none after with
	//	  timestamp before)
	// + Contains no strange transactions
	// TODO put this in catcoin.cpp|.h
	static MapCheckpoints mapCheckpoints =
		boost::assign::map_list_of
		(    4, uint256("fe508d41b7dc2c3079e827d4230e6f7ddebca43c9afc721c1e6431f78d6ff1de"))
		(    5, uint256("7fc79021dbfa30255ade9bb8d898640516d9c771c3342a9b889ce380c52c6c1f"))
		( 5000, uint256("ec268a9cfe87adb4d10a147a544493406c80e00a3e6868641e520b184e7ddce3"))
		(10000, uint256("29c63023c3b8a36b59837734a9c16133a4ef08d9a0e95f639a830c56e415070d"))
		(20000, uint256("3a0c072b76a298dabffc4f825a084c0f86dc55fe58f9bf31cc7e21bbfb2ead52"))
		(22500, uint256("fd3c87eae2e9be72499978155844598a8675eff7a61c90f9aebcedc94e1b217f"))
		(22544, uint256("6dd1a90cc56cf4a46c8c47528c4861c255e86d5f97fcee53ce356174e15c3045"))
		(22554, uint256("b13e8b128989f9a9fc1a4c1e547330d0b34d3f60189c00391a116922fa4fcb8c"))
		(22600, uint256("9e2d7f2fdab36c3e2b6f0455470cd957c12172ad7877f7c8e414fd736469c8d2"))
		(22650, uint256("7afbd354496346819b8a214693af70e1431bfadbf68d49a688ae27539fc6b37e"))
		(22700, uint256("35154b803fa5700b69f8081aa6d7c798c1e7fd027971252598a18549092a1291"))
		(22750, uint256("67e6eca7d46c1a612b7638e7a503e6dbc7cca4da493f4267833a6f1c9a655a35"))
		(22800, uint256("49e84c3b5c261966c37c101ac7691886bd641a382f514c2221735088b1b2beea"))
		(22850, uint256("c44cec57381a97c3983df0ef1fcf150669dd1794943202d89b805f423a65516f"))
		(22900, uint256("44de4c262de678a23554dd06a6f57270815ea9d145f6c542ab2a8dfbd2ca242c"))
		(22950, uint256("cecc4ab30b39fc09bf85eb191e64c1660ab2206c5f80953694997ec5c2db5338"))
		(25890, uint256("4806f91100ae83904aa0113cc3acda8fe6ac422186243719a68b76c98e7487c2"))
		(29400,	uint256("6740c8907d9a13dfa1019142cc3b1e0abfe2fe8c832c5333df82a404d9a3e40e"))
		(30000, uint256("ff05303dc58caf2d102c85a0504ed16939c7840c91f5f0b37a5bf128e9afb73f"))
		(31830, uint256("9275b100cd5e540177c285c8801a63e644e7611a60a49b50831f70df6e5ea825"))
		(32040, uint256("d5c7bdecfd330721f489ea59930204345d3fc05dd5df0d2f09c6c53c8c0352b6"))
		(35000, uint256("8c5b56e660e47b398395fd01fd721b115fe523da400d23c82120c6fd37636423"))
		(40000, uint256("b8a6e8aaf4f92d4b521bd022de3008884eba51ff2a5c79e0269d65a03d109283"))
		(41000, uint256("88f114a60cb0841735df03cecc3c5662ffbdac184c7344d30cef4f98f5b61ed3"))
		(42000, uint256("4a538c3557ab865d74327e38837b5aac63aaebdc4718c2ee7b8101bcdd241eb6"))
		(43000, uint256("d2428f19de225b56853090fd548d1d7dd2d3d180b989c785eddb615e60f94209"))
		(44000, uint256("587b814e0a113eaf52b94e4920362f4c076d7dc942a4f8c5c4900f2d94adbc26"))
//		(33000, uint256("0x"))

		;
	const CCheckpointData data = {
		&mapCheckpoints,
		1434870875, 	// * UNIX timestamp of last checkpoint block
		106400,		// * total number of transactions between genesis and last checkpoint
					//	 (the tx=... number in the SetBestChain debug.log lines)
		1000.0		// * estimated number of transactions per day after checkpoint
	};
}
