#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <omp.h>

#define SUDOKU_SIZE 9
#define SUBGRID_SIZE 3

int sudoku[SUDOKU_SIZE][SUDOKU_SIZE];

// Función para verificar las filas
int check_rows() {
    for (int i = 0; i < SUDOKU_SIZE; i++) {
        int nums[SUDOKU_SIZE + 1] = {0};
        for (int j = 0; j < SUDOKU_SIZE; j++) {
            int value = sudoku[i][j];
            if (nums[value] == 1)
                return 0;
            nums[value] = 1;
        }
    }
    return 1;
}

// Función para verificar las columnas
int check_columns() {
    for (int i = 0; i < SUDOKU_SIZE; i++) {
        int nums[SUDOKU_SIZE + 1] = {0};
        for (int j = 0; j < SUDOKU_SIZE; j++) {
            int value = sudoku[j][i];
            if (nums[value] == 1)
                return 0;
            nums[value] = 1;
        }
    }
    return 1;
}

// Función para verificar los subarreglos de 3x3
int check_subgrids() {
    for (int row = 0; row < SUDOKU_SIZE; row += SUBGRID_SIZE) {
        for (int col = 0; col < SUDOKU_SIZE; col += SUBGRID_SIZE) {
            int nums[SUDOKU_SIZE + 1] = {0};
            for (int i = row; i < row + SUBGRID_SIZE; i++) {
                for (int j = col; j < col + SUBGRID_SIZE; j++) {
                    int value = sudoku[i][j];
                    if (nums[value] == 1)
                        return 0;
                    nums[value] = 1;
                }
            }
        }
    }
    return 1;
}

// Función para el pthread
void *column_check_thread(void *arg) {
    printf("Thread ID: %ld\n", syscall(SYS_gettid));
    int result = check_columns();
    pthread_exit((void *)(long)result);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <archivo_sudoku>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Error al abrir el archivo");
        return 1;
    }

    char *file_content = mmap(NULL, 81, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_content == MAP_FAILED) {
        perror("Error al mappear el archivo");
        return 1;
    }

    for (int i = 0, k = 0; i < SUDOKU_SIZE; i++) {
        for (int j = 0; j < SUDOKU_SIZE; j++, k++) {
            sudoku[i][j] = file_content[k] - '0';
        }
    }

    int subgrids_check = check_subgrids();

    pid_t pid = fork();
    if (pid == 0) {
        char pid_str[16];
        snprintf(pid_str, sizeof(pid_str), "%d", getppid());
        execlp("ps", "ps", "-p", pid_str, "-lLf", NULL);
        perror("Error al ejecutar ps");
        return 1;
    }

    pthread_t th;
    pthread_create(&th, NULL, column_check_thread, NULL);

    int column_check;
    pthread_join(th, (void **)&column_check);
    printf("Parent Thread ID: %ld\n", syscall(SYS_gettid));

    wait(NULL);

    int row_check = check_rows();

    if (row_check && column_check && subgrids_check)
        printf("La solución del sudoku es válida.\n");
    else
        printf("La solución del sudoku no es válida.\n");

    pid = fork();
    if (pid == 0) {
        char pid_str[16];
        snprintf(pid_str, sizeof(pid_str), "%d", getppid());
        execlp("ps", "ps", "-p", pid_str, "-lLf", NULL);
        perror("Error al ejecutar ps");
        return 1;
    }

    wait(NULL);

    return 0;
}