#pragma once

#include <pthread.h>
#include <sys/time.h>

enum class RequestType {
  INSERT, READ, DELETE
};

std::ostream& operator<<(std::ostream& os, RequestType requestType) {
  switch (requestType) {
    case RequestType::INSERT:
      os << "INSERT";
      break;
    case RequestType::READ:
      os << "READ";
      break;
    case RequestType::DELETE:
      os << "DELETE";
      break;
  }
  return os;
}

typedef struct Request {
  int key;
  int value;
  RequestType type;
} Request;

typedef struct Response {
  int value;
  RequestType type;
  bool success; 
} Response;

typedef struct CommBuffer {
  Request request;
  bool requestUpdated;
  pthread_cond_t requestUpdatedCv;

  Response response;
  bool responseUpdated;
  pthread_cond_t responseUpdatedCv;

  pthread_mutex_t lock;
} CommBuffer;

const int N_SERVER_THREADS = 8;

const char* SHM_NAME = "/hash_table";
const int SHM_SIZE = sizeof(CommBuffer) * N_SERVER_THREADS;

void waitCvWithTimeout(pthread_cond_t* cv, pthread_mutex_t* mutex) {
  struct timeval tv;
  struct timespec ts;
  int wait_time_in_ms = 10000;

  gettimeofday(&tv, NULL);
  ts.tv_sec = time(NULL) + wait_time_in_ms / 1000;
  ts.tv_nsec = tv.tv_usec * 1000 + 1000 * 1000 * (wait_time_in_ms % 1000);
  ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
  ts.tv_nsec %= (1000 * 1000 * 1000);
  pthread_cond_timedwait(cv, mutex, &ts);
}