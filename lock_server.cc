// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_server::lock_server():
  nacquire (0)
{
  pthread_mutex_init(&lock_table_mutex, NULL);
}

lock_server::~lock_server()
{
  pthread_mutex_destroy(&lock_table_mutex);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, lock_protocol::status &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("acquire request from clt %d\n", clt);

  pthread_mutex_t *mutex = NULL;
  pthread_mutex_lock(&lock_table_mutex);
  std::map<lock_protocol::lockid_t, pthread_mutex_t *>::iterator it = lock_table.find(lid);
  if (it == lock_table.end()) {
    mutex = new pthread_mutex_t();
    pthread_mutex_init(mutex, NULL);
    lock_table[lid] = mutex;
  } else {
    mutex = it->second;
  }
  pthread_mutex_unlock(&lock_table_mutex);
  pthread_mutex_lock(mutex);

  r = lock_protocol::OK;
  return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, lock_protocol::status &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("release request from clt %d\n", clt);

  pthread_mutex_lock(&lock_table_mutex);
  std::map<lock_protocol::lockid_t, pthread_mutex_t *>::iterator it = lock_table.find(lid);
  VERIFY (it != lock_table.end() && it->second != NULL);
  pthread_mutex_unlock(it->second);
  pthread_mutex_unlock(&lock_table_mutex);

  r = lock_protocol::OK;
  return ret;
}
