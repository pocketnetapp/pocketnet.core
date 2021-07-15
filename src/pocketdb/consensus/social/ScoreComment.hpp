// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SCORECOMMENT_HPP
#define POCKETCONSENSUS_SCORECOMMENT_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/dto/ScoreComment.hpp"
#include "pocketdb/consensus/Reputation.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  ScoreComment consensus base class
    *
    *******************************************************************************************************************/
    class ScoreCommentConsensus : public SocialBaseConsensus
    {
    public:
        ScoreCommentConsensus(int height) : SocialBaseConsensus(height) {}

    protected:
        virtual int64_t GetFullAccountScoresLimit() { return 600; }

        virtual int64_t GetTrialAccountScoresLimit() { return 300; }


        virtual int64_t GetScoresLimit(AccountMode mode)
        {
            return mode == AccountMode_Full ? GetFullAccountScoresLimit() : GetTrialAccountScoresLimit();
        }

        tuple<bool, SocialConsensusResult> ValidateModel(const PTransactionRef& tx) override
        {
            auto ptx = static_pointer_cast<ScoreComment>(tx);

            // Check registration
            vector<string> addresses = {*ptx->GetAddress()};
            if (!PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses))
                return make_tuple(false, SocialConsensusResult_NotRegistered);

            // Comment should be exists
            auto[lastContentOk, lastContent] = PocketDb::ConsensusRepoInst.GetLastContent(*ptx->GetCommentTxHash());
            if (!lastContentOk)
                return {false, SocialConsensusResult_NotFound};

            // Scores to deleted comments not allowed
            if (*lastContent->GetType() == PocketTxType::CONTENT_COMMENT_DELETE)
                return {false, SocialConsensusResult_NotFound};

            // Check score to self
            if (*ptx->GetAddress() == *lastContent->GetString1())
                return {false, SocialConsensusResult_SelfCommentScore};

            // Check Blocking
            if (auto[ok, result] = ValidateBlocking(*lastContent->GetString1(), ptx); !ok)
                return {false, result};

            // Check already scored content
            if (PocketDb::ConsensusRepoInst.ExistsScore(
                *ptx->GetAddress(), *ptx->GetCommentTxHash(), ACTION_SCORE_COMMENT, false))
                return {false, SocialConsensusResult_DoubleCommentScore};

            // Check OP_RETURN with Payload
            if (*ptx->GetOPRAddress() != *lastContent->GetString1() || *ptx->GetOPRValue() != *ptx->GetValue())
                return {false, SocialConsensusResult_OpReturnFailed};

            return Success;
        }

        virtual bool CheckBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockPtx)
        {
            return *blockPtx->GetTime() <= *ptx->GetTime();
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const PTransactionRef& tx,
            const PocketBlock& block) override
        {
            auto ptx = static_pointer_cast<ScoreComment>(tx);

            // Get count from chain
            int count = ConsensusRepoInst.CountChainContent(
                *ptx->GetAddress(),
                *ptx->GetTime(),
                PocketTxType::ACTION_SCORE_COMMENT);

            // Get count from block
            for (auto blockTx : block)
            {
                if (!IsIn(*blockTx->GetType(), {ACTION_SCORE_COMMENT}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<ScoreComment>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                {
                    if (CheckBlockLimitTime(tx, blockTx))
                        count += 1;

                    if (*blockPtx->GetHash() == *ptx->GetCommentTxHash())
                        return {false, SocialConsensusResult_DoubleCommentScore};
                }
            }

            return ValidateLimit(ptx, count);
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const PTransactionRef& tx) override
        {
            auto ptx = static_pointer_cast<ScoreComment>(tx);

            // Check already scored content
            if (PocketDb::ConsensusRepoInst.ExistsScore(
                *ptx->GetAddress(), *ptx->GetCommentTxHash(), ACTION_SCORE_COMMENT, true))
                return {false, SocialConsensusResult_DoubleCommentScore};

            // Check count from chain
            int count = ConsensusRepoInst.CountChainContent(
                *ptx->GetAddress(),
                *ptx->GetTime(),
                PocketTxType::ACTION_SCORE_COMMENT);

            count += ConsensusRepoInst.CountMempoolContent(*ptx->GetAddress(), PocketTxType::ACTION_SCORE_COMMENT);

            return ValidateLimit(ptx, count);
        }

        virtual tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr <ScoreComment> tx, int count)
        {
            auto reputationConsensus = ReputationConsensusFactory::Instance(Height);
            auto accountMode = reputationConsensus->GetAccountMode(*tx->GetAddress());
            auto limit = GetScoresLimit(accountMode);

            if (count >= limit)
                return {false, SocialConsensusResult_CommentScoreLimit};

            return Success;
        }

        virtual tuple<bool, SocialConsensusResult> ValidateBlocking(const string& commentAddress,
            shared_ptr <ScoreComment> tx)
        {
            return Success;
        }


        tuple<bool, SocialConsensusResult> CheckModel(const PTransactionRef& tx) override
        {
            auto ptx = static_pointer_cast<ScoreComment>(tx);

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetCommentTxHash())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetValue())) return {false, SocialConsensusResult_Failed};

            auto value = *ptx->GetValue();
            if (value != 1 && value != -1)
                return {false, SocialConsensusResult_Failed};

            return Success;
        }

        tuple<bool, SocialConsensusResult> CheckOpReturnHash(const PTransactionRef& tx) override
        {
            if (auto[ok, result] = SocialBaseConsensus::CheckOpReturnHash(tx); !ok)
                return {false, result};

            return Success;
        }
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 430000 block
    *
    *******************************************************************************************************************/
    class ScoreCommentConsensus_checkpoint_430000 : public ScoreCommentConsensus
    {
    protected:
        int CheckpointHeight() override { return 430000; }

        tuple<bool, SocialConsensusResult> ValidateBlocking(
            const string& commentAddress, shared_ptr <ScoreComment> tx) override
        {
            auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                commentAddress,
                *tx->GetAddress()
            );

            if (existsBlocking && blockingType == ACTION_BLOCKING)
                return {false, SocialConsensusResult_Blocking};

            return Success;
        }

    public:
        ScoreCommentConsensus_checkpoint_430000(int height) : ScoreCommentConsensus(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 514184 block
    *
    *******************************************************************************************************************/
    class ScoreCommentConsensus_checkpoint_514184 : public ScoreCommentConsensus_checkpoint_430000
    {
    protected:
        int CheckpointHeight() override { return 514184; }

        tuple<bool, SocialConsensusResult> ValidateBlocking(
            const string& commentAddress, shared_ptr <ScoreComment> tx) override
        {
            return Success;
        }

    public:
        ScoreCommentConsensus_checkpoint_514184(int height) : ScoreCommentConsensus_checkpoint_430000(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 1124000 block
    *
    *******************************************************************************************************************/
    class ScoreCommentConsensus_checkpoint_1124000 : public ScoreCommentConsensus_checkpoint_514184
    {
    public:
        ScoreCommentConsensus_checkpoint_1124000(int height) : ScoreCommentConsensus_checkpoint_514184(height) {}

    protected:
        int CheckpointHeight() override { return 1124000; }

        bool CheckBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockPtx) override
        {
            return true;
        }
    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class ScoreCommentConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<ScoreCommentConsensus*(int height)>> m_rules =
            {
                {1124000, [](int height) { return new ScoreCommentConsensus_checkpoint_1124000(height); }},
                {514184,  [](int height) { return new ScoreCommentConsensus_checkpoint_514184(height); }},
                {430000,  [](int height) { return new ScoreCommentConsensus_checkpoint_430000(height); }},
                {0,       [](int height) { return new ScoreCommentConsensus(height); }},
            };
    public:
        shared_ptr <ScoreCommentConsensus> Instance(int height)
        {
            return shared_ptr<ScoreCommentConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_SCORECOMMENT_HPP