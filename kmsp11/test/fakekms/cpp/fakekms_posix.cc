#include <csignal>

#include "absl/strings/str_format.h"
#include "gtest/gtest.h"
#include "kmsp11/test/fakekms/cpp/fakekms.h"
#include "kmsp11/test/runfiles.h"
#include "kmsp11/util/cleanup.h"
#include "kmsp11/util/status_macros.h"

namespace kmsp11 {

namespace {

class PosixFakeKms : public FakeKms {
 public:
  static StatusOr<std::unique_ptr<PosixFakeKms>> New();

  PosixFakeKms(std::string listen_addr, pid_t pid)
      : FakeKms(listen_addr), pid_(pid) {}

  ~PosixFakeKms() { CHECK_EQ(kill(pid_, SIGINT), 0); }

 private:
  pid_t pid_;
};

static absl::Status PosixErrorToStatus(absl::string_view prefix) {
  return absl::InternalError(
      absl::StrFormat("%s: %s", prefix, strerror(errno)));
}

StatusOr<std::unique_ptr<PosixFakeKms>> PosixFakeKms::New() {
  int fd[2];
  if (pipe(fd) == -1) {
    return PosixErrorToStatus("unable to create output pipe");
  }
  Cleanup c([&fd]() {
    CHECK_EQ(close(fd[0]), 0);
    CHECK_EQ(close(fd[1]), 0);
  });

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

      std::string bin_path = RunfileLocation(
          "com_google_kmstools/kmsp11/test/fakekms/main/fakekms_/fakekms");
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
      return absl::make_unique<PosixFakeKms>(address, pid);
    }
  }
}

}  // namespace

StatusOr<std::unique_ptr<FakeKms>> FakeKms::New() {
  ASSIGN_OR_RETURN(std::unique_ptr<PosixFakeKms> fake, PosixFakeKms::New());
  return std::unique_ptr<FakeKms>(std::move(fake));
}

}  // namespace kmsp11