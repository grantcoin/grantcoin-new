// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2014 Uro Foundation
// Copyright (c) 2014 Troy Benjegerdes, under AGPLv3
// Distributed under the Affero GNU General public license version 3
// file COPYING or http://www.gnu.org/licenses/agpl-3.0.html

#include "uro.h"
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
CBigNum bnProofOfWorkLimit(~uint256(0) >> 20);	// *coin: starting difficulty is 1 / 2^20

const string strMessageMagic = "Uro Signed Message:\n";

// DNS seeds
// Each pair gives a source name and a seed name.
// The first name is used as information source for addrman.
// The second name should resolve to a list of seed addresses.
// FIXME use a single string and/or objectify this
const char *strMainNetDNSSeed[][2] = {
	{"catstat.info", "seed.catstat.info"},
	{"uro.io", "uro.io"},
	{NULL, NULL}
};

const char *strTestNetDNSSeed[][2] = {
	{"catstat.info", "testnet-seed.catstat.info"},
	{"uro.io", "seed.uro.io"},
	{NULL, NULL}
};

int64_t GetBlockValue(CBlockIndex *block, int64_t nFees)
{
	int64_t nSubsidy = 12 * COIN;

	if (block->nHeight > 83334)
	{
		nSubsidy = 0.6 * COIN;
		return nSubsidy + nFees;
	}

	return nSubsidy + nFees;
}

static const int64_t nTargetTimespan = 24 * 60 * 60; // Uro: 24 hours
static const int64_t nTargetSpacing = 3 * 60; // Uro: 3 minutes
static const int64_t nInterval = nTargetTimespan / nTargetSpacing;

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
		// Maximum 400% adjustment...
		bnResult *= 4;
		// ... in best-case exactly 4-times-normal target time
		nTime -= nTargetTimespan*4;
	}
	if (bnResult > bnProofOfWorkLimit)
		bnResult = bnProofOfWorkLimit;
	return bnResult.GetCompact();
}

bool AcceptBlockTimestamp(CValidationState &state, CBlockIndex* pindexPrev, const CBlockHeader *pblock)
{
	return true;	
}

unsigned int static GetNextWorkRequired_V1(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
	unsigned int nProofOfWorkLimit = bnProofOfWorkLimit.GetCompact();

	// Genesis block
	if (pindexLast == NULL)
		return nProofOfWorkLimit;

	// Only change once per interval
	if ((pindexLast->nHeight+1) % nInterval != 0)
	{
		// Special difficulty rule for testnet:
		if (fTestNet)
		{
			// If the new block's timestamp is more than 2* 2.5 minutes
			// then allow mining of a min-difficulty block.
			if (pblock->nTime > pindexLast->nTime + nTargetSpacing*2)
				return nProofOfWorkLimit;
			else
			{
				// Return the last non-special-min-difficulty-rules-block
				const CBlockIndex* pindex = pindexLast;
				while (pindex->pprev && pindex->nHeight % nInterval != 0 && pindex->nBits == nProofOfWorkLimit)
					pindex = pindex->pprev;
				return pindex->nBits;
			}
		}

		return pindexLast->nBits;
	}

	// Uro: This fixes an issue where a 51% attack can change difficulty at will.
	// Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
	int blockstogoback = nInterval-1;
	if ((pindexLast->nHeight+1) != nInterval)
		blockstogoback = nInterval;

	// Go back by what we want to be 14 days worth of blocks
	const CBlockIndex* pindexFirst = pindexLast;
	for (int i = 0; pindexFirst && i < blockstogoback; i++)
		pindexFirst = pindexFirst->pprev;
	assert(pindexFirst);

	// Limit adjustment step
	int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
	printf("  nActualTimespan = %" PRId64"  before bounds\n", nActualTimespan);
	if (nActualTimespan < nTargetTimespan/4)
		nActualTimespan = nTargetTimespan/4;
	if (nActualTimespan > nTargetTimespan*4)
		nActualTimespan = nTargetTimespan*4;

	// Retarget
	CBigNum bnNew;
	bnNew.SetCompact(pindexLast->nBits);
	bnNew *= nActualTimespan;
	bnNew /= nTargetTimespan;

	if (bnNew > bnProofOfWorkLimit)
		bnNew = bnProofOfWorkLimit;

#if 0
	/// debug print
	printf("GetNextWorkRequired RETARGET\n");
	printf("nTargetTimespan = %" PRId64"    nActualTimespan = %" PRId64"\n", nTargetTimespan, nActualTimespan);
	printf("Before: %08x  %s\n", pindexLast->nBits, CBigNum().SetCompact(pindexLast->nBits).getuint256().ToString().c_str());
	printf("After:	%08x  %s\n", bnNew.GetCompact(), bnNew.getuint256().ToString().c_str());
#endif

	return bnNew.GetCompact();
}


