#include <iostream>
#include <unordered_map>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>

#include "Common.h"
#include "HashTable.h"

typedef struct ServerThreadArgs {
  CommBuffer* commBuffer;
  HashTable* table;
  int thread;
} ServerThreadArgs;

void processNextRequest(CommBuffer* commBuffer, HashTable* table, int thread) {
  pthread_mutex_lock(&(commBuffer->lock));
  std::cout << "Server thread #" << thread << " is ready to process the next request." << std::endl;

  // Wait until a client writes a request
  while (!commBuffer->requestUpdated) {
    waitCvWithTimeout(&(commBuffer->requestUpdatedCv), &(commBuffer->lock));
  }

  // Print the request
  std::cout << "Server thread #" << thread << " received a request: {type: " << commBuffer->request.type 
            << ", key: " << commBuffer->request.key
            << ", value: " << commBuffer->request.value 
            << "}" << std::endl;

  // Handle the request based on its type
  switch (commBuffer->request.type) {
    case RequestType::INSERT: {
      bool success = table->insert(commBuffer->request.key, commBuffer->request.value);
      commBuffer->response.type = commBuffer->request.type;
      commBuffer->response.success = success;
      commBuffer->response.value = commBuffer->request.value;
      break;
    }
    case RequestType::READ: {
      std::pair<bool, int> result = table->get(commBuffer->request.key);
      commBuffer->response.type = commBuffer->request.type;
      commBuffer->response.success = result.first;
      commBuffer->response.value = result.second;
      break;
    }
    case RequestType::DELETE: {
      bool success = table->remove(commBuffer->request.key);
      commBuffer->response.type = commBuffer->request.type;
      commBuffer->response.success = success;
      commBuffer->response.value = commBuffer->request.value;
      break;
    }
  }

  commBuffer->responseUpdated = true;
  commBuffer->requestUpdated = false;
  table->print();
  pthread_cond_broadcast(&(commBuffer->responseUpdatedCv));
  pthread_mutex_unlock(&(commBuffer->lock));
}

void* runServerThread(void* args) {
  ServerThreadArgs* threadArgs = (ServerThreadArgs*) args;
  std::cout << "Starting server thread #" << threadArgs->thread << std::endl;
  while (1) {
    processNextRequest(threadArgs->commBuffer, threadArgs->table, threadArgs->thread);
  }
}

CommBuffer* initSharedBufferArray(void* sharedPtr) {
  CommBuffer* commBufferArray = new (sharedPtr) CommBuffer[N_SERVER_THREADS]();

  pthread_condattr_t attrcond;
  pthread_condattr_init(&attrcond);
  pthread_condattr_setpshared(&attrcond, PTHREAD_PROCESS_SHARED);

  pthread_mutexattr_t attrmutex;
  pthread_mutexattr_init(&attrmutex);
  pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);

  for (int i = 0; i < N_SERVER_THREADS; i++) {
    pthread_cond_init(&(commBufferArray[i].requestUpdatedCv), &attrcond);
    pthread_cond_init(&(commBufferArray[i].responseUpdatedCv), &attrcond);
    pthread_mutex_init(&(commBufferArray[i].lock), &attrmutex);
  }

  return commBufferArray;
}

int main(int argc, char** argv) {
  // parse the hash table size from command line arguments
  int table_size = atoi(argv[1]);
  if (table_size < 1) {
    std::cerr << "Table size argument must be a positive integer" << std::endl;
    exit(-1);
  }

  std::cout << "Shared memory based multiple reader/writer hash table server started!" << std::endl;

  // create the shared memory segment
  int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  ftruncate(shm_fd, SHM_SIZE);
  void* ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  std::cout << "Shared memory created with name " << SHM_NAME << " and size " << SHM_SIZE << std::endl;

  CommBuffer* commBuffer = initSharedBufferArray(ptr);
  std::cout << "Shared memory locks and condition variables are initialized." << std::endl;

  HashTable* table = new HashTable(table_size);

  pthread_t serverThreads[N_SERVER_THREADS];
  ServerThreadArgs serverThreadArgs[N_SERVER_THREADS];
  
  // start server threads
  for (int i = 0; i < N_SERVER_THREADS; i++) {
    serverThreadArgs[i].commBuffer = commBuffer + i;
    serverThreadArgs[i].table = table;
    serverThreadArgs[i].thread = i;
    pthread_create(&(serverThreads[i]), NULL, &runServerThread, (void*)&serverThreadArgs[i]);
  }

  // Join server threads
  for (int i = 0; i < N_SERVER_THREADS; i++) {
    pthread_join(serverThreads[i], NULL);
  }

  // clean up resources
  delete table;
  munmap(ptr, SHM_SIZE);
  shm_unlink(SHM_NAME);
  close(shm_fd);
  return 0;
}
