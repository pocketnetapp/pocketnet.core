// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_EDIT_HPP
#define POCKETCONSENSUS_COMMENT_EDIT_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/dto/CommentEdit.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  CommentEdit consensus base class
    *
    *******************************************************************************************************************/
    class CommentEditConsensus : public SocialBaseConsensus
    {
    public:
        CommentEditConsensus(int height) : SocialBaseConsensus(height) {}
        CommentEditConsensus() : SocialBaseConsensus() {}

        tuple<bool, SocialConsensusResult> Validate(shared_ptr<Transaction> tx, PocketBlock& block) override
        {
            return Validate(static_pointer_cast<CommentEdit>(tx), block);
        }

        tuple<bool, SocialConsensusResult> Check(shared_ptr<Transaction> tx) override
        {
            if (auto[ok, result] = SocialBaseConsensus::Check(tx); !ok)
                return make_tuple(ok, result);

            return Check(static_pointer_cast<CommentEdit>(tx));
        }

    protected:



    private:
    
        tuple<bool, SocialConsensusResult> Validate(shared_ptr<CommentEdit> tx, PocketBlock& block)
        {
            return make_tuple(true, SocialConsensusResult_Success);
            // std::string _address = oitm["address"].get_str();
            // int64_t _time = oitm["time"].get_int64();

            // std::string _msg = oitm["msg"].get_str();
            // std::string _txid = oitm["txid"].get_str();
            // std::string _otxid = oitm["otxid"].get_str();
            // std::string _postid = oitm["postid"].get_str();
            // std::string _parentid = oitm["parentid"].get_str();
            // std::string _answerid = oitm["answerid"].get_str();

            // // User registered?
            // if (!CheckRegistration(oitm, _address, checkMempool, checkWithTime, height, blockVtx, result)) {
            //     return false;
            // }

            // // Size message limit
            // if (_msg == "" || UrlDecode(_msg).length() > GetActualLimit(Limit::comment_size_limit, height)) {
            //     result = ANTIBOTRESULT::Size;
            //     return false;
            // }

            // // Original comment exists
            // reindexer::Item _original_comment_itm;
            // if (!g_pocketdb->SelectOne(Query("Comment").Where("otxid", CondEq, _otxid).Where("txid", CondEq, _otxid).Where("address", CondEq, _address).Where("block", CondLt, height), _original_comment_itm).ok()) {
            //     result = ANTIBOTRESULT::NotFound;
            //     return false;
            // }

            // // Last comment not deleted
            // if (!g_pocketdb->Exists(Query("Comment").Where("otxid", CondEq, _otxid).Where("last", CondEq, true).Where("address", CondEq, _address).Not().Where("msg", CondEq, "").Where("block", CondLt, height))) {
            //     result = ANTIBOTRESULT::CommentDeletedEdit;
            //     return false;
            // }

            // // Parent comment
            // if (_parentid != _original_comment_itm["parentid"].As<string>() || (_parentid != "" && !g_pocketdb->Exists(Query("Comment").Where("otxid", CondEq, _parentid).Where("last", CondEq, true).Not().Where("msg", CondEq, "").Where("block", CondLt, height)))) {
            //     result = ANTIBOTRESULT::InvalidParentComment;
            //     return false;
            // }

            // // Answer comment
            // if (_answerid != _original_comment_itm["answerid"].As<string>() || (_answerid != "" && !g_pocketdb->Exists(Query("Comment").Where("otxid", CondEq, _answerid).Where("last", CondEq, true).Not().Where("msg", CondEq, "").Where("block", CondLt, height)))) {
            //     result = ANTIBOTRESULT::InvalidAnswerComment;
            //     return false;
            // }

            // // Original comment edit only 24 hours
            // if (_time - _original_comment_itm["time"].As<int64_t>() > GetActualLimit(Limit::edit_comment_timeout, height)) {
            //     result = ANTIBOTRESULT::CommentEditLimit;
            //     return false;
            // }

            // Item post_itm;
            // if (_postid == "" || !g_pocketdb->SelectOne(Query("Posts").Where("txid", CondEq, _postid).Where("block", CondLt, height), post_itm).ok()) {
            //     result = ANTIBOTRESULT::NotFound;
            //     return false;
            // }

            // // Blocking
            // if (g_pocketdb->Exists(Query("BlockingView").Where("address", CondEq, post_itm["address"].As<string>()).Where("address_to", CondEq, _address).Where("block", CondLt, height))) {
            //     result = ANTIBOTRESULT::Blocking;
            //     return false;
            // }

            // // Double edit in block denied
            // if (blockVtx.Exists("Comment")) {
            //     for (auto& mtx : blockVtx.Data["Comment"]) {
            //         if (mtx["txid"].get_str() != _txid && mtx["otxid"].get_str() == _otxid) {
            //             result = ANTIBOTRESULT::DoubleCommentEdit;
            //             return false;
            //         }
            //     }
            // }

            // // Double edit in mempool denied
            // if (checkMempool) {
            //     reindexer::QueryResults res;
            //     if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Comment").Not().Where("txid", CondEq, _txid), res).ok()) {
            //         for (auto& m : res) {
            //             reindexer::Item mItm = m.GetItem();
            //             std::string t_src = DecodeBase64(mItm["data"].As<string>());

            //             reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Comment");
            //             if (t_itm.FromJSON(t_src).ok()) {
            //                 if (t_itm["otxid"].As<string>() == _otxid) {
            //                     result = ANTIBOTRESULT::DoubleCommentEdit;
            //                     return false;
            //                 }
            //             }
            //         }
            //     }
            // }

            // // Check limit
            // {
            //     size_t edit_count = g_pocketdb->SelectCount(Query("Comment").Where("otxid", CondEq, _otxid).Where("block", CondLt, height));

            //     ABMODE mode;
            //     getMode(_address, mode, height);
            //     int limit = getLimit(CommentEdit, mode, height);
            //     if (edit_count >= limit) {
            //         result = ANTIBOTRESULT::CommentEditLimit;
            //         return false;
            //     }
            // }

            // return true;
        }

        tuple<bool, SocialConsensusResult> Check(shared_ptr<CommentEdit> tx)
        {
            
        }

    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class CommentEditConsensus_checkpoint_1 : public CommentEditConsensus
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        CommentEditConsensus_checkpoint_1(int height) : CommentEditConsensus(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class CommentEditConsensusFactory
    {
    private:
        const std::map<int, std::function<CommentEditConsensus*(int height)>> m_rules =
        {
            {0, [](int height) { return new CommentEditConsensus(height); }},
        };
    public:
        shared_ptr <CommentEditConsensus> Instance(int height)
        {
            return shared_ptr<CommentEditConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_COMMENT_EDIT_HPP