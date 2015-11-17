// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2013 SolarCoin developers
// Copyright (c) 2014 Troy Benjegerdes, under AGPLv3
// Distributed under the Affero GNU General public license version 3
// file COPYING or http://www.gnu.org/licenses/agpl-3.0.html

#include "solarcoin.h"
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

const string strMessageMagic = "SolarCoin Signed Message:\n";

// DNS seeds
// Each pair gives a source name and a seed name.
// The first name is used as information source for addrman.
// The second name should resolve to a list of seed addresses.
// FIXME use a single string and/or objectify this
const char *strMainNetDNSSeed[][2] = {
	{"catstat.info", "seed.catstat.info"},
	{"solarcoin.org", "seed.solarcoin.org"},
	{NULL, NULL}
};

const char *strTestNetDNSSeed[][2] = {
	{"catstat.info", "testnet-seed.catstat.info"},
	{"solarcoin.org", "seed.solarcoin.org"},
	{NULL, NULL}
};

int64_t GetBlockValue(CBlockIndex *block, int64_t nFees)
{
    int64_t nSubsidy = 100 * COIN; 
	if(block->nHeight < 99) {nSubsidy = 1000000000 * COIN;}
	nSubsidy >>= (block->nHeight / 525600);
    return nSubsidy + nFees;
}

static const int64_t nTargetTimespan_Version1 = 24 * 60 * 60; // SolarCoin: 24 Hours
static const int64_t nTargetSpacing = 60 ; // SolarCoin: 1 Minute Blocks
static const int64_t nInterval_Version1 = nTargetTimespan_Version1 / nTargetSpacing; // SolarCoin: 1440 blocks

//this is used in computeminwork but no longer in getnextwork
static const int64_t nHeight_Version2 = 208440;
static const int64_t nInterval_Version2 = 15;
static const int64_t nTargetTimespan_Version2 = nInterval_Version2 * nTargetSpacing; // 15 minutes

//block to apply patch
static const int64_t DiffChangeBlock = 200000;

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
        nTime -= nTargetTimespan_Version2*4;
    }
    if (bnResult > bnProofOfWorkLimit)
        bnResult = bnProofOfWorkLimit;
    return bnResult.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
    unsigned int nProofOfWorkLimit = bnProofOfWorkLimit.GetCompact();
    int nHeight = pindexLast->nHeight + 1;

    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;
        
    int64_t nInterval;
    int64_t nTargetTimespan;
    
    nInterval = nInterval_Version1;
    nTargetTimespan = nTargetTimespan_Version1;
    
    
    if (nHeight >= DiffChangeBlock) {
        nTargetTimespan = 60; //1 min
        nInterval = nTargetTimespan / nTargetSpacing; //60/60 = 1 block
        //target timespan remains the same, 1 min
    }
        
    // Only change once per interval
    if ((pindexLast->nHeight+1) % nInterval != 0)
    {
        // Special difficulty rule for testnet:
        if (fTestNet)
        {
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->nTime > pindexLast->nTime + nTargetSpacing*2)
                return nProofOfWorkLimit;
            
            /* 5/30 - temporarily removing to test difficulty retargeting
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % nInterval != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            } */
        }

        return pindexLast->nBits;
    }

    // Litecoin: This fixes an issue where a 51% attack can change difficulty at will.
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
    
    if (nHeight >= DiffChangeBlock) //courtesy RealSolid and WDC
    {
        // amplitude filter - thanks to daft27 for this code
        nActualTimespan = nTargetTimespan + (nActualTimespan - nTargetTimespan)/8;
        if (nActualTimespan < (nTargetTimespan - (nTargetTimespan/4)) ) nActualTimespan = (nTargetTimespan - (nTargetTimespan/4));
        if (nActualTimespan > (nTargetTimespan + (nTargetTimespan/2)) ) nActualTimespan = (nTargetTimespan + (nTargetTimespan/2));
    }

    // Retarget
    CBigNum bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;

    if (bnNew > bnProofOfWorkLimit)
        bnNew = bnProofOfWorkLimit;

    /// debug print
    printf("GetNextWorkRequired RETARGET\n");
    printf("nTargetTimespan = %" PRId64"    nActualTimespan = %" PRId64"\n", nTargetTimespan, nActualTimespan);
    printf("Before: %08x  %s\n", pindexLast->nBits, CBigNum().SetCompact(pindexLast->nBits).getuint256().ToString().c_str());
    printf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.getuint256().ToString().c_str());

    return bnNew.GetCompact();
}

