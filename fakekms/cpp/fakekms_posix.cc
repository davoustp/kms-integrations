#include <csignal>

#include "absl/cleanup/cleanup.h"
#include "absl/strings/str_format.h"
#include "fakekms/cpp/fakekms.h"

namespace fakekms {
namespace {

class PosixServer : public Server {
 public:
  static absl::StatusOr<std::unique_ptr<PosixServer>> New(
      absl::string_view flags);

  PosixServer(std::string listen_addr, pid_t pid)
      : Server(listen_addr), pid_(pid) {}

  ~PosixServer() { CHECK_EQ(kill(pid_, SIGINT), 0); }

 private:
  pid_t pid_;
};

absl::Status PosixErrorToStatus(absl::string_view prefix) {
  return absl::InternalError(
      absl::StrFormat("%s: %s", prefix, strerror(errno)));
}

}  // namespace

absl::StatusOr<std::unique_ptr<Server>> Server::New() {
  int fd[2];
  if (pipe(fd) == -1) {
    return PosixErrorToStatus("unable to create output pipe");
  }
  absl::Cleanup c = [&] {
    CHECK_EQ(close(fd[0]), 0);
    CHECK_EQ(close(fd[1]), 0);
  };

  pid_t pid = fork();
  switch (pid) {
    // fork failure
    case -1: {
      return PosixErrorToStatus("failure forking");
    }

    // post-fork child
    case 0: {
      if (dup2(fd[1], STDOUT_FILENO) == -1) {
        exit(1);
      }

      // we'll be replacing the executable, so cleanup must happen manually
      c.~Cleanup();

      std::string bin_path = BinaryLocation();
      execl(bin_path.c_str(), bin_path.c_str(), (char*)0);

      // the previous line replaces the executable, so this
      // line shouldn't be reached
      exit(2);
    }

    // post-fork parent
    default: {
      FILE* file = fdopen(fd[0], "r");
      if (!file) {
        return PosixErrorToStatus("error opening pipe");
      }

      char* line = nullptr;
      size_t len = 0;
      if (getline(&line, &len, file) == -1) {
        free(line);
        return PosixErrorToStatus("failure reading address");
      }

      std::string address(line, len);
      free(line);
      return absl::make_unique<PosixServer>(address, pid);
    }
  }
}

}  // namespace fakekms