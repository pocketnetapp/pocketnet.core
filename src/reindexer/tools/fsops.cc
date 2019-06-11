
#include "fsops.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <memory>

#include "errors.h"
#include "tools/oscompat.h"
#include "tools/logger.h"

namespace reindexer {
namespace fs {

int MkDirAll(const string &path) {
	char tmp[0x1000], *p = nullptr;
	size_t len;
	int err;

	snprintf(tmp, sizeof(tmp), "%s", path.c_str());
	len = strlen(tmp);
	if (tmp[len - 1] == '/' || tmp[len - 1] == '\\') tmp[len - 1] = 0;
	for (p = tmp + 1; *p; p++) {
		if (*p == '/' || *p == '\\') {
			*p = 0;
			err = mkdir(tmp, S_IRWXU);
			if ((err < 0) && (errno != EEXIST) && (errno != EACCES)) {
                return err;
            }

			*p = '/';
		}
	}
	return ((mkdir(tmp, S_IRWXU) < 0) && errno != EEXIST) ? -1 : 0;
}

int RmDirAll(const string &path) {
#ifndef _WIN32
	return nftw(path.c_str(), [](const char *fpath, const struct stat *, int, struct FTW *) { return ::remove(fpath); }, 64,
				FTW_DEPTH | FTW_PHYS);
#else
	(void)path;
	return 0;
#endif
}

int ReadFile(const string &path, string &content) {
	FILE *f = fopen(path.c_str(), "r");
	if (!f) {
		return -1;
	}
	fseek(f, 0, SEEK_END);
	size_t sz = ftell(f);
	content.resize(sz);
	fseek(f, 0, SEEK_SET);
	auto nread = fread(&content[0], 1, sz, f);
	fclose(f);
	return nread;
}

int ReadDir(const string &path, vector<DirEntry> &content) {
#ifndef _WIN32
	struct dirent *entry;
	auto dir = opendir(path.c_str());

	if (!dir) {
		return -1;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_name[0] == '.') {
			continue;
		}
		bool isDir = entry->d_type == DT_DIR;
		if (entry->d_type == DT_UNKNOWN) {
			struct stat stat;
			if (lstat((path + "/" + entry->d_name).c_str(), &stat) >= 0 && S_ISDIR(stat.st_mode)) {
				isDir = true;
			}
		}
		content.push_back({entry->d_name, isDir});
	};

	closedir(dir);
#else
	HANDLE hFind;
	WIN32_FIND_DATA entry;

	if ((hFind = FindFirstFile((path + "/*.*").c_str(), &entry)) != INVALID_HANDLE_VALUE) {
		do {
			if (entry.cFileName[0] == '.') {
				continue;
			}
			bool isDir = entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
			content.push_back({entry.cFileName, isDir});
		} while (FindNextFile(hFind, &entry));
		FindClose(hFind);
	}

#endif
	return 0;
}

string GetCwd() {
	char buff[FILENAME_MAX];
	return std::string(getcwd(buff, FILENAME_MAX));
}

string GetTempDir() {
#ifdef _WIN32
	char tmpBuf[512];
	*tmpBuf = 0;
	::GetTempPathA(sizeof(tmpBuf), tmpBuf);
	return tmpBuf;
#else
	if (const char *tmpDir = getenv("TMPDIR")) {
		if (tmpDir && *tmpDir) return tmpDir;
	}
	return "/tmp";
#endif
}

string GetHomeDir() {
	if (const char *homeDir = getenv("HOME")) {
		if (homeDir && *homeDir) return homeDir;
	}
	return ".";
}

FileStatus Stat(const string &path) {
#ifdef _WIN32
	struct _stat state;
	if (_stat(path.c_str(), &state) < 0) return StatError;
	return (state.st_mode & _S_IFDIR) ? StatDir : StatFile;
#else
	struct stat state;
	if (stat(path.c_str(), &state) < 0) return StatError;
	return S_ISDIR(state.st_mode) ? StatDir : StatFile;
#endif
}

bool DirectoryExists(const string &directory) {
	if (!directory.empty()) {
#ifdef _WIN32
		if (_access(directory.c_str(), 0) == 0) {
			struct _stat status;
			_stat(directory.c_str(), &status);
			if (status.st_mode & _S_IFDIR) return true;
		}
#else
		if (access(directory.c_str(), F_OK) == 0) {
			struct stat status;
			stat(directory.c_str(), &status);
			if (status.st_mode & S_IFDIR) return true;
		}
#endif
	}
	return false;
}

Error TryCreateDirectory(const string &dir) {
	using reindexer::fs::MkDirAll;
	using reindexer::fs::DirectoryExists;
	using reindexer::fs::GetTempDir;
	if (!dir.empty()) {
		if (!DirectoryExists(dir) && dir != GetTempDir()) {
			if (MkDirAll(dir) < 0) return Error(errLogic, "Could not create '%s'. Reason: %s\n", dir.c_str(), strerror(errno));
#ifdef _WIN32
		} else if (_access(dir.c_str(), 6) < 0) {
#else
		} else if (access(dir.c_str(), R_OK | W_OK) < 0) {
#endif
			return Error(errLogic, "Could not access dir '%s'. Reason: %s\n", dir.c_str(), strerror(errno));
		}
	}
	return 0;
}

string GetDirPath(const string &path) {
	size_t lastSlashPos = path.find_last_of("/\\");
	return path.substr(0, lastSlashPos + 1);
}

Error ChownDir(const string &path, const string &user) {
#ifndef _WIN32
	if (!user.empty() && !path.empty()) {
		struct passwd pwd, *usr;
		char buf[0x4000];

		int res = getpwnam_r(user.c_str(), &pwd, buf, sizeof(buf), &usr);
		if (usr == nullptr) {
			if (res == 0) {
				return Error(errLogic, "Could get uid of user and gid for user `%s`. Reason: user `%s` not found", user.c_str(),
							 user.c_str());
			} else {
				return Error(errLogic, "Could not change user to `%s`. Reason: %s", user.c_str(), strerror(errno));
			}
		}

		if (getuid() != usr->pw_uid || getgid() != usr->pw_gid) {
			if (chown(path.c_str(), usr->pw_uid, usr->pw_gid) < 0) {
				return Error(errLogic, "Could not change ownership for directory '%s'. Reason: %s\n", path.c_str(), strerror(errno));
			}
		}
	}
#else
	(void)path;
	(void)user;
#endif
	return 0;
}

Error ChangeUser(const char *userName) {
#ifndef _WIN32
	struct passwd pwd, *result;
	char buf[0x4000];

	int res = getpwnam_r(userName, &pwd, buf, sizeof(buf), &result);
	if (result == nullptr) {
		if (res == 0) {
			return Error(errLogic, "Could not change user to `%s`. Reason: user `%s` not found", userName, userName);
		} else {
			errno = res;
			return Error(errLogic, "Could not change user to `%s`. Reason: %s", userName, strerror(errno));
		}
	}

	if (setgid(pwd.pw_gid) != 0) return Error(errLogic, "Could not change user to `%s`. Reason: %s", userName, strerror(errno));
	if (setuid(pwd.pw_uid) != 0) return Error(errLogic, "Could not change user to `%s`. Reason: %s", userName, strerror(errno));
#else
	(void)userName;
#endif
	return 0;
}

}  // namespace fs
}  // namespace reindexer
