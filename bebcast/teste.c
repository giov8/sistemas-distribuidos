#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{

  FILE *fp;
  char path[1035];

  int i = 1;
  int s = 3;
  int j = 2;

  /* Open the command for reading. */
  char str[64];
  sprintf(str, "./cisj %d %d %d", i, s, j);
  printf("%s", str);
  fp = popen(str, "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  /* Read the output a line at a time - output it. */
  while (fgets(path, sizeof(path), fp) != NULL) {
    printf("%s", path);
    int novo = atoi(path);
   	printf("Novo: %d\n", novo);
  }

  /* close */
  pclose(fp);

  return 0;
}