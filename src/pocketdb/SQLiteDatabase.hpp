// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0


#ifndef POCKETDB_SQLITEDATABASE_H
#define POCKETDB_SQLITEDATABASE_H

#include "logging.h"
#include "sync.h"
#include "tinyformat.h"
#include "fs.h"

#include <sqlite3.h>
#include <iostream>

namespace PocketDb
{
    static std::string db_structure_sql = R"sql(
        create table if not exists Transactions
        (
            Type      int    not null,
            Hash      string not null primary key,
            Time      int    not null,

            BlockHash string null,
            Height    int    null,

            -- User.Id
            -- Post.Id
            -- Comment.Id
            Id        int    null,

            -- User.AddressHash
            -- Post.AddressHash
            -- Comment.AddressHash
            -- ScorePost.AddressHash
            -- ScoreComment.AddressHash
            -- Subscribe.AddressHash
            -- Blocking.AddressHash
            -- Complain.AddressHash
            String1   string null,

            -- User.ReferrerAddressHash
            -- Post.RootTxHash
            -- Comment.RootTxHash
            -- ScorePost.PostTxHash
            -- ScoreComment.CommentTxHash
            -- Subscribe.AddressToHash
            -- Blocking.AddressToHash
            -- Complain.PostTxHash
            String2   string null,

            -- Post.RelayTxHash
            -- Comment.PostTxHash
            String3   string null,

            -- Comment.ParentTxHash
            String4   string null,

            -- Comment.AnswerTxHash
            String5   string null,

            -- ScorePost.Value
            -- ScoreComment.Value
            -- Complain.Reason
            Int1      int    null
        );


        --------------------------------------------
        --               EXT TABLES               --
        --------------------------------------------
        create table if not exists Payload
        (
            TxHash  string primary key, -- Transactions.Hash

            -- User.Lang
            -- Post.Lang
            -- Comment.Lang
            String1 string null,

            -- User.Name
            -- Post.Caption
            -- Comment.Message
            String2 string null,

            -- User.Avatar
            -- Post.Message
            String3 string null,

            -- User.About
            -- Post.Tags JSON
            String4 string null,

            -- User.Url
            -- Post.Images JSON
            String5 string null,

            -- User.Pubkey
            -- Post.Settings JSON
            String6 string null,

            -- User.Donations JSON
            -- Post.Url
            String7 string null
        );
        --------------------------------------------
        create table if not exists TxOutputs
        (
            TxHash      string not null, -- Transactions.Hash
            Number      int    not null, -- Number in tx.vout
            AddressHash string not null, -- Address
            Value       int    not null, -- Amount
            SpentHeight int    null,     -- Where spent
            SpentTxHash string null,     -- Who spent
            primary key (TxHash, Number, AddressHash)
        );


        --------------------------------------------
        create table if not exists Ratings
        (
            Type   int not null,
            Height int not null,
            Id     int not null,
            Value  int not null,
            primary key (Type, Height, Id, Value)
        );


        --------------------------------------------
        --                 VIEWS                  --
        --------------------------------------------
        drop view if exists vAccounts;
        create view if not exists vAccounts as
        select t.Type,
               t.Hash,
               t.Time,
               t.BlockHash,
               t.Height,
               t.Id,
               t.String1 as AddressHash,
               t.String2 as ReferrerAddressHash
        from Transactions t
        where t.Height is not null
          and t.Type in (100, 101, 102);


        drop view if exists vUsers;
        create view if not exists vUsers as
        select a.*
        from vAccounts a
        where a.Type = 100;

        --------------------------------------------
        drop view if exists vContents;
        create view if not exists vContents as
        select t.Type,
               t.Hash,
               t.Time,
               t.BlockHash,
               t.Height,
               t.Id,
               t.String1 as AddressHash,
               t.String2 as RootTxHash,
               t.String3,
               t.String4,
               t.String5
        from Transactions t
        where t.Height is not null
          and t.Type in (200, 201, 202, 203, 204, 205);


