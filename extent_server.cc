// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extent_server::extent_server() {
  pthread_mutex_init(&extent_table_mutex, NULL);
  int tmp;
  put(1, std::string(), tmp);
}


int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  if (pthread_mutex_lock(&extent_table_mutex) != 0) {
    return extent_protocol::IOERR;
  }

  extent_protocol::status ret = extent_protocol::OK;

  std::map<extent_protocol::extentid_t, extent>::iterator it = extent_table.find(id);
  extent &e = extent_table[id];
  e.content = buf;
  e.attribute.mtime = e.attribute.ctime = time(NULL);
  if (it == extent_table.end()) {
    e.attribute.atime = e.attribute.mtime;
  }
  e.attribute.size = e.content.size();

  pthread_mutex_unlock(&extent_table_mutex);
  return ret;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  if (pthread_mutex_lock(&extent_table_mutex) != 0) {
    return extent_protocol::IOERR;
  }

  extent_protocol::status ret = extent_protocol::OK;

  std::map<extent_protocol::extentid_t, extent>::iterator it = extent_table.find(id);
  if (it == extent_table.end()) {
    ret = extent_protocol::NOENT;
  } else {
    extent &e = it->second;
    buf = e.content;
    e.attribute.atime = time(NULL);
  }

  pthread_mutex_unlock(&extent_table_mutex);
  return ret;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
  if (pthread_mutex_lock(&extent_table_mutex) != 0) {
    return extent_protocol::IOERR;
  }

  extent_protocol::status ret = extent_protocol::OK;

  std::map<extent_protocol::extentid_t, extent>::iterator it = extent_table.find(id);
  if (it == extent_table.end()) {
    ret = extent_protocol::NOENT;
  } else {
    extent &e = it->second;
    a = e.attribute;
  }

  pthread_mutex_unlock(&extent_table_mutex);
  return ret;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  if (pthread_mutex_lock(&extent_table_mutex) != 0) {
    return extent_protocol::IOERR;
  }

  extent_table.erase(id);

  pthread_mutex_unlock(&extent_table_mutex);
  return extent_protocol::OK;
}

