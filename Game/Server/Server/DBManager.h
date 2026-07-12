#pragma once
#include "pch.h"

#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <string>

class DBManager {
    DECLARE_SINGLE(DBManager)
public:
    bool Initialize(const std::wstring& dsn);
    void Cleanup();

    bool Login(const std::string& id, const std::string& pw);
    bool Register(const std::string& id, const std::string& pw);

private:
    DBManager() = default;
    ~DBManager() { Cleanup(); }

    SQLHENV m_hEnv = SQL_NULL_HENV;
    SQLHDBC m_hDbc = SQL_NULL_HDBC;
    bool m_bConnected = false;

    // Helper for error logging
    void ExtractError(SQLSMALLINT handleType, SQLHANDLE handle);
};
