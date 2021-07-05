// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_POST_HPP
#define POCKETCONSENSUS_POST_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/dto/Post.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  Post consensus base class
    *
    *******************************************************************************************************************/
    class PostConsensus : public SocialBaseConsensus
    {
    public:
        PostConsensus(int height) : SocialBaseConsensus(height) {}

    protected:

        virtual int64_t GetEditPostWindow() { return 86400; }
        virtual int64_t GetFullLimit() { return 30; }
        virtual int64_t GetFullEditLimit() { return 5; }
        virtual int64_t GetTrialLimit() { return 15; }
        virtual int64_t GetTrialEditLimit() { return 5; }

        virtual int64_t GetLimit(AccountMode mode)
        {
            return mode == AccountMode_Full ? GetFullLimit() : GetTrialLimit();
        }
        virtual int64_t GetEditLimit(AccountMode mode)
        {
            return mode == AccountMode_Full ? GetFullEditLimit() : GetTrialEditLimit();
        }

        // ------------------------------------------------------------------------------------------------------------

        tuple<bool, SocialConsensusResult> ValidateModel(shared_ptr <Transaction> tx) override
        {
            auto ptx = static_pointer_cast<Post>(tx);

            vector<string> addresses = {*ptx->GetAddress()};
            if (!PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses))
                return {false, SocialConsensusResult_NotRegistered};

            if (ptx->IsEdit())
                return ValidateEditModel(ptx);

            return Success;
        }

        virtual tuple<bool, SocialConsensusResult> ValidateEditModel(shared_ptr <Post> tx)
        {
            // First get original post transaction
            auto originalTx = PocketDb::TransRepoInst.GetByHash(*tx->GetRootTxHash());
            if (!originalTx)
                return {false, SocialConsensusResult_NotFound};

            // Change type not allowed
            if (*originalTx->GetType() != *tx->GetType())
                return {false, SocialConsensusResult_NotAllowed};

            // Cast tx to Post for next checks
            auto originalPostTx = static_pointer_cast<Post>(originalTx);

            // You are author? Really?
            if (*tx->GetAddress() != *originalPostTx->GetAddress())
                return {false, SocialConsensusResult_ContentEditUnauthorized};

            // Original post edit only 24 hours
            if ((*tx->GetTime() - *originalPostTx->GetTime()) > GetEditPostWindow())
                return {false, SocialConsensusResult_ContentEditLimit};

            return make_tuple(true, SocialConsensusResult_Success);
        }

        // ------------------------------------------------------------------------------------------------------------

        virtual bool CheckBlockLimitTime(shared_ptr <Post> ptx, shared_ptr <Post> blockPtx)
        {
            return *blockPtx->GetTime() <= *ptx->GetTime();
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr <Transaction> tx, const PocketBlock& block) override
        {
            auto ptx = static_pointer_cast<Post>(tx);

            // ---------------------------------------------------------
            // Edit posts
            if (ptx->IsEdit())
                return ValidateEditLimit(ptx, block);

            // ---------------------------------------------------------
            // New posts

            // Get count from chain
            int count = ConsensusRepoInst.CountChainContent(
                *ptx->GetAddress(),
                *ptx->GetTime(),
                PocketTxType::CONTENT_POST
            );

            // Get count from block
            for (auto blockTx : block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_POST}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Post>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                {
                    if (CheckBlockLimitTime(ptx, blockPtx))
                        count += 1;
                }
            }

            return ValidateLimit(ptx, count);
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr <Transaction> tx) override
        {
            auto ptx = static_pointer_cast<Post>(tx);

            // ---------------------------------------------------------
            // Edit posts
            if (ptx->IsEdit())
                return ValidateEditLimit(ptx);

            // ---------------------------------------------------------
            // New posts

            // Get count from chain
            int count = ConsensusRepoInst.CountChainContent(
                *ptx->GetAddress(),
                *ptx->GetTime(),
                PocketTxType::CONTENT_POST
            );

            count += ConsensusRepoInst.CountMempoolContent(*ptx->GetAddress(), PocketTxType::CONTENT_POST);

            return ValidateLimit(ptx, count);
        }

        virtual tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr <Post> tx, int count)
        {
            auto reputationConsensus = ReputationConsensusFactory::Instance(Height);
            auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*tx->GetAddress());
            auto limit = GetLimit(mode);

            if (count >= limit)
                return {false, SocialConsensusResult_ContentLimit};

            return Success;
        }

        // ------------------------------------------------------------------------------------------------------------

        virtual tuple<bool, SocialConsensusResult> ValidateEditLimit(shared_ptr <Post> tx, const PocketBlock& block)
        {
            // Double edit in block not allowed
            for (auto blockTx : block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_POST}))
                    continue;

                if (*blockTx->GetHash() == *tx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Post>(blockTx);
                if (*tx->GetRootTxHash() == *blockPtx->GetRootTxHash())
                    return {false, SocialConsensusResult_DoubleContentEdit};
            }

            // Check edit one post limit
            int count = ConsensusRepoInst.CountChainContentEdit(*tx->GetRootTxHash());

            auto reputationConsensus = ReputationConsensusFactory::Instance(Height);
            auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*tx->GetAddress());
            auto limit = GetEditLimit(mode);

            if (count >= limit)
                return {false, SocialConsensusResult_ContentEditLimit};

            return Success;
        }

        virtual tuple<bool, SocialConsensusResult> ValidateEditLimit(shared_ptr <Post> tx)
        {
            if (ConsensusRepoInst.CountMempoolContentEdit(*tx->GetRootTxHash()) > 0)
                return {false, SocialConsensusResult_DoubleContentEdit};

            return Success;
        }


        tuple<bool, SocialConsensusResult> CheckModel(shared_ptr <Transaction> tx) override
        {
            auto ptx = static_pointer_cast<Post>(tx);

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};

            return Success;
        }

    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at ? block
    *
    *******************************************************************************************************************/
    // TODO (brangr): change time to block height
    class PostConsensus_checkpoint_ : public PostConsensus
    {
    protected:
        int CheckpointHeight() override { return 0; }
    public:
        PostConsensus_checkpoint_(int height) : PostConsensus(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 1124000 block
    *
    *******************************************************************************************************************/
    class PostConsensus_checkpoint_1124000 : public PostConsensus
    {
    public:
        PostConsensus_checkpoint_1124000(int height) : PostConsensus(height) {}
    protected:
        int CheckpointHeight() override { return 1124000; }

        bool CheckBlockLimitTime(shared_ptr <Post> ptx, shared_ptr <Post> blockPtx) override
        {
            return true;
        }
    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class PostConsensusFactory
    {
    private:
        const std::map<int, std::function<PostConsensus*(int height)>> m_rules =
            {
                {1124000, [](int height) { return new PostConsensus_checkpoint_1124000(height); }},
                {0, [](int height) { return new PostConsensus(height); }},
            };
    public:
        shared_ptr <PostConsensus> Instance(int height)
        {
            return shared_ptr<PostConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_POST_HPP