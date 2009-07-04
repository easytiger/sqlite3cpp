#include "sqcpp.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sqlite3.h>
#include <assert.h>

#include <iostream> // todo remove when done

using namespace std;

Sqlite3::SqliteDB::SqliteDB()
     : db(NULL)
{
 
}

Sqlite3::SqliteDB::SqliteDB(const char* db_filename) 
     : db (NULL)
{
     open_db(db_filename);
     assert(db != NULL);

     pthread_mutex_init(&com_mutex, NULL);
}

Sqlite3::SqliteDB::~SqliteDB()
{
     pthread_mutex_destroy(&com_mutex);
     if (is_open())
          close_db();
}

void
Sqlite3::SqliteDB::open_db(const char* db_filename)
{
     assert(db == NULL);
     if (is_open())
          throw SqliteDBException("Database connection already in use");

     int ret = sqlite3_open(db_filename, &db);

     if (ret == SQLITE_OK)
          enhance_perf();
     else
          throw SqliteDBException("Could not open the database\n");

     assert(db != NULL);
}

void
Sqlite3::SqliteDB::getCommitLock()
{
     pthread_mutex_lock(&com_mutex);
}

void
Sqlite3::SqliteDB::releaseCommitLock()
{
     pthread_mutex_unlock(&com_mutex);
}

void
Sqlite3::SqliteDB::close_db()
{
     if (is_open()) {
          sqlite3_close(db);
          db = NULL;
     }
}

bool
Sqlite3::SqliteDB::is_open()
{
     return (db != NULL);
}

void
Sqlite3::SqliteDB::enhance_perf() {

     //todo: replace with execDML
     assert(db != NULL);
     // increases write speed
     if (sqlite3_exec(db, "PRAGMA synchronous = OFF", NULL, NULL, NULL)
         != SQLITE_OK)
          throw SqliteDBException("Could not set PRAGMA synchronous = OFF");
  
     // decreases db size (and we dont need utf16). May not need this anyway
     if (sqlite3_exec(db, "PRAGMA encoding = \"UTF-8\"", NULL, NULL, NULL)
         != SQLITE_OK)
          throw SqliteDBException("Could not set PRAGMA encoding  = UTF8");
}

sqlite3*
Sqlite3::SqliteDB::get_db_ptr()
{
     return db;
}

sqlite3_int64
Sqlite3::SqliteDB::get_last_row_id()
{
     assert(db != NULL);
  
     return sqlite3_last_insert_rowid(db);

}

Sqlite3::SqliteDBStatement::SqliteDBStatement (SqliteDB& db_r)        
     : db_ref (db_r),
       in_result_set (false)
			
{
     //  assert(db_r != NULL);
}


Sqlite3::SqliteDBStatement::~SqliteDBStatement()
{
     //todo: catch error and thrown if != SQLITE? any benefit?

    
}

sqlite3_stmt* 
Sqlite3::SqliteDBStatement::get_stmt_ptr (void)
{
     return stmt_p;
}

void
Sqlite3::SqliteDBStatement::prepare(const string sql)
{
     prepare (sql.c_str());
}

void
Sqlite3::SqliteDBStatement::prepare(const char * sql)
{
     int ret = 
          sqlite3_prepare_v2(db_ref.get_db_ptr(), sql, -1, &stmt_p, NULL);
 
     if (ret != SQLITE_OK)
          throw SqliteDBErrorException("Could not prepare SQL statement", ret);
}


bool
Sqlite3::SqliteDBStatement::step (void)
{
     /* Return true if we have more results to step through */
     int ret = sqlite3_step(stmt_p); 
  
     if ( ret == SQLITE_ROW )
     {
          in_result_set = true;
     }
     else if ( ret == SQLITE_DONE )
     {
          in_result_set = false;
     }
     else if ( ret == SQLITE_ERROR || 
               ret == SQLITE_MISUSE || 
               ret == SQLITE_BUSY)
     {
          in_result_set = false;
          throw SqliteDBErrorException("Error stepping through results", ret);      
     }
    
     return in_result_set;
}

void
Sqlite3::SqliteDBStatement::execDML (const string sql)
{
     execDML (sql.c_str());
}

void
Sqlite3::SqliteDBStatement::execDML (const char * sql)
{
     //  assert(stmt_p != NULL);/// can be null but is not always? WTF?
     char *err;
     int ret = sqlite3_exec(db_ref.get_db_ptr(), sql, NULL, NULL, &err); 

     if (ret != SQLITE_OK)
     {
          string _err(err);
          throw SqliteDBException(err);
          sqlite3_free(err);//todo reuse this approach elsewhere in the code
          // need to call rollback here?
     }
}


