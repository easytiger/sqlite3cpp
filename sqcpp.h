/* 
 * File:   sqcpp.h
 * Author: gsteele@nyx.com
 *
 * Created on 08 December 2008, 10:31
 */

#ifndef _SQCPP_H
#define	_SQCPP_H

#include <pthread.h>
#include <memory>
#include <stdexcept>
#include <string>

#include <sqlite3.h>

// todo: specify explicit throws in funcs
using namespace std;

namespace Wombat
{
     namespace Utils
     {
          // main class to control acess to the DB
          class SqliteDB
          {
          public:
               SqliteDB ( );
               SqliteDB ( const char * db_filename);
               ~SqliteDB ( void );

               void open_db (const char * db_filename);
               void close_db ( void );
               bool is_open ( void );

               sqlite3_int64 get_last_row_id (void);
	    
               sqlite3 * get_db_ptr (void);

               void enhance_perf(); // function idea from google gears
               void getCommitLock();
               void releaseCommitLock();

          private:
               sqlite3*         db;
	       pthread_mutex_t  com_mutex;
	      
          };//class SqliteDB

          class SqliteDBException : public runtime_error
          {
          public:
               SqliteDBException (const string& msg = "")
                    : runtime_error(msg) {}
          };

          class SqliteDBErrorException : public SqliteDBException
          {
          public:
          SqliteDBErrorException(const string& msg="", int err=-999)
               :  SqliteDBException (msg) {} //todo overide what()
          };

          // wraps the sqlite3_stmt object and provides facilities for queries.
          class SqliteDBStatement
          {
          public:
	    //    SqliteDBStatement (SqliteDB& db_r);
               SqliteDBStatement (SqliteDB& db_r);
               ~SqliteDBStatement();

               // underlying c object
               sqlite3_stmt* get_stmt_ptr (void);

               void reset (void);
               void reset_bindings (void);
               void reset_all (void);

               // set up a new statement with sql. 
               // call step to execute it
               void prepare (const string sql);
               void prepare (const char * sql);

               // move through results. false if none left or error
               bool step ( void );
	    
               // run statements to completion which don't need
               // results. eg CREATE TABLE. 
               void execDML (const string sql);
               void execDML (const char * sql);
	    
               void begin_transaction (void);
               void commit_transaction (void);
               void rollback_transaction (void);
	    
               // the text binds, integer arg denotes # of chars. -ve use \0
               void bind_text    (int pos, const char * txt, int len=-1);
               void bind_text    (int pos, const string& txt, int len=-1);
               void bind_int     (int pos, int val);
               void bind_int64   (int pos, sqlite3_int64 val);
               void bind_double  (int pos, double val);
               void bind_long    (int pos, long val);

               // overloaded binds, prob just use these instead of the above.
               void bind         (int pos, const char * txt);
               void bind         (int pos, const string& txt);
               void bind         (int pos, int val);
               void bind         (int pos, double val);

               // todo: bind by column name 

               // extracting information after calling step...
               int getColumnAsInt (int pos);
               double getColumnAsDouble (int pos);
               string getColumnAsString (int pos);

               int getColumnCount (void);

               // todo: expose col type information 

          private:
               bool done;
               SqliteDB& db_ref;
               sqlite3_stmt* stmt_p;

               bool in_result_set;


	    
          };// SqliteDBStatement

     }//namespace Utils

}//namespace Wombat

#endif	/* _SQCPP_H */
