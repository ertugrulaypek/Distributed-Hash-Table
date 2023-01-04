#include <iostream>
#include <unordered_map>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>
#include <random>

#include "Common.h"

int selectServerThread() {
  // Generate a non-deterministic seed
  std::random_device rd;
  std::mt19937 gen(rd());

  // Generate a random integer between 0 and N_SERVER_THREADS-1 (inclusive)
  std::uniform_int_distribution<> dis(0, N_SERVER_THREADS-1);
  return dis(gen);
}

void sendRequest(CommBuffer* commBuffer, RequestType type, int key, int value = 0) {
  int selectedServerThread = selectServerThread();
  std::cout << "Client uses server thread #" << selectedServerThread 
            << " to enter request {type:" << type << ", key: " << key << ", value: " << value
            << "}" << std::endl;
  commBuffer += selectedServerThread;
  
  pthread_mutex_lock(&(commBuffer->lock));

  // wait until the current request is processed
  while (commBuffer->requestUpdated) {
    waitCvWithTimeout(&(commBuffer->responseUpdatedCv), &(commBuffer->lock));
  }

  // update request
  commBuffer->request.type = type;
  commBuffer->request.key = key;
  commBuffer->request.value = value;
  commBuffer->requestUpdated = true;

  // let the server know about the new request
  pthread_cond_broadcast(&(commBuffer->requestUpdatedCv));

  // wait until the server processes the request and writes the response
  while (!commBuffer->responseUpdated) {
    waitCvWithTimeout(&(commBuffer->responseUpdatedCv), &(commBuffer->lock));
  }

  // print the response
  std::cout << "Response received: {type: " << commBuffer->response.type 
            << ", value: " << commBuffer->response.value 
            << ", success: " << std::boolalpha << commBuffer->response.success 
            << "}" << std::endl;

  commBuffer->responseUpdated = false;

  pthread_mutex_unlock(&(commBuffer->lock));
}

int main(int argc, char** argv) {
  std::cout << "Shared memory based multiple reader/writer hash table client started!" << std::endl;

  // open the shared memory segment
  int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
  void* ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  CommBuffer* commBuffer = (CommBuffer*) ptr;

  std::cout << "Client initialized the comm buffer." << std::endl;

  // Requests sent in order. Change them if you want to.
  sendRequest(commBuffer, RequestType::INSERT, 1, 100);
  sendRequest(commBuffer, RequestType::INSERT, 2, 200);
  sendRequest(commBuffer, RequestType::INSERT, 3, 300);
  sendRequest(commBuffer, RequestType::INSERT, 4, 400);
  sendRequest(commBuffer, RequestType::INSERT, 5, 500);
  sendRequest(commBuffer, RequestType::INSERT, 6, 600);
  sendRequest(commBuffer, RequestType::READ, 2);
  sendRequest(commBuffer, RequestType::INSERT, 2, 700);
  sendRequest(commBuffer, RequestType::READ, 2);
  sendRequest(commBuffer, RequestType::DELETE, 1);
  sendRequest(commBuffer, RequestType::READ, 1);
  sendRequest(commBuffer, RequestType::INSERT, 8, 800);
  sendRequest(commBuffer, RequestType::READ, 8);

  // clean up resources
  munmap(ptr, SHM_SIZE);
  close(shm_fd);
  return 0;
}
