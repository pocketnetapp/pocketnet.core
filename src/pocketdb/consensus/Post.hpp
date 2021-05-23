// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_POST_HPP
#define POCKETCONSENSUS_POST_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  Post consensus base class
    *
    *******************************************************************************************************************/
    class PostConsensus : public BaseConsensus
    {
    protected:
    public:
        PostConsensus() = default;
    };


    /*******************************************************************************************************************
    *
    *  Start checkpoint
    *
    *******************************************************************************************************************/
    class PostConsensus_checkpoint_0 : public PostConsensus
    {
    protected:
    public:

        PostConsensus_checkpoint_0() = default;

    }; // class PostConsensus_checkpoint_0


    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class PostConsensus_checkpoint_1 : public PostConsensus_checkpoint_0
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class PostConsensusFactory
    {
    private:
        inline static std::vector<std::pair<int, std::function<PostConsensus *()>>> m_rules
        {
            {1, []() { return new PostConsensus_checkpoint_1(); }},
            {0, []() { return new PostConsensus_checkpoint_0(); }},
        };
    public:
        shared_ptr <PostConsensus> Instance(int height)
        {
            for (const auto& rule : m_rules) {
                if (height >= rule.first) {
                    return shared_ptr<PostConsensus>(rule.second());
                }
            }
        }
    };
}

#endif // POCKETCONSENSUS_POST_HPP