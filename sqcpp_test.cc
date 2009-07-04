#include <memory>
#include <iostream>
#include "sqcpp.h"
#include <ctime>
#include <cstdio>
#include <cstdlib>

// simple test app to hopefull call every single function in the API
// has no real purpose at all

using namespace Wombat::Utils;
using namespace std;

int main(int argc, char ** argv)
{
  if (argc != 3)
    {
      cout << "Two arguments required: db_name.sb <iterations> " << endl;
      return -1;
    }

  cout << "Number of iterations: " << argv[2] << endl;
  SqliteDB db_test(argv[1]);
  SqliteDBStatement stmt_insert(db_test);

  stmt_insert.execDML ("CREATE TABLE IF NOT EXISTS tbl_test \
     (id INTEGER PRIMARY KEY, s_time VARCHAR(40), e_time VARCHAR(40));");
 
  stmt_insert.prepare("INSERT INTO tbl_test VALUES(NULL, ?, ?); ");

  // i was able to get ~220k rows/second commited with -O2
  // transactions massivley speed up iterative commits rather w

  int x;
  stmt_insert.begin_transaction();
  for (x = 0; x < atoi(argv[2]); ++x)
    {
      stmt_insert.bind(1, "Gerry Steele");
      stmt_insert.bind(2, x);

      stmt_insert.step();     
      stmt_insert.reset();

      //  cout << "Primary Key of Last commit: " 
      //   << db_test.get_last_row_id() << endl;
    }

    stmt_insert.commit_transaction();

    // one single entry
    stmt_insert.begin_transaction();
    stmt_insert.prepare("INSERT INTO tbl_test VALUES(NULL, ?, ?); ");
    stmt_insert.bind(1, "TEST PERSON");
    stmt_insert.bind(2, x);
    stmt_insert.step();
    stmt_insert.commit_transaction();

    // reuse this statement object again:
    stmt_insert.reset_all();
    //    stmt_insert.prepare("select * from tbl_test where age <= 100;");
    
    /*
    while (stmt_insert.step())
      {
	
	cout << "Number of cols: " << stmt_insert.getColumnCount() << endl;

	cout << "steping through result set with" 
	     << " Primary Key: " << stmt_insert.getColumnAsInt(0) 
	     << " Name: " << stmt_insert.getColumnAsString(1)
	     << endl;
	
	     }*/
}
