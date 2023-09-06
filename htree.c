#include <stdio.h>     
#include <stdlib.h>   
#include <stdint.h>  
#include <inttypes.h>  
#include <errno.h>     // for EINTR
#include <fcntl.h>     
#include <unistd.h>    
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <math.h>
#include <sys/mman.h>
#include <string.h>

// Print out the usage of the program and exit.
void Usage(char*);
uint32_t jenkins_one_at_a_time_hash(const uint8_t* , uint64_t );

double GetTime();

void *tree(void *arg);

// block size
#define BSIZE 4096

uint8_t* arr;


unsigned long long int threadsize = 0;
unsigned long long int mthreads  = 0;

int main(int argc, char** argv) 
{
  int32_t fd;
  unsigned long long int nblocks;
  // input checking 
  if (argc != 3)
    Usage(argv[0]);

  mthreads = atoi(argv[2]);

  // open input file
  fd = open(argv[1], O_RDWR);
  if (fd == -1) 
  {
    perror("open failed");
    exit(EXIT_FAILURE);
  }
  // use fstat to get file size
  struct stat file_stat;
  fstat(fd, &file_stat);
  
  uint64_t size = file_stat.st_size;
  
  // calculate nblocks 
  nblocks = size / BSIZE;
  printf(" no. of blocks = %llu \n", nblocks);
  
  threadsize = nblocks / mthreads;
  
  printf("blocks per thread: %llu\n", threadsize);
  double start = GetTime();

  // calculate hash value of the input file
  
  arr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);


  int ht = 0;
  
  pthread_t p1;
	pthread_create(&p1, NULL, tree, &ht);
 
  void* result;
  pthread_join(p1, &result);

  double end = GetTime();

  printf("hash value = %u \n", *(uint32_t*)result);
  printf("time taken = %f \n", (end - start));
  close(fd);
  munmap(arr, size);
    
  return EXIT_SUCCESS;
  }

uint32_t jenkins_one_at_a_time_hash(const uint8_t* key, uint64_t length) 
{
  uint64_t i = 0;
  uint32_t hash = 0;

  while (i != length) 
  {
    hash += key[i++];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}


//From common.h
double GetTime()
{
  struct timeval time;
  int rc = gettimeofday(&time, NULL);
  assert(rc == 0);
  
  return (double)time.tv_sec + (double)time.tv_usec/1e6;
}

void Spin(int length){
  double time = GetTime();
  while((GetTime() - time) < (double)length)
  ;
}

void Usage(char* s) 
{
  fprintf(stderr, "Usage: %s filename num_threads \n", s);
  exit(EXIT_FAILURE);
}

void* tree(void* arg) 
{ 
    int* id = (int*)arg;
    
    uint32_t* hash = malloc(sizeof(uint32_t));
    *hash = jenkins_one_at_a_time_hash(arr + ((*id) * BSIZE * threadsize), BSIZE * threadsize); 
    if(2 * (*id) + 2 < mthreads)
    {
      //printf("full Int. Thread at height %i\n", *id);
        
      int ht1 = (2 * (*id)) + 1;
      int ht2 = (2 * (*id)) + 2;
       
      pthread_t p1;
      pthread_create(&p1, NULL, tree, &ht1);
       
      pthread_t p2;
      pthread_create(&p2, NULL, tree, &ht2);
     
      void* leftresult;
      void* rightresult;
      
      pthread_join(p1, &leftresult);
     	pthread_join(p2, &rightresult);
    
    
      char center[100];
      sprintf(center, "%u", *hash);
      char left[100];
      sprintf(left, "%u", *((uint32_t*)leftresult));
      char right[100];
      sprintf(right, "%u", *((uint32_t*)rightresult));
      
      printf("left id: %i sent %s to parent\n", ht1, left);
      printf("right id: %i sent %s to parent\n", ht2, right);
      
      char* centerleft = strcat(center, left);
      char* new_hash_string = strcat(centerleft, right);
      
      printf("id: %i concat: %s\n",*id ,new_hash_string);
      
      *hash = jenkins_one_at_a_time_hash((uint8_t*)new_hash_string, strlen(new_hash_string)); 
      
    }
    else if(2 * (*id) + 1 < mthreads)
    {
      //printf("Int. Thread at height %i\n", *id);
      
      int ht1 = (2 * (*id)) + 1;
      
      pthread_t p1;
      
      pthread_create(&p1, NULL, tree, &ht1);
      
      
      
      void* result;
      pthread_join(p1, &result);
      
      char center[100];
      sprintf(center, "%u", *hash);
      char left[100];
      sprintf(left, "%u", *((uint32_t*)result));
      
      char* new_hash_string = strcat(center, left);
      
      printf("id: %i concat: %s\n",*id ,new_hash_string);
      
      printf("id: %i sent %s to parent\n", ht1, left);
      
      *hash = jenkins_one_at_a_time_hash((uint8_t*)new_hash_string, strlen(new_hash_string)); 
    }
    else
    {  
      
       
    }
    printf("id: %i Hash: %u\n",*id , *hash);
    pthread_exit((void*) hash);
    return NULL;
}