/*
   GFuzz Variable Scoring Mechanism - Test Program
   -----------------------------------------------
   
   This program tests the variable scoring mechanism by providing
   various types of variables and operations that should be scored.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function with various variable types
int process_data(char* input, int len) {
  int count = 0;
  int* ptr = NULL;
  char buffer[256];
  float score = 0.0;
  
  // Memory operations (should score high on memory_context)
  ptr = (int*)malloc(sizeof(int) * 10);
  if (!ptr) return -1;
  
  memcpy(buffer, input, len < 256 ? len : 255);
  buffer[255] = '\0';
  
  // Control flow operations (should score high on control_flow)
  if (buffer[0] == 'A') {
    count++;
  } else if (buffer[0] == 'B') {
    count += 2;
  }
  
  // Data flow operations (should score high on data_flow)
  for (int i = 0; i < len && i < 10; i++) {
    ptr[i] = buffer[i];
    score += (float)buffer[i] / 100.0;
  }
  
  // Frequent usage (should score high on frequency)
  for (int j = 0; j < 5; j++) {
    count += ptr[j % 10];
  }
  
  free(ptr);
  
  return count;
}

// Function with pointer operations
void* allocate_memory(size_t size) {
  void* data = malloc(size);
  if (data) {
    memset(data, 0, size);
  }
  return data;
}

// Function with string operations
int string_compare(const char* str1, const char* str2) {
  char temp[128];
  strcpy(temp, str1);
  
  if (strcmp(temp, str2) == 0) {
    return 1;
  }
  
  return 0;
}

int main(int argc, char** argv) {
  char input[64];
  int result;
  void* memory;
  
  if (argc < 2) {
    printf("Usage: %s <input>\n", argv[0]);
    return 1;
  }
  
  strncpy(input, argv[1], 63);
  input[63] = '\0';
  
  result = process_data(input, strlen(input));
  printf("Result: %d\n", result);
  
  memory = allocate_memory(1024);
  if (memory) {
    free(memory);
  }
  
  if (string_compare(input, "test")) {
    printf("Matched!\n");
  }
  
  return 0;
}
