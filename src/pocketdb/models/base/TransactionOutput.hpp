// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_TRANSACTIONOUTPUT_H
#define POCKETTX_TRANSACTIONOUTPUT_H

#include "pocketdb/models/base/Base.hpp"
#include <crypto/sha256.h>
#include <string>
#include <univalue.h>
#include <utility>
#include <utilstrencodings.h>

namespace PocketTx
{
    class TransactionOutput : public Base
    {
    public:
        TransactionOutput() = default;

        [[nodiscard]] shared_ptr <string> GetTxHash() const { return m_txHash; }
        void SetTxHash(string value) { m_txHash = make_shared<string>(value); }

        [[nodiscard]] shared_ptr <int64_t> GetNumber() const { return m_number; }
        void SetNumber(int64_t value) { m_number = make_shared<int64_t>(value); }

        [[nodiscard]] shared_ptr <int64_t> GetValue() const { return m_value; }
        void SetValue(int64_t value) { m_value = make_shared<int64_t>(value); }


        [[nodiscard]] shared_ptr <vector<string>> GetDestinations() const { return m_destinations; }
        void AddDestination(string value) { m_destinations->push_back(value); }

    protected:
        shared_ptr <string> m_txHash = nullptr;
        shared_ptr <int64_t> m_number = nullptr;
        shared_ptr <int64_t> m_value = nullptr;

        shared_ptr <vector<string>> m_destinations;

    private:
    };

} // namespace PocketTx

#endif // POCKETTX_TRANSACTIONOUTPUT_H