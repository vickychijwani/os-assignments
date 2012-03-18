#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  int pid;
  FILE *fp;
  char dirpath[255], filepath[255], line[1000];
  time_t running_time, waiting_time, user_mode, kernel_mode;

  if (argc != 2) {
    fprintf(stderr, "Usage: ./ProcessInfo <pid>\n");
    exit(1);
  }
  pid = atoi(argv[1]);

  // construct the directory path corresponding to the process
  sprintf(dirpath, "/proc/%d", pid);

  // command line with which the process was started
  sprintf(filepath, "%s/cmdline", dirpath);
  fp = fopen(filepath, "r");
  if (fp == NULL) {
    fprintf(stderr, "Could not open file %s (perhaps there is no process with pid %d?)\n", filepath, pid);
    exit(1);
  }
  fgets(line, sizeof(line), fp);
  printf("Command line for pid %d:\n%s\n", pid, line);
  fclose(fp);

  // time spent running and waiting
  printf("\n");
  sprintf(filepath, "%s/schedstat", dirpath);
  fp = fopen(filepath, "r");
  if (fp == NULL) {
    fprintf(stderr, "Could not open file %s (perhaps there is no process with pid %d?)\n", filepath, pid);
    exit(1);
  }
  fscanf(fp, "%ld %ld", &running_time, &waiting_time);
  printf("Time spent running by pid %d = %ld ms\n", pid, running_time);
  printf("Time spent waiting by pid %d = %ld ms\n", pid, waiting_time);
  fclose(fp);

  // time spent in user mode and kernel mode
  printf("\n");
  sprintf(filepath, "%s/stat", dirpath);
  fp = fopen(filepath, "r");
  if (fp == NULL) {
      fprintf(stderr, "Could not open file %s (perhaps there is no process with pid %d?)\n", filepath, pid);
      exit(1);
  }
  fscanf(fp, "%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %ld %ld", &user_mode, &kernel_mode);
  user_mode = user_mode * 1.0 / sysconf(_SC_CLK_TCK) * 1000;
  kernel_mode = kernel_mode * 1.0 / sysconf(_SC_CLK_TCK) * 1000;
  printf("Time spent in user mode by pid %d = %ld ms\n", pid, user_mode);
  printf("Time spent in kernel mode by pid %d = %ld ms\n", pid, kernel_mode);
  fclose(fp);

  // contents of address space
  // NOTE: /proc/<pid>/maps gives a listing of the memory mappings for the
  // process; to get the actual contents of the process's memory, read
  // corresponding sections (as given in the /proc/<pid>/maps listing) from
  // /proc/<pid>/mem
  printf("\nContents of address space of Process:\n");
  sprintf(filepath, "%s/maps", dirpath);
  fp = fopen(filepath, "r");
  if (fp == NULL) {
      fprintf(stderr, "Could not open file %s (perhaps there is no process with pid %d?)\n", filepath, pid);
      exit(1);
  }
  printf("address\t\t  perms\toffset\tdev\tinode\t\tpath\n");
  while (fgets(line, sizeof(line), fp) != NULL) {
      printf("%s", line);
  }
  fclose(fp);

  return 0;
}
