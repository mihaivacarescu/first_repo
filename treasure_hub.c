#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

pid_t monitor_pid = -1; // PID-ul procesului monitor

// Functia care trimite semnale procesului monitor
void send_signal_to_monitor(int sig, const char *hunt_id, int treasure_id) {
    if (monitor_pid > 0) {
        union sigval value;
        value.sival_ptr = (void*)hunt_id; // trimite hunt_id ca pointer
        if (treasure_id != -1) {
            value.sival_int = treasure_id; // trimite treasure_id ca int
        }
        
        // Trimite semnalul cu parametrii corespunzători
        sigqueue(monitor_pid, sig, value);
    } else {
        printf("Error: No monitor process running.\n");
    }
}

// Functia de tratare a semnalului SIGCHLD pentru a detecta terminarea monitorului
void handle_child_termination(int sig) {
    int status;
    waitpid(monitor_pid, &status, 0); // Aștept terminarea monitorului
    if (WIFEXITED(status)) {
        printf("Monitor process terminated normally.\n");
    } else {
        printf("Monitor process terminated abnormally.\n");
    }
    monitor_pid = -1; // Resetăm PID-ul
}

int main() {
    char command[256];

    // Instalăm handler-ul pentru SIGCHLD
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_child_termination;
    sigaction(SIGCHLD, &sa, NULL);

    while (1) {
        printf("Enter command: ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0; // Elimină newline-ul

        if (strcmp(command, "start_monitor") == 0) {
            if (monitor_pid == -1) {
                monitor_pid = fork();
                if (monitor_pid == 0) {
                    // Codul pentru monitor (treasure_manager)
                    execl("./treasure_manager", "treasure_manager", (char *)NULL);
                    exit(1); // Dacă exec fail, terminăm procesul
                }
                printf("Monitor started with PID: %d\n", monitor_pid);
            } else {
                printf("Monitor is already running.\n");
            }

        } else if (strcmp(command, "list_hunts") == 0) {
            send_signal_to_monitor(SIGUSR1, "", -1); // Trimite SIGUSR1 pentru a lista vânătorile

        } else if (strcmp(command, "list_treasures") == 0) {
            send_signal_to_monitor(SIGUSR1, "hunt_id_placeholder", -1); // Trimite SIGUSR1 pentru a lista comorile

        } else if (strcmp(command, "view_treasure") == 0) {
            int id;
            printf("Enter treasure ID to view: ");
            scanf("%d", &id);
            getchar(); // Consumă newline-ul rămas
            send_signal_to_monitor(SIGUSR2, "hunt_id_placeholder", id); // Trimite SIGUSR2 cu ID-ul comorii

        } else if (strcmp(command, "stop_monitor") == 0) {
            if (monitor_pid > 0) {
                kill(monitor_pid, SIGTERM); // Trimite SIGTERM pentru a opri monitorul
                printf("Stopping monitor...\n");
            } else {
                printf("Monitor is not running.\n");
            }

        } else if (strcmp(command, "exit") == 0) {
            if (monitor_pid != -1) {
                printf("Error: Stop the monitor before exiting.\n");
            } else {
                break; // Ieșire din program
            }

        } else {
            printf("Unknown command.\n");
        }
    }
    return 0;
}
