// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0


#ifndef POCKETTX_COMMENT_DELETE_HPP
#define POCKETTX_COMMENT_DELETE_HPP

#include "pocketdb/models/dto/Comment.hpp"

namespace PocketTx
{

    class CommentDelete : public Comment
    {
    public:

        CommentDelete(string& hash, int64_t time) : Comment(hash, time)
        {
            SetType(PocketTxType::CONTENT_COMMENT_DELETE);
        }

    protected:

        void BuildPayload(const UniValue& src) override { }

    };

} // namespace PocketTx

#endif //POCKETTX_COMMENT_DELETE_HPP
