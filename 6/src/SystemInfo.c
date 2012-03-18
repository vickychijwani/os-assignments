#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define TIME_LEN 17

int main(void) {
  FILE *fp;
  int cpu_count = 0, free_mem, total_mem, swap_size;
  float avgload;
  char cpu_speed[20], version[30], btime_str[100], line[255], swap_path[255];
  time_t btime, user_mode, user_mode_niced, kernel_mode, interrupts;
  long context_switches;

  // number of CPU cores and clock speeds
  fp = popen("cat /proc/cpuinfo | grep 'cpu MHz' | grep '[0-9][0-9.]*' --only-matching", "r");
  if (fp == NULL) {
    fprintf(stderr, "Failed to get CPU info\n");
    exit(1);
  }
  while (fgets(line, sizeof(line), fp) != NULL) {
    sscanf(line, "%s", cpu_speed);
    cpu_count++;
    printf("Speed of CPU %d: %s MHz\n", cpu_count, cpu_speed);
  }
  printf("Number of CPUs: %d\n", cpu_count);
  pclose(fp);

  // Linux kernel version
  printf("\n");
  fp = fopen("/proc/version", "r");
  if (fp == NULL) {
    fprintf(stderr, "Could not open file /proc/version\n");
    exit(1);
  }
  fscanf(fp, "Linux version %s", version);
  printf("Linux version: %s\n", version);
  fclose(fp);

  // Last boot time
  printf("\n");
  fp = popen("cat /proc/stat | grep 'btime' | grep '[0-9][0-9]*' --only-matching", "r");
  if (fp == NULL) {
    fprintf(stderr, "Failed to get last boot time\n");
    exit(1);
  }
  fscanf(fp, "%ld", &btime);
  strftime(btime_str, TIME_LEN, "%d %b, %H:%M:%S", localtime(&btime));
  printf("Last boot time: %s\n", btime_str);
  pclose(fp);

  // average load in the last 5 minutes
  printf("\n");
  fp = fopen("/proc/loadavg", "r");
  if (fp == NULL) {
    fprintf(stderr, "Could not open file /proc/loadavg\n");
    exit(1);
  }
  fscanf(fp, "%*f %f", &avgload);
  printf("Average load over the last 5 minutes: %0.2f\n", avgload);
  fclose(fp);

  // total usable and free memory
  printf("\n");
  fp = fopen("/proc/meminfo", "r");
  if (fp == NULL) {
    fprintf(stderr, "Could not open file /proc/meminfo\n");
    exit(1);
  }
  fgets(line, sizeof(line), fp);
  sscanf(line, "MemTotal: %d kB", &total_mem);
  fgets(line, sizeof(line), fp);
  sscanf(line, "MemFree: %d kB", &free_mem);
  printf("Total usable memory = %d kB\n", total_mem);
  printf("Free memory = %d kB\n", free_mem);
  fclose(fp);

  // swap partitions and their sizes
  printf("\n");
  fp = fopen("/proc/swaps", "r");
  if (fp == NULL) {
    fprintf(stderr, "Could not open file /proc/swaps");
    exit(1);
  }
  fgets(line, sizeof(line), fp);
  printf("Swap partitions:\n");
  printf("Partition\tSize\n");
  while (fgets(line, sizeof(line), fp) != NULL) {
    sscanf(line, "%s %*s %d", swap_path, &swap_size);
    printf("%s\t%d kB\n", swap_path, swap_size);
  }
  fclose(fp);

  // time spent by CPU in different modes and servicing interrupts
  printf("\n");
  fp = popen("cat /proc/stat | grep 'cpu[^0-9]'", "r");
  if (fp == NULL) {
    fprintf(stderr, "Failed to get time spent by CPU in various modes\n");
    exit(1);
  }
  fscanf(fp, "cpu %ld %ld %ld %*s %*s %*s %ld", &user_mode, &user_mode_niced, &kernel_mode, &interrupts);
  user_mode = user_mode * 1.0 / sysconf(_SC_CLK_TCK) * 1000;
  user_mode_niced = user_mode_niced * 1.0 / sysconf(_SC_CLK_TCK) * 1000;
  kernel_mode = kernel_mode * 1.0 / sysconf(_SC_CLK_TCK) * 1000;
  interrupts = interrupts * 1.0 / sysconf(_SC_CLK_TCK) * 1000;
  printf("Time spent by CPU in user mode = %ld ms\n", user_mode + user_mode_niced);
  printf("Time spent by CPU in kernel mode = %ld ms\n", kernel_mode);
  printf("Time spent by CPU handling interrupts = %ld ms\n", interrupts);
  pclose(fp);

  // time spent by CPU in different modes and servicing interrupts
  printf("\n");
  fp = popen("cat /proc/stat | grep 'ctxt'", "r");
  if (fp == NULL) {
    fprintf(stderr, "Failed to get time spent by CPU in various modes\n");
    exit(1);
  }
  fscanf(fp, "ctxt %ld", &context_switches);
  printf("Number of context switches made by the CPU = %ld\n", context_switches);
  pclose(fp);

  return 0;
}
