// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_UTXOREPOSITORY_HPP
#define POCKETDB_UTXOREPOSITORY_HPP

#include "pocketdb/pocketnet.h"
#include "pocketdb/repositories/BaseRepository.hpp"
#include "pocketdb/models/base/Utxo.hpp"

#include <univalue.h>

namespace PocketDb
{
    using namespace PocketTx;

    class UtxoRepository : public BaseRepository
    {
    public:
        explicit UtxoRepository(SQLiteDatabase &db) : BaseRepository(db)
        {
        }

        void Init() override
        {
            SetupSqlStatements();
        }

        void Destroy() override
        {
            FinalizeSqlStatement(m_clear_all_stmt);
            FinalizeSqlStatement(m_insert_stmt);
            FinalizeSqlStatement(m_spent_stmt);
        }

        bool ClearAll()
        {
            assert(m_database.m_db);
            auto result = false;

            if (TryStepStatement(m_clear_all_stmt))
                result = true;

            sqlite3_reset(m_clear_all_stmt);
            return result;
        }

        bool Insert(const shared_ptr<Utxo> &utxo)
        {
            if (ShutdownRequested())
                return false;

            assert(m_database.m_db);
            auto result = TryBindInsertStatement(m_insert_stmt, utxo) && TryStepStatement(m_insert_stmt);
            sqlite3_clear_bindings(m_insert_stmt);
            sqlite3_reset(m_insert_stmt);
            return result;
        }

        bool BulkInsert(const std::vector<shared_ptr<Utxo>> &utxos)
        {
            if (ShutdownRequested())
                return false;

            assert(m_database.m_db);

            if (!m_database.BeginTransaction())
                return false;

            try
            {
                for (const auto &utxo : utxos)
                {
                    if (!Insert(utxo))
                        throw std::runtime_error(strprintf("%s: can't insert in Utxo\n", __func__));
                }

                if (!m_database.CommitTransaction())
                    throw std::runtime_error(strprintf("%s: can't commit insert Utxo\n", __func__));

            } catch (std::exception &ex)
            {
                m_database.AbortTransaction();
                return false;
            }

            return true;
        }

        bool Spent(const shared_ptr<Utxo> &utxo)
        {
            if (ShutdownRequested())
                return false;

            assert(m_database.m_db);
            auto result = TryBindSpentStatement(m_spent_stmt, utxo) && TryStepStatement(m_spent_stmt);
            sqlite3_clear_bindings(m_spent_stmt);
            sqlite3_reset(m_spent_stmt);
            return result;
        }

        bool BulkSpent(const std::vector<shared_ptr<Utxo>> &utxos)
        {
            if (ShutdownRequested())
                return false;

            assert(m_database.m_db);

            if (!m_database.BeginTransaction())
                return false;

            try
            {
                for (const auto &utxo : utxos)
                {
                    if (!Spent(utxo))
                        throw std::runtime_error(strprintf("%s: can't spent Utxo\n", __func__));
                }

                if (!m_database.CommitTransaction())
                    throw std::runtime_error(strprintf("%s: can't commit spent Utxo\n", __func__));

            } catch (std::exception &ex)
            {
                m_database.AbortTransaction();
                return false;
            }

            return true;
        }

        UniValue SelectTopUtxo(int count)
        {
            assert(m_database.m_db);

            UniValue result(UniValue::VARR);

            if (!TryBindStatementInt(m_select_top_stmt, 0, count))
                return result;

            try
            {
                auto res = sqlite3_step(m_select_top_stmt);
                if (res != SQLITE_ROW)
                {
                    if (res != SQLITE_DONE)
                    {
                        return result;
                    }
                }

                while(sqlite3_column_text(m_select_top_stmt, 0))
                {
                    UniValue utxo(UniValue::VOBJ);
                    utxo.pushKV("address", sqlite3_column_text(m_select_top_stmt, 0));
                    utxo.pushKV("balance", sqlite3_column_int(m_select_top_stmt, 1));

                    result.push_back(utxo);

                    sqlite3_step(m_select_top_stmt);
                }
            } catch (std::exception &ex)
            {
                return result;
            }

            sqlite3_clear_bindings(m_select_top_stmt);
            sqlite3_reset(m_select_top_stmt);

            return result;
        }

    private:
        sqlite3_stmt *m_clear_all_stmt{nullptr};
        sqlite3_stmt *m_insert_stmt{nullptr};
        sqlite3_stmt *m_spent_stmt{nullptr};
        sqlite3_stmt *m_select_top_stmt{nullptr};

        void SetupSqlStatements()
        {
            m_clear_all_stmt = SetupSqlStatement(
                m_clear_all_stmt,
                "DELETE FROM Utxo;"
            );

            m_insert_stmt = SetupSqlStatement(
                m_insert_stmt,
                " INSERT INTO Utxo ("
                "   TxId,"
                "   Block,"
                "   TxOut,"
                "   TxTime,"
                "   Address,"
                "   BlockSpent,"
                "   Amount"
                " )"
                " SELECT ?,?,?,?,?,?,?"
                " WHERE not exists (select 1 from Utxo t where t.TxId = ? and t.TxOut = ?)"
                " ;"
            );

            m_spent_stmt = SetupSqlStatement(
                m_spent_stmt,
                " UPDATE Utxo set"
                "   BlockSpent = ?"
                " WHERE TxId = ?"
                "   AND TxOut = ?"
                " ;"
            );

            m_select_top_stmt = SetupSqlStatement(
                m_select_top_stmt,
                "SELECT u.Address, SUM(u.Amount)Balance"
                "FROM Utxo u"
                "WHERE u.BlockSpent is null"
                "GROUP BY u.Address"
                "ORDER BY sum(u.Amount) desc"
                "LIMIT ?"
            );
        }

        bool TryBindInsertStatement(sqlite3_stmt *stmt, const shared_ptr<Utxo> &utxo)
        {
            auto result = TryBindStatementText(stmt, 1, utxo->GetTxId());
            result &= TryBindStatementInt64(stmt, 2, utxo->GetBlock());
            result &= TryBindStatementInt64(stmt, 3, utxo->GetTxOut());
            result &= TryBindStatementInt64(stmt, 4, utxo->GetTxTime());
            result &= TryBindStatementText(stmt, 5, utxo->GetAddress());
            result &= TryBindStatementInt64(stmt, 6, utxo->GetBlockSpent());
            result &= TryBindStatementInt64(stmt, 7, utxo->GetAmount());
            result &= TryBindStatementText(stmt, 8, utxo->GetTxId());
            result &= TryBindStatementInt64(stmt, 9, utxo->GetTxOut());

            if (!result)
                sqlite3_clear_bindings(stmt);

            return result;
        }

        bool TryBindSpentStatement(sqlite3_stmt *stmt, const shared_ptr<Utxo> &utxo)
        {
            auto result = TryBindStatementInt64(stmt, 1, utxo->GetBlockSpent());
            result &= TryBindStatementText(stmt, 2, utxo->GetTxId());
            result &= TryBindStatementInt64(stmt, 3, utxo->GetTxOut());

            if (!result)
                sqlite3_clear_bindings(stmt);

            return result;
        }

    };

} // namespace PocketDb

#endif // POCKETDB_UTXOREPOSITORY_HPP