void
Sqlite3::SqliteDBStatement::begin_transaction()
{
     db_ref.getCommitLock();
     execDML("BEGIN TRANSACTION;");
}


void
Sqlite3::SqliteDBStatement::rollback_transaction()
{
     execDML("ROLLBACK TRANSACTION;");
}


void
Sqlite3::SqliteDBStatement::commit_transaction()
{

     execDML("COMMIT TRANSACTION;");
     db_ref.releaseCommitLock();
}


void
Sqlite3::SqliteDBStatement::reset()
{
     // reset doesn't effect bindings
     if (stmt_p != NULL)
     {
          int ret = sqlite3_reset(stmt_p);
     }
}


void
Sqlite3::SqliteDBStatement::reset_bindings()
{
     // reset bindings
     if (stmt_p != NULL)
     {
          int ret = sqlite3_clear_bindings(stmt_p);
     }
}

void
Sqlite3::SqliteDBStatement::reset_all()
{
     reset();
     reset_bindings();
}


void 
Sqlite3::SqliteDBStatement::bind_text(int pos, const char * txt, int len)
{
     int l = 0;
     len == -1 ? l = strlen(txt) : l = len;

     if (sqlite3_bind_text(stmt_p, pos, txt, l, SQLITE_TRANSIENT) != SQLITE_OK)
          throw SqliteDBException("Could not call bind_text");
}

void 
Sqlite3::SqliteDBStatement::bind_text(int pos, const string& txt, int len)
{
     int l = 0;
     len == -1 ? l = txt.length() : l = len;
  
     if (sqlite3_bind_text(stmt_p, pos, txt.c_str(), l, SQLITE_TRANSIENT) != SQLITE_OK)
          throw SqliteDBException("Could not call bind_text");
}


void 
Sqlite3::SqliteDBStatement::bind_int(int pos, int val)
{
     if (sqlite3_bind_int(stmt_p, pos, val) != SQLITE_OK)
     {
          std::cout << "SQLITE ERROR: " <<  sqlite3_bind_int(stmt_p, pos, val) << std::endl;
          throw SqliteDBException("Could not call bind_int");
     }
}


void 
Sqlite3::SqliteDBStatement::bind_int64(int pos, sqlite3_int64 val)
{
     if (sqlite3_bind_int64(stmt_p, pos, val) != SQLITE_OK)
          throw SqliteDBException("Could not call bind_int64");
}


void 
Sqlite3::SqliteDBStatement::bind_double(int pos, double val)
{
     if (sqlite3_bind_double(stmt_p, pos, val) != SQLITE_OK)
          throw SqliteDBException("Could not call bind_double");
}

void 
Sqlite3::SqliteDBStatement::bind_long(int pos, long val)
{
     if (sqlite3_bind_int64(stmt_p, pos, val) != SQLITE_OK)
          throw SqliteDBException("Could not call bind_double");
}


void
Sqlite3::SqliteDBStatement::bind(int pos, const char *txt)
{
     bind_text(pos, txt);
}

void
Sqlite3::SqliteDBStatement::bind(int pos, const string& txt)
{
     bind_text(pos, txt);
}

void
Sqlite3::SqliteDBStatement::bind(int pos, int val)
{
     bind_int(pos, val);
}


void
Sqlite3::SqliteDBStatement::bind(int pos, double val)
{
     bind_double(pos, val);
}

int 
Sqlite3::SqliteDBStatement::getColumnAsInt (int pos)
{
  
     if (in_result_set)
     {
          int ret = sqlite3_column_int(stmt_p, pos);
          return ret;
     }
     else
     {
          throw SqliteDBException("getColumnAsInt: not currently in a result set");      
     }
}

string
Sqlite3::SqliteDBStatement::getColumnAsString (int pos)
{
     if (in_result_set)
     {
          //could this lead to a memory leak??
          return string( (const char *)sqlite3_column_text(stmt_p, pos));
     }
     else
     {
          throw SqliteDBException("getColumnAsInt: not currently in a result set");      
     }
}

int 
Sqlite3::SqliteDBStatement::getColumnCount ()
{
     int ret = -1;
  
     if (in_result_set)
     {
          ret =  sqlite3_column_count(stmt_p);
     }
     else
     {
          throw SqliteDBException("getColumnAsInt: not currently in a result set");      
     }

     return ret;
}
