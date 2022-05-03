#include <iostream>
#include <pequena/database/sqlite.h>

// QUERIES
std::string create = "CREATE TABLE PERSON("  \
"ID INT PRIMARY KEY     NOT NULL," \
"NAME           TEXT    NOT NULL," \
"AGE            INT     NOT NULL);";

std::string insert = "INSERT INTO PERSON (ID,NAME,AGE) "  \
"VALUES (1, @name0, 33); " \
"INSERT INTO PERSON (ID,NAME,AGE) "  \
"VALUES (2, 'Justus', 29); " \
"INSERT INTO PERSON (ID,NAME,AGE) "  \
"VALUES (3, @name1, 55); " \
"INSERT INTO PERSON (ID,NAME,AGE) "  \
"VALUES (4, 'Elisa', 30); ";


std::string insert2 = "INSERT INTO PERSON (ID,NAME,AGE) "  \
"VALUES (5, 'Sakari', 13); " \
"INSERT INTO PERSON (ID,NAME,AGE) "  \
"VALUES (6, 'Tami', @age0); " \
"INSERT INTO PERSON (ID,NAME,AGE) "  \
"VALUES (7, 'Severus', @age1); " \
"INSERT INTO PERSON (ID,NAME,AGE) "  \
"VALUES (8, 'Max', 30); ";

std::string select = "SELECT * from PERSON";
std::string drop = " DROP TABLE PERSON";


// structure for select results
struct Person
{
	int ID = -1;
	std::string NAME = "";
	int AGE = 0;

	// setuo reflector
	DATABASE_COLUMNS(ID, NAME, AGE)
};

int main()
{
	// Open database connection
	peq::database::SQLiteConnection dbConnection("testdb.db");

	if (dbConnection.open())
	{
		// Create table
		peq::database::SQLiteQuery q0(&dbConnection, create);
		if (q0.executeNoResult())
		{
			std::cout << "Table created" << std::endl;
		}
		else
		{
			std::cout << "Create error: " << q0.lastError().text << std::endl;
		}

		//  Run transaction and rollback it
		{
			peq::database::SQLiteTransaction transaction(&dbConnection);
			if (transaction.begin())
			{
				std::cout << "transaction begin" << std::endl;

				peq::database::SQLiteQuery q1(&dbConnection, insert);
				q1.bind("@name0", "TESTINIMI");
				q1.bind("@name1", "TESTINIMI2");

				if (q1.executeNoResult())
				{
					std::cout << "Insert1 ok" << std::endl;
				}
				else
				{
					std::cout << "Insert1 error: " << q1.lastError().text << std::endl;
				}

				peq::database::SQLiteQuery q2(&dbConnection, insert2);
				q2.bind("@age0", 255);
				q2.bind("@age1", 399);
				if (q2.executeNoResult())
				{
					std::cout << "Insert2 ok" << std::endl;
				}
				else
				{
					std::cout << "Insert2error: " << q1.lastError().text << std::endl;
				}

				// roll back
				if (transaction.rollback())
				{
					std::cout << "transaction rollback" << std::endl;
				}
			}
		}
		// Select data, should return 0 rows
		{
			peq::database::SQLiteQuery q2(&dbConnection, select);
			auto result = q2.execute();
			if (result.has_value()) {

				std::cout << "Select returned: " << result->rowCount() << " rows" << std::endl;
			}
		}

		//  Run transaction and commit it
		{
			// Transactions: commit
			peq::database::SQLiteTransaction transaction(&dbConnection);
			if (transaction.begin())
			{
				std::cout << "transaction begin" << std::endl;

				peq::database::SQLiteQuery q1(&dbConnection, insert);
				q1.bind("@name0", "TEST NAME 0");
				q1.bind("@name1", "TEST NAME 1");
				if (q1.executeNoResult())
				{
					std::cout << "Insert1 ok" << std::endl;
				}
				else
				{
					std::cout << "Insert1 error: " << q1.lastError().text << std::endl;
				}

				peq::database::SQLiteQuery q2(&dbConnection, insert2);
				q2.bind("@age0", 255);
				q2.bind("@age1", 399);
				if (q2.executeNoResult())
				{
					std::cout << "Insert2 ok" << std::endl;
				}
				else
				{
					std::cout << "Insert2 error: " << q1.lastError().text << std::endl;
				}


				// Parameters just for fun
				peq::database::SQLiteQuery q3(&dbConnection, "INSERT INTO PERSON(ID, NAME, AGE) VALUES (@id, @name, @age);");
				q3.bind("@id", 88);
				q3.bind("@name", "kalakeitto");
				q3.bind("@age", 950);
				if (q3.executeNoResult())
				{
					std::cout << "Insert3 with parameters ok" << std::endl;
				}
				else
				{
					std::cout << "Insert2 error: " << q1.lastError().text << std::endl;
				}

				// commit
				if (transaction.commit())
				{
					std::cout << "transaction commit" << std::endl;
				}
			}
		}
		// Select data, should return 8 rows
		// Use reflector to fill Person struct
		{
			peq::database::SQLiteQuery q2(&dbConnection, select);
			auto result = q2.execute();
			if (result.has_value())
			{
				std::cout << "Select returned: " << result->rowCount() << " rows" << std::endl;
				auto all = result->all<Person>();
				for (auto& it : all)
				{
					std::cout << it.ID << " " << it.NAME << " " << it.AGE << std::endl;
				}
			}
		}


		{
			// Parameters just for fun
			peq::database::SQLiteQuery q3(&dbConnection, "SELECT * from PERSON where ID < @l");
			q3.bind("@l", 900);
			auto r = q3.execute();
		}
		// Drop table
		{
			peq::database::SQLiteQuery q3(&dbConnection, drop);
			if (q3.executeNoResult())
			{
				std::cout << "Drop table ok" << std::endl;
			}
		}
	}
	return 0;
}