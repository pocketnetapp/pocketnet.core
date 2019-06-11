//------------------------------------------------------------------------
// Functions for POCKETNET logic
//------------------------------------------------------------------------
#ifndef POCKETNET_H
#define POCKETNET_H

#include <chainparams.h>
#include <primitives/transaction.h>
#include <core_io.h>
#include <sstream>

// Antibot limits
enum Limit {
    threshold_reputation,
    threshold_reputation_score,
    threshold_reputation_complains,
    threshold_reputation_blocking,
    threshold_balance,
    trial_post_limit,
    trial_post_edit_limit,
    trial_score_limit,
    trial_complain_limit,
    full_post_limit,
    full_post_edit_limit,
    full_score_limit,
    full_complain_limit,
    change_info_timeout,
    edit_post_timeout,
    max_user_size,
    max_post_size,
    bad_reputation
};

void FillLimits(const CChainParams& params);

// Get actual limit for current height
int64_t GetActualLimit(Limit type, int height = 0);


// Pocketnet transaction types
#define OR_SCORE "7570766f74655368617265"
#define OR_COMPLAIN "636f6d706c61696e5368617265"
#define OR_POST "7368617265"
#define OR_POSTEDIT "736861726565646974"
#define OR_SUBSCRIBE "737562736372696265"
#define OR_SUBSCRIBEPRIVATE "73756273637269626550726976617465"
#define OR_UNSUBSCRIBE "756e737562736372696265"
#define OR_USERINFO "75736572496e666f"
#define OR_BLOCKING "626c6f636b696e67"
#define OR_UNBLOCKING "756e626c6f636b696e67"


// Check transaction type is pocketnet
bool IsPocketTX(const CTxOut& out);
bool IsPocketTX(const CTransaction& tx);
bool IsPocketTX(const CTransactionRef& tx);


// Transaction type convert to reindexer table name
bool ConvertOPToTableName(std::string op, std::string& ri_table);

#endif // POCKETNET_H