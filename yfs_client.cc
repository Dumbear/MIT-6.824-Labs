// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);

}

yfs_client::inum
yfs_client::n2i(std::string n)
{
  std::istringstream ist(n);
  unsigned long long finum;
  ist >> finum;
  return finum;
}

std::string
yfs_client::filename(inum inum)
{
  std::ostringstream ost;
  ost << inum;
  return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
  if(inum & 0x80000000)
    return true;
  return false;
}

bool
yfs_client::isdir(inum inum)
{
  return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
  int r = OK;

  printf("getfile %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }

  fin.atime = a.atime;
  fin.mtime = a.mtime;
  fin.ctime = a.ctime;
  fin.size = a.size;
  printf("getfile %016llx -> sz %llu\n", inum, fin.size);

 release:

  return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;

  printf("getdir %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

 release:
  return r;
}

yfs_client::status yfs_client::to_status(extent_protocol::status s) {
  if (s == extent_protocol::OK) {
    return yfs_client::OK;
  }
  if (s == extent_protocol::RPCERR) {
    return yfs_client::RPCERR;
  }
  if (s == extent_protocol::NOENT) {
    return yfs_client::NOENT;
  }
  if (s == extent_protocol::IOERR) {
    return yfs_client::IOERR;
  }
  return yfs_client::RPCERR;
}

void yfs_client::join(const std::list<yfs_client::dirent> &entries, std::string &s) {
  s.clear();
  for (std::list<yfs_client::dirent>::const_iterator i = entries.begin(); i != entries.end(); ++i) {
    s += filename(i->inum);
    s += '\0';
    s += i->name;
    s += '\0';
  }
}

void yfs_client::split(const std::string &s, std::list<yfs_client::dirent> &entries) {
  entries.clear();
  size_t last = 0;
  dirent entry;
  entry.name = "prename";
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '\0') {
      if (entry.name.empty()) {
        entry.name = s.substr(last, i - last);
        entries.push_back(entry);
      } else {
        entry.inum = n2i(s.substr(last, i - last));
        entry.name.clear();
      }
      last = i + 1;
    }
  }
}

yfs_client::status yfs_client::readdir(yfs_client::inum inum, std::list<yfs_client::dirent> &entries) {
  printf("readdir %016llx\n", inum);

  if (!isdir(inum)) {
    return NOENT;
  }

  yfs_client::status r = OK;

  std::string s;
  if ((r = to_status(ec->get(inum, s))) != yfs_client::OK) {
    return r;
  }

  split(s, entries);

  return r;
}

yfs_client::status yfs_client::create(yfs_client::inum parent, std::string name, yfs_client::inum &inum) {
  printf("create '%s' in %016llx\n", name.c_str(), inum);

  if (!isdir(parent)) {
    return NOENT;
  }

  yfs_client::status r = OK;

  std::string s;
  if ((r = to_status(ec->get(parent, s))) != yfs_client::OK) {
    return r;
  }

  std::list<yfs_client::dirent> entries;
  split(s, entries);
  for (std::list<yfs_client::dirent>::iterator i = entries.begin(); i != entries.end(); ++i) {
    if (i->name == name) {
      return EXIST;
    }
  }

  inum = (rand() & 0xFFFFFFFF) | 0x80000000; /* may collision */
  if ((r = to_status(ec->put(inum, std::string()))) != yfs_client::OK) {
    return r;
  }

  yfs_client::dirent entry;
  entry.inum = inum;
  entry.name = name;
  entries.push_back(entry);
  join(entries, s);
  if ((r = to_status(ec->put(parent, s))) != yfs_client::OK) {
    return r;
  }

  return r;
}

yfs_client::status yfs_client::lookup(yfs_client::inum parent, std::string name, yfs_client::inum &inum) {
  printf("lookup '%s' in %016llx\n", name.c_str(), inum);

  if (!isdir(parent)) {
    return NOENT;
  }

  yfs_client::status r = OK;

  std::string s;
  if ((r = to_status(ec->get(parent, s))) != yfs_client::OK) {
    return r;
  }

  std::list<yfs_client::dirent> entries;
  split(s, entries);
  for (std::list<yfs_client::dirent>::iterator i = entries.begin(); i != entries.end(); ++i) {
    if (i->name == name) {
      inum = i->inum;
      return EXIST;
    }
  }

  return r;
}

yfs_client::status yfs_client::setsize(yfs_client::inum inum, unsigned long long size, bool no_trunc) {
  printf("setsize %016llx\n", inum);

  if (!isfile(inum)) {
    return NOENT;
  }

  yfs_client::status r = OK;

  std::string s;
  if ((r = to_status(ec->get(inum, s))) != yfs_client::OK) {
    return r;
  }
  if (s.size() >= size) {
    if (!no_trunc) {
      s = s.substr(0, size);
    }
  } else {
    s += std::string(size - s.size(), '\0');
  }

  if ((r = to_status(ec->put(inum, s))) != yfs_client::OK) {
    return r;
  }

  return r;
}

yfs_client::status yfs_client::read(yfs_client::inum inum, unsigned long long size, unsigned long long offset, std::string &s) {
  printf("read %016llx\n", inum);

  if (!isfile(inum)) {
    return NOENT;
  }

  yfs_client::status r = OK;

  if ((r = to_status(ec->get(inum, s))) != yfs_client::OK) {
    return r;
  }

  if (offset >= s.size()) {
    s.clear();
  } else {
    s = s.substr(offset, size);
  }

  return r;
}

yfs_client::status yfs_client::write(yfs_client::inum inum, unsigned long long size, unsigned long long offset, std::string str) {
  printf("write %016llx\n", inum);

  if (!isfile(inum)) {
    return NOENT;
  }

  yfs_client::status r = OK;

  if ((r = setsize(inum, offset + size, true)) != yfs_client::OK) {
    return r;
  }

  std::string s;
  if ((r = to_status(ec->get(inum, s))) != yfs_client::OK) {
    return r;
  }
  s.replace(offset, size, str);

  if ((r = to_status(ec->put(inum, s))) != yfs_client::OK) {
    return r;
  }

  return r;
}
