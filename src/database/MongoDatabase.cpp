#include "DatabaseBackend.h"
#include "DBBackendFactory.h"
#include "DatabaseServer.h"

#include "core/global.h"
#include "util/DatagramIterator.h"
#include "util/Datagram.h"

#include <client/dbclient.h>
#include <bson/bson.h>

using namespace std;
using namespace mongo;

static ConfigVariable<string> server("server", "localhost", db_backend_config);
static ConfigVariable<string> database("database", "test", db_backend_config);
static ConfigVariable<string> username("username", "", db_backend_config);
static ConfigVariable<string> password("password", "", db_backend_config);

// These are helper functions to convert between BSONElement and packed Bamboo
// field values.

// TODO: Right now they just store the packed Bamboo field data in a BSON blob
//       verbatim. They need to do an in-depth conversion so that the database
//       is accessible by other tools and tolerant of changes to the .dc file.
static void unpack_bson(const dclass::Field *field,
                        const std::vector<uint8_t> data,
                        BSONObjBuilder &ob)
{
	ob.appendBinData(field->get_name(), data.size(), BinDataGeneral, data.data());
}

static void pack_bson(const dclass::Field *field,
                      const BSONElement &element,
                      std::vector<uint8_t> data)
{
	int len;
	const char *rawdata = element.binData(len);
	data.resize(len);
	memcpy(data.data(), rawdata, len);
}

class MongoDatabase : public DatabaseBackend
{
	public:
		MongoDatabase(ConfigNode dbeconfig, doid_t min_id, doid_t max_id) :
			DatabaseBackend(dbeconfig, min_id, max_id)
		{
			stringstream log_name;
			log_name << "Database-MongoDB" << "(Range: [" << min_id << ", " << max_id << "])";
			m_log = new LogCategory("mongodb", log_name.str());

			// Init connection.
			string error;

			// TODO: This only creates a single connection. When this class is
			// made multithreaded, we will need a connection pool instead.
			if(!m_conn.connect(server.get_rval(m_config), error))
			{
				m_log->fatal() << "Connection failure: " << error << endl;
				exit(1);
			}

			database.get_rval(m_config);

			string u = username.get_rval(m_config);
			string p = password.get_rval(m_config);
			if(!u.empty() && !p.empty())
			{
				if(!m_conn.auth(m_db, u, p, error))
				{
					m_log->fatal() << "Authentication failure: " << error << endl;
					exit(1);
				}
			}

			// Init the collection names:
			stringstream obj_collection;
			obj_collection << m_db << ".astron.objects";
			string m_obj_collection = obj_collection.str();
			stringstream global_collection;
			global_collection << m_db << ".astron.globals";
			string m_global_collection = global_collection.str();

			// Init the globals collection/document:
			BSONObj globals = BSON("_id" << "GLOBALS" <<
			                       "doid" << BSON(
				                       "monotonic" << min_id
			                       ));
			m_conn.insert(m_global_collection, globals);
		}

		virtual void submit(DBOperation *operation)
		{
			// TODO: This should run in a separate thread.
			handle_operation(operation);
		}

	private:
		LogCategory *m_log;

		DBClientConnection m_conn;
		string m_db;
		string m_obj_collection;
		string m_global_collection;

		void handle_operation(DBOperation *operation)
		{
			// First, figure out what kind of operation it is, and dispatch:
			if(operation->m_type == DBOperation::OperationType::CREATE_OBJECT)
			{
				handle_create(operation);
			}
			else if(operation->m_type == DBOperation::OperationType::DELETE_OBJECT)
			{
				handle_delete(operation);
			}
			else if(operation->m_type == DBOperation::OperationType::GET_OBJECT ||
			        operation->m_type == DBOperation::OperationType::GET_FIELDS)
			{
				handle_get(operation);
			}
			else if(operation->m_type == DBOperation::OperationType::MODIFY_FIELDS)
			{
				handle_modify(operation);
			}
		}

		void handle_create(DBOperation *operation)
		{
			// TODO: CREATE
		}

		void handle_delete(DBOperation *operation)
		{
			// TODO: DELETE
		}

		void handle_get(DBOperation *operation)
		{
			// TODO: GET
		}

		void handle_modify(DBOperation *operation)
		{
			// TODO: MODIFY
		}

		// This function is used by handle_create to get a fresh DOID assignment.
		doid_t assign_doid()
		{
			// TODO: DOID assignment
			return 0;
		}
};

DBBackendFactoryItem<MongoDatabase> mongodb_factory("mongodb");