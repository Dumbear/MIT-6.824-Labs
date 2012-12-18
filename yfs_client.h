#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>


class yfs_client {
  extent_client *ec;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirent {
    std::string name;
    yfs_client::inum inum;
  };

 private:
  static std::string filename(inum);
  static inum n2i(std::string);
  static yfs_client::status to_status(extent_protocol::status);
  static void join(const std::list<yfs_client::dirent> &entries, std::string &s);
  static void split(const std::string &s, std::list<yfs_client::dirent> &entries);
 public:

  yfs_client(std::string, std::string);

  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &);

  yfs_client::status readdir(yfs_client::inum inum, std::list<yfs_client::dirent> &entries);
  yfs_client::status create(yfs_client::inum parent, std::string name, yfs_client::inum &inum);
  yfs_client::status lookup(yfs_client::inum parent, std::string name, yfs_client::inum &inum);
};

#endif 
