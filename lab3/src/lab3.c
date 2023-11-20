#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int my_atoi(const char *str) {
    int result = 0;
    int sign = 1;

    if (*str == '-') {
        sign = -1;
        ++str;
    }
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        ++str;
    }

    return result * sign;
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // Получение размера файла
    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }

    // Создание отображения файла в памяти для родительского процесса
    char* shared_memory = (mmap(NULL, file_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (shared_memory == MAP_FAILED) {
        perror("mmap error");
        close(fd);
        return 1;
    }
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek");
        munmap(shared_memory, file_stat.st_size);
        close(fd);
        return 1;
    }

    // Создание отображения для новой переменной типа int
    int* sum_shared_memory = (mmap(NULL, file_stat.st_size + 2*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (sum_shared_memory == MAP_FAILED) {
        perror("mmap error");
        munmap(shared_memory, file_stat.st_size);
        close(fd);
        return 1;
    }

    // Создание дочернего процесса
    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        munmap(shared_memory, file_stat.st_size);
        munmap(sum_shared_memory, file_stat.st_size + 2*sizeof(int));
        close(fd);
        return 1;
    }

    if (child_pid == 0) {
        // Дочерний процесс
        int sum = 0;
        char current_number[20];
        int current_number_index = 0;
        int is_negative = 0; 
        for (int i = 0; i < file_stat.st_size; ++i) {
            if (shared_memory[i] == '-' && current_number_index == 0) {
                is_negative = 1;
            } else if ((shared_memory[i] >= '0' && shared_memory[i] <= '9') || (shared_memory[i] == '.')) {
                current_number[current_number_index] = shared_memory[i];
                current_number_index++;
            } else if (shared_memory[i] == ' ' || shared_memory[i] == '\n' || shared_memory[i] == '\t') {
                if (current_number_index > 0) {
                    current_number[current_number_index] = '\0';  
                    int current_value = my_atoi(current_number);
                    if (is_negative) {
                        current_value *= -1;  
                        is_negative = 0;
                    }
                    sum += current_value;
                    current_number_index = 0;
                }
            }
        }
        sum_shared_memory[file_stat.st_size /sizeof(int) + 1] = sum;

        _exit(0);
    } else {
        // Родительский процесс

        // Ждем завершения дочернего процесса
        int status;
        if (waitpid(child_pid, &status, 0) == -1) {
            perror("waitpid");
        }
        int result = sum_shared_memory[file_stat.st_size /sizeof(int) + 1];
        printf("%d", result);
        munmap(shared_memory, file_stat.st_size);
        munmap(sum_shared_memory, file_stat.st_size + 2*sizeof(int));
        close(fd);
    }

    return 0;
}
