#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>

#include <picojson.h>

static bool g_signaled = false;

void OnSIGINT(int signum) {
  g_signaled = true;
}

void InstallSignalHandler() {
  signal(SIGINT, OnSIGINT);
}

void CreateAiProcess(int argc, char** argv, pid_t* pid, int* fd) {
  int pipefd[2];
  pipe(pipefd);
  *pid = fork();
  if (*pid != 0) {
    close(pipefd[1]);
    *fd = pipefd[0];
    return;
  }
  dup2(pipefd[1], 1);
  close(pipefd[0]);
  close(pipefd[1]);
  execvp(argv[0], argv);
  exit(28);
}

int GetSolutionScore(const std::string& json_text) {
  std::istringstream json_in(json_text);
  picojson::value solutions_value;
  if (!(json_in >> solutions_value)) {
    return -1;
  }
  if (!solutions_value.is<picojson::array>()) {
    return -1;
  }
  const picojson::array& solutions = solutions_value.get<picojson::array>();
  if (solutions.size() != 1) {
    return -1;
  }
  const picojson::value& solution_value = solutions[0];
  if (!solution_value.is<picojson::object>()) {
    return -1;
  }
  const picojson::object& solution = solution_value.get<picojson::object>();
  if (solution.count("_score") == 0) {
    return -1;
  }
  const picojson::value& score_value = solution.find("_score")->second;
  if (!score_value.is<int64_t>()) {
    return -1;
  }
  return static_cast<int>(score_value.get<int64_t>());
}

int main(int argc, char** argv) {
  InstallSignalHandler();

  pid_t pid;
  int fd;
  CreateAiProcess(argc - 1, argv + 1, &pid, &fd);

  int returncode = 0;
  std::string best_json_text = "{\"_score\":0,\"tag\":\"sentinel\"}";
  int best_score = 0;
  std::string json_buf;

  for (;;) {
    if (g_signaled) {
      kill(pid, SIGKILL);
      while (waitpid(pid, NULL, 0) != 0 && errno == EINTR);
      returncode = 28;
      break;
    }

    int status;
    if (waitpid(pid, &status, WNOHANG) > 0) {
      returncode = WEXITSTATUS(status);
      break;
    }

    char tmp_buf[4096];
    int read_size = read(fd, tmp_buf, sizeof(tmp_buf));
    if (read_size > 0) {
      json_buf += std::string(tmp_buf, read_size);
    }

    std::string::size_type pos;
    while ((pos = json_buf.find("\n")) != std::string::npos) {
      std::string json_text = json_buf.substr(0, pos);
      json_buf = json_buf.substr(pos + 1);
      int score = GetSolutionScore(json_text);
      if (score > best_score) {
        best_score = score;
        best_json_text = json_text;
      }
    }
  }

  std::cout << best_json_text << std::endl;

  return 0;
}