        drop view if exists vPosts;
        create view if not exists vPosts as
        select c.Type,
               c.Hash,
               c.Time,
               c.BlockHash,
               c.Height,
               c.Id,
               c.AddressHash,
               c.RootTxHash,
               c.String3 as RelayTxHash
        from vContents c
        where c.Type = 200;


        drop view if exists vComments;
        create view if not exists vComments as
        select c.Type,
               c.Hash,
               c.Time,
               c.BlockHash,
               c.Height,
               c.Id,
               c.AddressHash,
               c.RootTxHash,
               c.String3 as PostTxHash,
               c.String4 as ParentTxHash,
               c.String5 as AnswerTxHash
        from vContents c
        where c.Type = 204;

        --------------------------------------------
        drop view if exists vScores;
        create view if not exists vScores as
        select t.Type,
               t.Hash,
               t.Time,
               t.BlockHash,
               t.Height,
               t.String1 as AddressHash,
               t.String2 as ContentTxHash,
               t.Int1    as Value
        from Transactions t
        where t.Height is not null
          and t.Type in (300, 301);


        drop view if exists vScorePosts;
        create view if not exists vScorePosts as
        select s.Type,
               s.Hash,
               s.Time,
               s.BlockHash,
               s.Height,
               s.AddressHash,
               s.ContentTxHash as PostTxHash,
               s.Value         as Value
        from vScores s
        where s.Type in (300);


