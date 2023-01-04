#include <iostream>
#include <pthread.h>

#include "HashTable.h"

HashTable::HashTable(int capacity) {
  this->capacity = capacity;
  this->used = 0;
  pthread_rwlock_init(&this->lock, NULL);
  this->table.reserve(capacity);
}

HashTable::~HashTable() {
  pthread_rwlock_destroy(&this->lock);
  for (int i = 0; i < this->capacity; i++) {
    if (this->table[i] != NULL) {
      delete this->table[i];
      this->table[i] = NULL;
    }
  }
}

bool HashTable::insert(int key, int value) {
  bool result = false;
  int bucket = this->hashCode(key);
  pthread_rwlock_wrlock(&this->lock);
  Entry* current = this->table[bucket];
  Entry* before = NULL;
  while (current) {
    if (current->key == key) {
      // If the given key already exists in the table, just update the corresponding value.
      result = true;
      current->value = value;
      break;
    }
    before = current;
    current = current->next;
  }

  if (!current) {
    // Given key was not already existing in the table. Try to insert a new entry at the end of the linked list.
    if (this->used == this->capacity) {
      // Table is fully occupied, insertion is not possible
      result = false;
    }
    else {
      if (before) {
        before->next = new Entry{key, value, NULL};
      }
      else {
        // Empty bucket, create the first one
        this->table[bucket] = new Entry{key, value, NULL}; 
      }
      result = true;
      this->used++;
    }
  }

  pthread_rwlock_unlock(&this->lock);
  return result;
}

bool HashTable::remove(int key) {
  bool result = false;
  int bucket = this->hashCode(key);
  
  pthread_rwlock_wrlock(&this->lock);
  Entry* current = this->table[bucket];
  Entry* before = NULL;
  while (current) {
    if (current->key == key) {
      result = true;
      if (before) {
        before->next = current->next;
      }
      else {
        this->table[bucket] = NULL;
      }
      delete current;
      break;
    }
    before = current;
    current = current->next;
  }

  if (result) {
    this->used--;
  }

  pthread_rwlock_unlock(&this->lock);
  return result;
}

std::pair<bool, int> HashTable::get(int key) {
  int bucket = this->hashCode(key);

  pthread_rwlock_rdlock(&this->lock);
  Entry* current = this->table[bucket];
  while (current) {
    if (current->key == key) {
      break;
    }
    current = current->next;
  }

  pthread_rwlock_unlock(&this->lock);
  if (current == NULL) {
    return std::make_pair(false, 0);
  }
  return std::make_pair(true, current->value);
}

int HashTable::hashCode(int key) {
  return key % this->capacity;
}

void HashTable::print() {
  std::cout << "HashTable state: {";
  for (int i = 0; i < this->capacity; i++) {
    Entry* current = this->table[i];
    while (current) {
      std::cout << current->key << ": " << current->value << ", ";
      current = current->next;
    }
  }
  std::cout << "}" << std::endl;
}
