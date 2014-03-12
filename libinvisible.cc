#define __USE_BSD 1

#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <clocale>
#include <climits>
#include <cctype>

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm> 
#include <functional> 

#include <unistd.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

static std::unordered_set<std::string> global_invisibles;
static std::unordered_map<DIR *, std::unordered_set<std::string> > directory_invisibles;

typedef DIR *(*opendir_fptr)(const char *name);
typedef DIR *(*fdopendir_fptr)(int fd);
typedef struct dirent *(*readdir_fptr)(DIR *dir);
typedef struct dirent64 *(*readdir64_fptr)(DIR *dir);
typedef int (*closedir_fptr)(DIR *dir);

static opendir_fptr next_opendir;
static fdopendir_fptr next_fdopendir;
static readdir_fptr next_readdir;
static readdir64_fptr next_readdir64;
static closedir_fptr next_closedir;

inline bool file_exists(const char *path)
{
  struct stat buffer;   
  return (stat(path, &buffer) == 0); 
}

static bool get_user_invisibles_path(char *path_buffer)
{
  char *prefix;

  prefix = getenv("XDG_CONFIG_HOME");
  if(prefix) {
    snprintf(path_buffer, (PATH_MAX - 1), "%s/invisible", prefix);
    return true;
  }

  prefix = getenv("HOME");
  if(prefix) {
    snprintf(path_buffer, (PATH_MAX - 1), "%s/.config/invisible", prefix);
    return true;
  }

  return false;
}

static inline std::string& ltrim(std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

static inline std::string& rtrim(std::string& s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

static bool load_invisibles_list(std::unordered_set<std::string>& inv_set, const std::string& path)
{
  std::ifstream infile(path);

  if(!infile)
    return false;

  std::string line;
  while(getline(infile, line)) {
    line = line.substr(0, line.find_first_of("#"));
    line = rtrim(ltrim(line));

    if(line.empty())
      continue;

    inv_set.insert(line);
  }

  return true;
}

void invisible_init (void) __attribute((constructor));
void invisible_init (void)
{
  next_opendir   = reinterpret_cast<opendir_fptr>(reinterpret_cast<intptr_t>(dlsym(RTLD_NEXT, "opendir")));
  next_fdopendir = reinterpret_cast<fdopendir_fptr>(reinterpret_cast<intptr_t>(dlsym(RTLD_NEXT, "fdopendir")));
  next_readdir   = reinterpret_cast<readdir_fptr>(reinterpret_cast<intptr_t>(dlsym(RTLD_NEXT, "readdir")));
  next_readdir64 = reinterpret_cast<readdir64_fptr>(reinterpret_cast<intptr_t>(dlsym(RTLD_NEXT, "readdir64")));
  next_closedir  = reinterpret_cast<closedir_fptr>(reinterpret_cast<intptr_t>(dlsym(RTLD_NEXT, "closedir")));

  load_invisibles_list(global_invisibles, "/etc/invisible");

  char user_invisibles_path[PATH_MAX];
  if(get_user_invisibles_path(user_invisibles_path) && file_exists(user_invisibles_path))
    load_invisibles_list(global_invisibles, user_invisibles_path);
}

inline bool is_hidden_metafile(struct dirent *entry) {
  return (entry->d_type == DT_REG && strcmp(".hidden", entry->d_name) == 0);
}

inline bool is_invisible(DIR *dir, const char *d_name) {
  return (directory_invisibles[dir].count(d_name) == 1);
}

static bool get_original_dir_path(char *dest_path_buf, DIR *dir) {
  char fd_link_path_buf[PATH_MAX];
  snprintf(fd_link_path_buf, (PATH_MAX - 1), "/proc/self/fd/%d", dirfd(dir));

  return readlink(fd_link_path_buf, dest_path_buf, (PATH_MAX - 1)) > 0;
}

static DIR *opendir_common(DIR *dir)
{
  if(!dir)
  	return dir;

  char own_path[PATH_MAX];
  get_original_dir_path(own_path, dir);

  directory_invisibles.emplace(dir, std::unordered_set<std::string>());
  std::unordered_set<std::string>& own_invisibles = directory_invisibles[dir];

  char own_hidden_file_path[PATH_MAX];
  snprintf(own_hidden_file_path, (PATH_MAX - 1), "%s/.hidden", own_path);

  if(file_exists(own_hidden_file_path))
    load_invisibles_list(own_invisibles, own_hidden_file_path);

  return dir;
}

int closedir(DIR *dir)
{
  directory_invisibles.erase(dir);
  return next_closedir(dir);
}

struct dirent *readdir (DIR *dir)
{
  struct dirent *dirent;

  while(dirent = next_readdir(dir))
  {
    if(!is_invisible(dir, dirent->d_name))
      break;
  }

  return (struct dirent *) dirent;
}

struct dirent64 *readdir64 (DIR *dir)
{
  int fd;
  struct dirent64 *dirent;

  while(dirent = next_readdir64(dir))
  {
    if(!is_invisible(dir, dirent->d_name))
      break;
  }

  return (struct dirent64 *) dirent;
}

DIR *opendir (const char *name)
{
  return opendir_common(next_opendir(name));
}

DIR *fdopendir (int fd)
{
  return opendir_common(next_fdopendir(fd));
}

