#pragma once

#include <vector>

typedef struct Entry {
  int key;
  int value;
  Entry* next;
} Entry;

class HashTable {
public:
  HashTable(int capacity);
  ~HashTable();
  bool insert(int key, int value);
  bool remove(int key);
  std::pair<bool, int> get(int key);
  void print();

private:
  int capacity;
  int used;
  pthread_rwlock_t lock;
  std::vector<Entry*> table;
  
  int hashCode(int key);
};