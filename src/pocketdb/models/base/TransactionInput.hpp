// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_TRANSACTIONINPUT_H
#define POCKETTX_TRANSACTIONINPUT_H

#include <string>
#include <univalue.h>
#include <utility>
#include <utilstrencodings.h>
#include <crypto/sha256.h>
#include "pocketdb/models/base/Base.hpp"

namespace PocketTx
{
    class TransactionInput : public Base
    {
    public:

        TransactionInput() = default;

        [[nodiscard]] shared_ptr <string> GetTxHash() const { return m_txHash; }
        void SetTxHash(string value) { m_txHash = make_shared<string>(value); }

        [[nodiscard]] shared_ptr <string> GetInputTxHash() const { return m_inputTxHash; }
        void SetInputTxHash(string value) { m_inputTxHash = make_shared<string>(value); }

        [[nodiscard]] shared_ptr <int64_t> GetInputTxNumber() const { return m_inputTxNumber; }
        void SetInputTxNumber(int64_t value) { m_inputTxNumber = make_shared<int64_t>(value); }

    protected:
        shared_ptr <string> m_txHash = nullptr;
        shared_ptr <string> m_inputTxHash = nullptr;
        shared_ptr <int64_t> m_inputTxNumber = nullptr;

    private:

    };

} // namespace PocketTx

#endif // POCKETTX_TRANSACTIONINPUT_H