unsigned int static KimotoGravityWell(const CBlockIndex* pindexLast, const CBlockHeader *pblock, uint64_t TargetBlocksSpacingSeconds, uint64_t PastBlocksMin, uint64_t PastBlocksMax) {
	/* current difficulty formula, megacoin - kimoto gravity well */
	const CBlockIndex *BlockLastSolved = pindexLast;
	const CBlockIndex *BlockReading = pindexLast;
	const CBlockHeader *BlockCreating = pblock;
	BlockCreating = BlockCreating;
	uint64_t PastBlocksMass = 0;
	int64_t PastRateActualSeconds = 0;
	int64_t PastRateTargetSeconds = 0;
	double PastRateAdjustmentRatio = double(1);
	CBigNum PastDifficultyAverage;
	CBigNum PastDifficultyAveragePrev;
	double EventHorizonDeviation;
	double EventHorizonDeviationFast;
	double EventHorizonDeviationSlow;
	
	if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || (uint64_t)BlockLastSolved->nHeight < PastBlocksMin) { return bnProofOfWorkLimit.GetCompact(); }
	int64_t LatestBlockTime = BlockLastSolved->GetBlockTime();
	for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
		if (PastBlocksMax > 0 && i > PastBlocksMax) { break; }
		PastBlocksMass++;
		
		if (i == 1) { PastDifficultyAverage.SetCompact(BlockReading->nBits); }
		else { PastDifficultyAverage = ((CBigNum().SetCompact(BlockReading->nBits) - PastDifficultyAveragePrev) / i) + PastDifficultyAveragePrev; }
		PastDifficultyAveragePrev = PastDifficultyAverage;
		
		if (LatestBlockTime < BlockReading->GetBlockTime()) {
		       if ((BlockReading->nHeight > 1) || (fTestNet && (BlockReading->nHeight >= 10)))
			       LatestBlockTime = BlockReading->GetBlockTime();
	       }
	       PastRateActualSeconds		       = LatestBlockTime - BlockReading->GetBlockTime();
		PastRateTargetSeconds = TargetBlocksSpacingSeconds * PastBlocksMass;
		PastRateAdjustmentRatio = double(1);
		if ((BlockReading->nHeight > 1) || (fTestNet && (BlockReading->nHeight >= 10))) {
		       if (PastRateActualSeconds < 1) { PastRateActualSeconds = 1; }
	       } else {
		       if (PastRateActualSeconds < 0) { PastRateActualSeconds = 0; }
	       }
		if (PastRateActualSeconds != 0 && PastRateTargetSeconds != 0) {
		PastRateAdjustmentRatio = double(PastRateTargetSeconds) / double(PastRateActualSeconds);
		}
		EventHorizonDeviation = 1 + (0.7084 * pow((double(PastBlocksMass)/double(28.2)), -1.228));
		EventHorizonDeviationFast = EventHorizonDeviation;
		EventHorizonDeviationSlow = 1 / EventHorizonDeviation;
		
		if (PastBlocksMass >= PastBlocksMin) {
			if ((PastRateAdjustmentRatio <= EventHorizonDeviationSlow) || (PastRateAdjustmentRatio >= EventHorizonDeviationFast)) { assert(BlockReading); break; }
		}
		if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
		BlockReading = BlockReading->pprev;
	}
	
	CBigNum bnNew(PastDifficultyAverage);
	if (PastRateActualSeconds != 0 && PastRateTargetSeconds != 0) {
		bnNew *= PastRateActualSeconds;
		bnNew /= PastRateTargetSeconds;
	}
	if (bnNew > bnProofOfWorkLimit) { bnNew = bnProofOfWorkLimit; }
	printf("GetNextWorkRequired KGW RETARGET\n");
	printf("PastRateTargetSeconds = %" PRId64" PastRateActualSeconds = %" PRId64"\n", PastRateTargetSeconds, PastRateActualSeconds);
	printf("Before: %08x  %s\n", pindexLast->nBits, CBigNum().SetCompact(pindexLast->nBits).getuint256().ToString().c_str());
	printf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.getuint256().ToString().c_str());
	
	return bnNew.GetCompact();
}