        drop view if exists vScoreComments;
        create view if not exists vScoreComments as
        select s.Type,
               s.Hash,
               s.Time,
               s.BlockHash,
               s.Height,
               s.AddressHash,
               s.ContentTxHash as CommentTxHash,
               s.Value         as Value
        from vScores s
        where s.Type in (301);
    )sql";

    static std::string db_drop_indexes_sql = R"sql(
        drop index if exists Transactions_Type;
        drop index if exists Transactions_Hash;
        drop index if exists Transactions_Time;
        drop index if exists Transactions_BlockHash;
        drop index if exists Transactions_Height;
        drop index if exists Transactions_String1;
        drop index if exists Transactions_String2;
        drop index if exists Transactions_String3;
        drop index if exists Transactions_String4;
        drop index if exists Transactions_String5;
        drop index if exists Transactions_Int1;
        drop index if exists Transactions_Type_String1_Height;
        drop index if exists Transactions_Type_String2_Height;
        drop index if exists Transactions_Type_Height_Id;
        drop index if exists TxOutput_SpentHeight;
        drop index if exists TxOutput_TxHash_Number;
        drop index if exists TxOutputs_AddressHash_SpentHeight_Value;
        drop index if exists Ratings_Height;
        drop index if exists Ratings_Type_Id_Value;
        drop index if exists Ratings_Type_Id_Height;
    )sql";

    static std::string db_create_indexes_sql = R"sql(
        create index if not exists Transactions_Type on Transactions (Type);
        create index if not exists Transactions_Hash on Transactions (Hash);
        create index if not exists Transactions_Time on Transactions (Time);
        create index if not exists Transactions_BlockHash on Transactions (BlockHash);
        create index if not exists Transactions_Height on Transactions (Height);
        create index if not exists Transactions_String1 on Transactions (String1);
        create index if not exists Transactions_String2 on Transactions (String2);
        create index if not exists Transactions_String3 on Transactions (String3);
        create index if not exists Transactions_String4 on Transactions (String4);
        create index if not exists Transactions_String5 on Transactions (String5);
        create index if not exists Transactions_Int1 on Transactions (Int1);
        create index if not exists Transactions_Type_String1_Height on Transactions (Type, String1, Height, Hash);
        create index if not exists Transactions_Type_String2_Height on Transactions (Type, String2, Height, Hash);
        create index if not exists Transactions_Type_Height_Id on Transactions (Type, Height, Id);
        create index if not exists TxOutput_SpentHeight on TxOutputs (SpentHeight);
        create index if not exists TxOutput_TxHash_Number on TxOutputs (TxHash, Number);
        create index if not exists TxOutputs_AddressHash_SpentHeight_Value on TxOutputs (AddressHash, SpentHeight, Value);
        create index if not exists Ratings_Height on Ratings (Height);
        create index if not exists Ratings_Type_Id_Value on Ratings (Type, Id, Value);
        create index if not exists Ratings_Type_Id_Height on Ratings (Type, Id, Height desc);
    )sql";

    class SQLiteDatabase
    {
    private:
        std::string m_dir_path;
        std::string m_file_path;

        void Cleanup() noexcept
        {
            Close();

            //LOCK(g_sqlite_mutex);
            if (--g_sqlite_count == 0)
            {
                int ret = sqlite3_shutdown();
                if (ret != SQLITE_OK)
                {
                    LogPrintf("%s: %d; Failed to shutdown SQLite: %s\n", __func__, ret, sqlite3_errstr(ret));
                }
            }
        }

        Mutex g_sqlite_mutex;
        int g_sqlite_count = 0; //GUARDED_BY(g_sqlite_mutex)

        static void ErrorLogCallback(void* arg, int code, const char* msg)
        {
            // From sqlite3_config() documentation for the SQLITE_CONFIG_LOG option:
            // "The void pointer that is the second argument to SQLITE_CONFIG_LOG is passed through as
            // the first parameter to the application-defined logger function whenever that function is
            // invoked."
            // Assert that this is the case:
            assert(arg == nullptr);
            LogPrintf("%s: %d; Message: %s\n", __func__, code, msg);
        }

        bool TryCreateDbIfNotExists()
        {
            try
            {
                return fs::create_directories(m_dir_path);
            }
            catch (const fs::filesystem_error&)
            {
                if (!fs::exists(m_dir_path) || !fs::is_directory(m_dir_path))
                    throw;

                return false;
            }
        }

        bool BulkExecute(std::string sql) const
        {
            try
            {
                char* errMsg = nullptr;
                size_t pos;
                std::string token;

                while ((pos = sql.find(';')) != std::string::npos)
                {
                    token = sql.substr(0, pos);

                    BeginTransaction();

                    if (sqlite3_exec(m_db, token.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK)
                        throw std::runtime_error("Failed to create init database");

                    CommitTransaction();

                    sql.erase(0, pos + 1);
                }
            }
            catch (const std::exception&)
            {
                AbortTransaction();
                LogPrintf("%s: Failed to create init database structure", __func__);
                return false;
            }

            return true;
        }

    public:
        sqlite3* m_db{nullptr};

        SQLiteDatabase() = default;

        void Init(const std::string& dir_path, const std::string& file_path)
        {
            m_dir_path = dir_path;
            m_file_path = file_path;

            if (++g_sqlite_count == 1)
            {
                // Setup logging
                int ret = sqlite3_config(SQLITE_CONFIG_LOG, ErrorLogCallback, nullptr);
                if (ret != SQLITE_OK)
                {
                    throw std::runtime_error(
                        strprintf("%s: %sd Failed to setup error log: %s\n", __func__, ret, sqlite3_errstr(ret)));
                }
                // Force serialized threading mode
                ret = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
                if (ret != SQLITE_OK)
                {
                    throw std::runtime_error(
                        strprintf("%s: %d; Failed to configure serialized threading mode: %s\n",
                            __func__, ret, sqlite3_errstr(ret)));
                }

                TryCreateDbIfNotExists();
            }
            int ret = sqlite3_initialize(); // This is a no-op if sqlite3 is already initialized
            if (ret != SQLITE_OK)
            {
                throw std::runtime_error(
                    strprintf("%s: %d; Failed to initialize SQLite: %s\n", __func__, ret, sqlite3_errstr(ret)));
            }

            try
            {
                Open();
            } catch (const std::runtime_error&)
            {
                // If open fails, cleanup this object and rethrow the exception
                Cleanup();
                throw;
            }
        }

        void CreateStructure()
        {
            LogPrintf("Creating Sqlite database structure..\n");

            if (!m_db || sqlite3_get_autocommit(m_db) == 0)
                throw std::runtime_error(strprintf("%s: Database not opened?\n", __func__));

            if (!BulkExecute(db_structure_sql))
                throw std::runtime_error(strprintf("%s: Failed to create database structure\n", __func__));
        }

        void DropIndexes()
        {
            LogPrintf("Drop Sqlite database indexes..\n");

            if (!m_db || sqlite3_get_autocommit(m_db) == 0)
                throw std::runtime_error(strprintf("%s: Database not opened?\n", __func__));

            if (!BulkExecute(db_drop_indexes_sql))
                throw std::runtime_error(strprintf("%s: Failed to drop indexes\n", __func__));
        }

        void CreateIndexes()
        {
            LogPrintf("Creating Sqlite database indexes..\n");

            if (!m_db || sqlite3_get_autocommit(m_db) == 0)
                throw std::runtime_error(strprintf("%s: Database not opened?\n", __func__));

            if (!BulkExecute(db_create_indexes_sql))
                throw std::runtime_error(strprintf("%s: Failed to create indexes\n", __func__));
        }

        void Open()
        {
            int flags = SQLITE_OPEN_READWRITE |
                        SQLITE_OPEN_CREATE; //SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

            if (m_db == nullptr)
            {
                int ret = sqlite3_open_v2(m_file_path.c_str(), &m_db, flags, nullptr);
                if (ret != SQLITE_OK)
                    throw std::runtime_error(strprintf("%s: %d; Failed to open database: %s\n",
                        __func__, ret, sqlite3_errstr(ret)));

                CreateStructure();
                CreateIndexes();
            }

            if (sqlite3_db_readonly(m_db, "main") != 0)
                throw std::runtime_error("Database opened in readonly");

            if (sqlite3_exec(m_db, "PRAGMA journal_mode = wal;", nullptr, nullptr, nullptr) != 0)
                throw std::runtime_error("Failed apply journal_mode = wal");

            //    // Acquire an exclusive lock on the database
            //    // First change the locking mode to exclusive
            //    int ret = sqlite3_exec(m_db, "PRAGMA locking_mode = exclusive", nullptr, nullptr, nullptr);
            //    if (ret != SQLITE_OK) {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Unable to change database locking mode to exclusive: %s\n", sqlite3_errstr(ret)));
            //    }
            //    // Now begin a transaction to acquire the exclusive lock. This lock won't be released until we close because of the exclusive locking mode.
            //    ret = sqlite3_exec(m_db, "BEGIN EXCLUSIVE TRANSACTION", nullptr, nullptr, nullptr);
            //    if (ret != SQLITE_OK) {
            //        throw std::runtime_error("SQLiteDatabase: Unable to obtain an exclusive lock on the database, is it being used by another bitcoind?\n");
            //    }
            //    ret = sqlite3_exec(m_db, "COMMIT", nullptr, nullptr, nullptr);
            //    if (ret != SQLITE_OK) {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Unable to end exclusive lock transaction: %s\n", sqlite3_errstr(ret)));
            //    }
            //
            //    // Enable fullfsync for the platforms that use it
            //    ret = sqlite3_exec(m_db, "PRAGMA fullfsync = true", nullptr, nullptr, nullptr);
            //    if (ret != SQLITE_OK) {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to enable fullfsync: %s\n", sqlite3_errstr(ret)));
            //    }
            //
            //    // Make the table for our key-value pairs
            //    // First check that the main table exists
            //    sqlite3_stmt* check_main_stmt{nullptr};
            //    ret = sqlite3_prepare_v2(m_db, "SELECT name FROM sqlite_master WHERE type='table' AND name='main'", -1, &check_main_stmt, nullptr);
            //    if (ret != SQLITE_OK) {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to prepare statement to check table existence: %s\n", sqlite3_errstr(ret)));
            //    }
            //    ret = sqlite3_step(check_main_stmt);
            //    if (sqlite3_finalize(check_main_stmt) != SQLITE_OK) {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to finalize statement checking table existence: %s\n", sqlite3_errstr(ret)));
            //    }
            //    bool table_exists;
            //    if (ret == SQLITE_DONE) {
            //        table_exists = false;
            //    } else if (ret == SQLITE_ROW) {
            //        table_exists = true;
            //    } else {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to execute statement to check table existence: %s\n", sqlite3_errstr(ret)));
            //    }
            //
            //    // Do the db setup things because the table doesn't exist only when we are creating a new wallet
            //    if (!table_exists) {
            //        ret = sqlite3_exec(m_db, "CREATE TABLE main(key BLOB PRIMARY KEY NOT NULL, value BLOB NOT NULL)", nullptr, nullptr, nullptr);
            //        if (ret != SQLITE_OK) {
            //            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to create new database: %s\n", sqlite3_errstr(ret)));
            //        }

            // Set the application id
            //        uint32_t app_id = ReadBE32(Params().MessageStart());
            //        std::string set_app_id = strprintf("PRAGMA application_id = %d", static_cast<int32_t>(app_id));
            //        ret = sqlite3_exec(m_db, set_app_id.c_str(), nullptr, nullptr, nullptr);
            //        if (ret != SQLITE_OK) {
            //            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to set the application id: %s\n", sqlite3_errstr(ret)));
            //        }

            // Set the user version
            //        std::string set_user_ver = strprintf("PRAGMA user_version = %d", WALLET_SCHEMA_VERSION);
            //        ret = sqlite3_exec(m_db, set_user_ver.c_str(), nullptr, nullptr, nullptr);
            //        if (ret != SQLITE_OK) {
            //            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to set the wallet schema version: %s\n", sqlite3_errstr(ret)));
            //        }
            //    }
        }

        void Close()
        {
            int res = sqlite3_close(m_db);
            if (res != SQLITE_OK)
            {
                throw std::runtime_error(
                    strprintf("%s: %d; Failed to close database: %s\n", __func__, res, sqlite3_errstr(res)));
            }
            m_db = nullptr;
        }

        bool BeginTransaction() const
        {
            if (!m_db || sqlite3_get_autocommit(m_db) == 0) return false;
            int res = sqlite3_exec(m_db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
            if (res != SQLITE_OK)
            {
                LogPrintf("%s: %d; Failed to begin the transaction: %s\n", __func__, res, sqlite3_errstr(res));
            }
            return res == SQLITE_OK;
        }

        bool CommitTransaction() const
        {
            if (!m_db || sqlite3_get_autocommit(m_db) != 0) return false;
            int res = sqlite3_exec(m_db, "COMMIT TRANSACTION", nullptr, nullptr, nullptr);
            if (res != SQLITE_OK)
            {
                LogPrintf("%s: %d; Failed to commit the transaction: %s\n", __func__, res, sqlite3_errstr(res));
            }
            return res == SQLITE_OK;
        }

        bool AbortTransaction() const
        {
            if (!m_db || sqlite3_get_autocommit(m_db) != 0) return false;
            int res = sqlite3_exec(m_db, "ROLLBACK TRANSACTION", nullptr, nullptr, nullptr);
            if (res != SQLITE_OK)
            {
                LogPrintf("%s: %d; Failed to abort the transaction: %s\n", __func__, res, sqlite3_errstr(res));
            }
            return res == SQLITE_OK;
        }

    };
} // namespace PocketDb

#endif // POCKETDB_SQLITEDATABASE_H
