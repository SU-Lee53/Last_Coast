#include "pch.h"
#include "DBManager.h"
#include <iostream>

#pragma comment(lib, "odbc32.lib")

bool DBManager::Initialize(const std::wstring& dsn) {
    SQLRETURN retCode;

    // Allocate Environment Handle
    retCode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_hEnv);
    if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Error Allocating Environment Handle" << std::endl;
        return false;
    }

    // Set ODBC Version
    SQLSetEnvAttr(m_hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

    // Allocate Connection Handle
    retCode = SQLAllocHandle(SQL_HANDLE_DBC, m_hEnv, &m_hDbc);
    if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Error Allocating Connection Handle" << std::endl;
        ExtractError(SQL_HANDLE_ENV, m_hEnv);
        return false;
    }

    // Connect to DSN
    retCode = SQLConnectW(m_hDbc, (SQLWCHAR*)dsn.c_str(), SQL_NTS, NULL, 0, NULL, 0);
    if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Error Connecting to DSN: " << std::string(dsn.begin(), dsn.end()) << std::endl;
        ExtractError(SQL_HANDLE_DBC, m_hDbc);
        return false;
    }

    m_bConnected = true;
    std::cout << "Successfully connected to DB: " << std::string(dsn.begin(), dsn.end()) << std::endl;
    return true;
}

void DBManager::Cleanup() {
    if (m_hDbc != SQL_NULL_HDBC) {
        SQLDisconnect(m_hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, m_hDbc);
        m_hDbc = SQL_NULL_HDBC;
    }
    if (m_hEnv != SQL_NULL_HENV) {
        SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
        m_hEnv = SQL_NULL_HENV;
    }
    m_bConnected = false;
}

void DBManager::ExtractError(SQLSMALLINT handleType, SQLHANDLE handle) {
    SQLWCHAR sqlState[6];
    SQLINTEGER nativeError;
    SQLWCHAR messageText[256];
    SQLSMALLINT textLength;
    SQLSMALLINT i = 1;

    while (SQLGetDiagRecW(handleType, handle, i, sqlState, &nativeError, messageText, sizeof(messageText) / sizeof(SQLWCHAR), &textLength) != SQL_NO_DATA) {
        std::wcerr << L"ODBC Error " << i << L": " << sqlState << L", " << messageText << std::endl;
        i++;
    }
}

bool DBManager::Login(const std::string& id, const std::string& pw) {
    if (!m_bConnected) return false;

    SQLHSTMT hStmt;
    SQLRETURN retCode = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &hStmt);
    if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO) return false;

    // SELECT 1 FROM Account WHERE ID = 'id' AND Password = 'pw'
    std::string query = "SELECT 1 FROM Account WHERE ID = '" + id + "' AND Password = '" + pw + "'";
    std::wstring wquery(query.begin(), query.end());

    retCode = SQLExecDirectW(hStmt, (SQLWCHAR*)wquery.c_str(), SQL_NTS);
    
    bool bSuccess = false;
    if (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO) {
        // Fetch the result
        retCode = SQLFetch(hStmt);
        if (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO) {
            bSuccess = true;
        }
    } else {
        ExtractError(SQL_HANDLE_STMT, hStmt);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return bSuccess;
}

bool DBManager::Register(const std::string& id, const std::string& pw) {
    if (!m_bConnected) return false;

    SQLHSTMT hStmt;
    SQLRETURN retCode = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &hStmt);
    if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO) return false;

    // First check if ID already exists
    std::string checkQuery = "SELECT 1 FROM Account WHERE ID = '" + id + "'";
    std::wstring wcheck(checkQuery.begin(), checkQuery.end());
    
    retCode = SQLExecDirectW(hStmt, (SQLWCHAR*)wcheck.c_str(), SQL_NTS);
    if (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO) {
        if (SQLFetch(hStmt) == SQL_SUCCESS || SQLFetch(hStmt) == SQL_SUCCESS_WITH_INFO) {
            // ID already exists
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return false;
        }
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

    // Insert new account
    retCode = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &hStmt);
    if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO) return false;

    std::string insertQuery = "INSERT INTO Account (ID, Password) VALUES ('" + id + "', '" + pw + "')";
    std::wstring winsert(insertQuery.begin(), insertQuery.end());

    retCode = SQLExecDirectW(hStmt, (SQLWCHAR*)winsert.c_str(), SQL_NTS);
    
    bool bSuccess = false;
    if (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO) {
        bSuccess = true;
    } else {
        ExtractError(SQL_HANDLE_STMT, hStmt);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return bSuccess;
}