unsigned int static GetNextWorkRequired_V2(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
	static const int64_t BlocksTargetSpacing = 3 * 60; // Uro: 3 minutes
	static const unsigned int TimeDaySeconds = 24 * 60 * 60; // Uro: 1 day
	int64_t PastSecondsMin = TimeDaySeconds * 0.025;
	int64_t PastSecondsMax = TimeDaySeconds * 7;
	if (pindexLast->nHeight + 1 >= 12760) {
		PastSecondsMin *= 2;
	}
	uint64_t PastBlocksMin = PastSecondsMin / BlocksTargetSpacing;
	uint64_t PastBlocksMax = PastSecondsMax / BlocksTargetSpacing;
	
	return KimotoGravityWell(pindexLast, pblock, BlocksTargetSpacing, PastBlocksMin, PastBlocksMax);
}


unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
        int DiffMode = 1;
        if (fTestNet) {
		DiffMode = 1;
        } else {
		if (pindexLast->nHeight + 1 >= 12760) {
			DiffMode = 2;
		}
        }
      
        if (DiffMode == 1) { 
		return GetNextWorkRequired_V1(pindexLast, pblock); 
	}

        else if (DiffMode == 2) { 
		return GetNextWorkRequired_V2(pindexLast, pblock); 
	}

}

const char *pchUROMain	= "\xfe\xc3\xb9\xde";
const char *pchUROTest	= "\xfe\xc4\xba\xde";

unsigned char pchMessageStart[4];

bool LoadBlockIndex()
{
	if (fTestNet)
	{	/* yes this could be more elegant */
		pchMessageStart[0] = pchUROTest[0];
		pchMessageStart[1] = pchUROTest[1];
		pchMessageStart[2] = pchUROTest[2];
		pchMessageStart[3] = pchUROTest[3];
		hashGenesisBlock = uint256("0x");
	} else {
		pchMessageStart[0] = pchUROMain[0];
		pchMessageStart[1] = pchUROMain[1];
		pchMessageStart[2] = pchUROMain[2];
		pchMessageStart[3] = pchUROMain[3];
		hashGenesisBlock = uint256("0x000001196bbe430b7e0cdce3504f5ddfda0d0313ea479526d79afbf4d090a880");
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
		// Genesis block
		const char* pszTimestamp = "URO";
		CTransaction txNew;
		txNew.vin.resize(1);
		txNew.vout.resize(1);
		txNew.vin[0].scriptSig = CScript() << 486604799 << CBigNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
		txNew.vout[0].nValue = 40 * COIN;
		txNew.vout[0].scriptPubKey = CScript() << ParseHex("040184710fa689ad5023690c80f3a49c8f13f8d45b8c857fbcbc8bc4a8e4d3eb4b10f4d4604fa08dce601aaf0f470216fe1b51850b4acf21b179c45070ac7b03a9") << OP_CHECKSIG;
		CBlock block;
		block.vtx.push_back(txNew);
		block.hashPrevBlock = 0;
		block.hashMerkleRoot = block.BuildMerkleTree();
		block.nVersion = 1;
		block.nTime    = 1398093006;
		block.nBits    = 0x1e0ffff0;
		block.nNonce   = 307242;

		if (fTestNet)
		{
			block.nTime    = 1398093006;
			block.nNonce   = 307242;
		}

		//// debug print
		uint256 hash = block.GetHash();
		printf("%s\n", hash.ToString().c_str());
		printf("%s\n", hashGenesisBlock.ToString().c_str());
		printf("%s\n", block.hashMerkleRoot.ToString().c_str());
		assert(block.hashMerkleRoot == uint256("0xcf112b0792eaf749de18d633d3545aecd7b1343d78e14a830a242a03a6c31339"));

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
			// TODO: add sync checkpoint
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
	(	0, hashGenesisBlock)
	;

	const CCheckpointData data = {
		&mapCheckpoints,
		1398103556,     // * UNIX timestamp of last checkpoint block
		274778,		// * total number of transactions between genesis and last checkpoint
					//	 (the tx=... number in the SetBestChain debug.log lines)
		1000.0		// * estimated number of transactions per day after checkpoint
	};

}
