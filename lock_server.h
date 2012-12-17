// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include <map>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"

class lock_server {

 protected:
  int nacquire;
  /* mutex for $lock_table */
  pthread_mutex_t lock_table_mutex;
  /* table of all the locks, each maps to a single mutex */
  std::map<lock_protocol::lockid_t, pthread_mutex_t *> lock_table;

 public:
  lock_server();
  ~lock_server();
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, lock_protocol::status &);
  lock_protocol::status release(int clt, lock_protocol::lockid_t lid, lock_protocol::status &);
};

#endif 







