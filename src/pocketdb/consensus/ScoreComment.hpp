// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SCORECOMMENT_HPP
#define POCKETCONSENSUS_SCORECOMMENT_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  ScoreComment consensus base class
    *
    *******************************************************************************************************************/
    class ScoreCommentConsensus : public BaseConsensus
    {
    protected:
    public:
        ScoreCommentConsensus() = default;
    };


    /*******************************************************************************************************************
    *
    *  Start checkpoint
    *
    *******************************************************************************************************************/
    class ScoreCommentConsensus_checkpoint_0 : public ScoreCommentConsensus
    {
    protected:
    public:

        ScoreCommentConsensus_checkpoint_0() = default;

    }; // class ScoreCommentConsensus_checkpoint_0


    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class ScoreCommentConsensus_checkpoint_1 : public ScoreCommentConsensus_checkpoint_0
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
    class ScoreCommentConsensusFactory
    {
    private:
        inline static std::vector<std::pair<int, std::function<ScoreCommentConsensus *()>>> m_rules
        {
            {1, []() { return new ScoreCommentConsensus_checkpoint_1(); }},
            {0, []() { return new ScoreCommentConsensus_checkpoint_0(); }},
        };
    public:
        shared_ptr <ScoreCommentConsensus> Instance(int height)
        {
            for (const auto& rule : m_rules) {
                if (height >= rule.first) {
                    return shared_ptr<ScoreCommentConsensus>(rule.second());
                }
            }
        }
    };
}

#endif // POCKETCONSENSUS_SCORECOMMENT_HPP