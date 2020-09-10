#ifndef MY_MEMORY_POOL_H
#define MY_MEMORY_POOL_H

#include <cstdint>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

#include <iostream>

//a simple shared memory pool
template<int ObjectSize>
class MyMemoryPool {
  union node {
    int offset;
    char data[ObjectSize];
  };
  int IPCKEY, shmid;
  void* shmaddr;
  int* first;
  uint8_t* used;
  node* data;
  int total, count;
  void set(int id);
  void reset(int id);
 public:
  MyMemoryPool();
  ~MyMemoryPool();
  int init(int IPCKEY, int ObjectNumber);
  void* apply();
  void* get(int id);
  int getstatus(int id);
  void release(void*);
  int empty();
  int size();
  int full();
  void clear();
  void print();
};


//mark the idth element as used
//do nothing if IPCKEY == -1 or id is invalid
template<int ObjectSize>
void MyMemoryPool<ObjectSize>::set(int id) {
  if (IPCKEY == -1) return;
  if (id < 0 || id >= total) return;
  used[id / 8] |= 1 << (7 - id % 8);
}

//mark the idth element as unused
//do nothing if IPCKEY == -1 or id is invalid
template<int ObjectSize>
void MyMemoryPool<ObjectSize>::reset(int id) {
  if (IPCKEY == -1) return;
  if (id < 0 || id >= total) return;
  used[id / 8] &= ~(1 << (7 - id % 8));
}

//initialize the shared memory pool using given IPCKEY and ObjectNumber
//if failed return -1, else return 0
template<int ObjectSize>
int MyMemoryPool<ObjectSize>::init(int IPCKEY, int ObjectNumber) {
  int totalSize = sizeof(int) + 4 * (1 + (ObjectNumber - 1) / 32) + ObjectSize * ObjectNumber; 
  total = ObjectNumber;
  shmid = shmget(IPCKEY, totalSize, 0666);
  if (shmid == -1) {
    shmid = shmget(IPCKEY, totalSize, 0666 | IPC_CREAT);
    if (shmid == -1) return -1;
    shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == reinterpret_cast<void*>(-1)) return -1;
    first = static_cast<int*>(shmaddr);
    used = reinterpret_cast<uint8_t*>(first) + sizeof(int);
    data = reinterpret_cast<node*>(used + 4 * (1 + (ObjectNumber - 1) / 32));
    this->IPCKEY = IPCKEY;
    clear();
  }
  else {
    shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == reinterpret_cast<void*>(-1)) return -1;
    this->IPCKEY = IPCKEY;
    first = static_cast<int*>(shmaddr);
    used = reinterpret_cast<uint8_t*>(first) + sizeof(int);
    data = reinterpret_cast<node*>(used + 4 * (1 + (ObjectNumber - 1) / 32));
    count = 0;
    for (int i = 0; i < total; ++i)
      count += getstatus(i);
  } 
  return 0;
}

template<int ObjectSize>
MyMemoryPool<ObjectSize>::MyMemoryPool() {
  IPCKEY = shmid = -1;
}

template<int ObjectSize>
MyMemoryPool<ObjectSize>::~MyMemoryPool() {
  if (IPCKEY != -1)
    shmdt(shmaddr);
}

//Allocates ObjectSize bytes of uninitialized storage from shared memory
//Returns NULL if no memory is available else a pointer to the allocated memory
template<int ObjectSize>
void* MyMemoryPool<ObjectSize>::apply() {
  if (IPCKEY == -1) return NULL;
  if (*first == 0) {
    return NULL;
  }
  void* ans = static_cast<uint8_t*>(shmaddr) + *first;
  int offset = (static_cast<node*>(ans))->offset;
  if (offset == 0)
    *first = 0;
  else
    *first = *first + (static_cast<node*>(ans))->offset;
  count++;
  int id = ((static_cast<node*>(ans)) - data);
  set(id);
  return ans;
}

//return the address of the idth element
//return NULL if IPCKEY == -1 or id is invalid
template<int ObjectSize>
void* MyMemoryPool<ObjectSize>::get(int id) {
  if (IPCKEY == -1) return NULL;
  if (id < 0 || id >= total) return NULL;
  return static_cast<void*>(data + id);
}

//return the status of the idth element
//return -1 if IPCKEY == -1 or id is invalid
//if the idth element is unused, return 0 else return 1
template<int ObjectSize>
int MyMemoryPool<ObjectSize>::getstatus(int id) {
  if (IPCKEY == -1) return -1;
  if (id < 0 || id >= total) return -1;
  return (used[id / 8] >> (7 - id % 8)) & 1;
}

//Deallocates the space previously allocated by apply
//If ptr is a null pointer, the function does nothing.
template<int ObjectSize>
void MyMemoryPool<ObjectSize>::release(void* ptr) {
  if (IPCKEY == -1) return;
  if (ptr == NULL) return;
  int delta = static_cast<uint8_t*>(ptr) - static_cast<uint8_t*>(shmaddr);
  if ((static_cast<uint8_t*>(ptr) - reinterpret_cast<uint8_t*>(data)) % sizeof(node) != 0) return;
  if (*first == 0)
    (static_cast<node*>(ptr))->offset = 0;
  else
    (static_cast<node*>(ptr))->offset = *first - delta;
  *first = delta;
  count--;
  int id = ((static_cast<node*>(ptr)) - data);
  reset(id);
}

//return -1 if IPCKEY == -1
//return 1 if the memory pool is empty 
template<int ObjectSize>
int MyMemoryPool<ObjectSize>::empty() {
  if (IPCKEY == -1) return -1;
  return count == 0;
}

//return -1 if IPCKEY == -1
//else return the size of the memory pool
template<int ObjectSize>
int MyMemoryPool<ObjectSize>::size() {
  if (IPCKEY == -1) return -1;
  return count;
}

//return -1 if IPCKEY == -1
//return 1 if the memory pool is full 
template<int ObjectSize>
int MyMemoryPool<ObjectSize>::full() {
  if (IPCKEY == -1) return -1;
  return count == total;
}

//clear the memory pool
//do nothing if IPCKEY == -1
template<int ObjectSize>
void MyMemoryPool<ObjectSize>::clear() {
  if (IPCKEY == -1) return;
  for (int i = 0; i < 1 + (total - 1) / 8; ++i)
    used[i] = 0;
  *first = reinterpret_cast<uint8_t*>(data) - reinterpret_cast<uint8_t*>(first);
  for (int i = 0; i < total; ++i) {
    if (i != total - 1)
      data[i].offset = sizeof(node);
    else 
      data[i].offset = 0;
  }
  count = 0;
}

//Print the shared memory, just for test
template<int ObjectSize>
void MyMemoryPool<ObjectSize>::print() {
  std::cout << "first = " << *first << std::endl;
  for (int i = 0; i < total; ++i) {
    std::cout << "offset = " << data[i].offset << std::endl;
  }
}

#endif /* MY_MEMORY_POOL_H */