const char *pchSLRMain	= "\x04\xf1\x04\xfd";
const char *pchSLRTest	= "\xfd\xc0\x5a\xf2";

unsigned char pchMessageStart[4];

bool LoadBlockIndex()
{
	if (fTestNet)
	{	/* yes this could be more elegant */
		pchMessageStart[0] = pchSLRTest[0];
		pchMessageStart[1] = pchSLRTest[1];
		pchMessageStart[2] = pchSLRTest[2];
		pchMessageStart[3] = pchSLRTest[3];
		hashGenesisBlock = uint256("0x");
	} else {
		pchMessageStart[0] = pchSLRMain[0];
		pchMessageStart[1] = pchSLRMain[1];
		pchMessageStart[2] = pchSLRMain[2];
		pchMessageStart[3] = pchSLRMain[3];
		hashGenesisBlock = uint256("0xedcf32dbfd327fe7f546d3a175d91b05e955ec1224e087961acc9a2aa8f592ee");
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
		const char* pszTimestamp = "One Megawatt Hour";
		CTransaction txNew;
		txNew.vin.resize(1);
		txNew.vout.resize(1);
		txNew.vin[0].scriptSig = CScript() << 486604799 << CBigNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
		txNew.vout[0].nValue = 100 * COIN;
		txNew.vout[0].scriptPubKey = CScript() << ParseHex("040184710fa689ad5023690c80f3a49c8f13f8d45b8c857fbcbc8bc4a8e4d3eb4b10f4d4604fa08dce601aaf0f470216fe1b51850b4acf21b179c45070ac7b03a9") << OP_CHECKSIG;
		txNew.strTxComment = "text:SolarCoin genesis block";
		CBlock block;
		block.vtx.push_back(txNew);
		block.hashPrevBlock = 0;
		block.hashMerkleRoot = block.BuildMerkleTree();
		block.nVersion = 1;
		block.nTime    = 1384473600;
		block.nBits    = 0x1e0ffff0;
		block.nNonce   = 1397766;

		//// debug print
		uint256 hash = block.GetHash();
		printf("%s\n", hash.ToString().c_str());
		printf("%s\n", hashGenesisBlock.ToString().c_str());
		printf("%s\n", block.hashMerkleRoot.ToString().c_str());
		assert(block.hashMerkleRoot == uint256("0x33ecdb1985425f576c65e2c85d7983edc6207038a2910fefaf86cfb4e53185a3"));

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
	(	0, hashGenesisBlock)
	(	1, uint256("0xe8666c8715fafbfb095132deb1dd2af63fe14d3d7163715341d48feffab458cc"))
	(	25, uint256("0xe49cfc3e60515965380cbc3a1add5ab007e5bd2f226624cad9ff0f79eef680cc"))
	(	50, uint256("0x0b082428186ab2dc55403b2b3c9bd14f087590b204e05c09a656914285520b4d"))
	(	98, uint256("0xd27e483ae4d334cc65575bcc66d65f7a97913f31188662e2d3fe329675714128"))
	(	128, uint256("0xbce9c463a9e8b0d7b1c6df522fc80468fb47873c00b7b650f6b8046546c95dd0"))
	(	50000, uint256("0xf4fd47272011481dda8bc2a7b1305e39ff3429ecf3bcb9bd5af32fdef3945860"))
	(	100000, uint256("68d5027a570c605f6a0d24f8bad5c454769438eb4a237e93b4ee7a638eaa01b0"))
	(	150000, uint256("a9d3915cc6c9a18a6fe72429d496c985308c5335e60afe616fe6c8123c6e624f"))
//	(	200000, uint256("5f295d3a00a74641d9fda7bf538585456b30261d20bf559c4f4ca30a949062fe"))
//	(	230000, uint256("6ef474fb57b765ced46e28878be7e49648046438894288e983f1d58c8450dfdd"))
//	(	238000, uint256("c87ccce7cd13651126be87c4eff3d1429bc6e89b0cea417a807487f7cc81d768"))
	;

	const CCheckpointData data = {
		&mapCheckpoints,
		1398103556,     // * UNIX timestamp of last checkpoint block
		274778,		// * total number of transactions between genesis and last checkpoint
					//	 (the tx=... number in the SetBestChain debug.log lines)
		1000.0		// * estimated number of transactions per day after checkpoint
	};

}
