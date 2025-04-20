#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>  //necesar pt mkdir
#include <string.h>
#include <fcntl.h>
#include <time.h>  //necesar pt functia list_treasures
#include <errno.h> //pentru a avea acces la codurile de eroare setate de sistem, atunci când o funcție de sistem esueaza
#include <stdlib.h>  //necesar pt ca folosesc atoi in main()

#define USERNAME_MAX 32
#define CLUE_MAX 128
#define MAX_PATH 256
#define TREASURE_FILE "treasures.dat"
#define LOG_FILE "logged_hunt"


typedef struct {
    int treasure_id;
    char username[USERNAME_MAX];
    float latitude;
    float longitude;
    char clue[CLUE_MAX];
    int value;
}Treasure;


void log_operation(const char *hunt_id, const char *message) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", hunt_id, LOG_FILE);
    int fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644);    //fisierul text de log  nu e unul executabil
    if (fd < 0) return;
    write(fd, message, strlen(message));
    write(fd, "\n", 1);  //pt o mai buna lizibilitate
    close(fd);

    char symlink_name[MAX_PATH];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);
    unlink(symlink_name); // șterge dacă deja există (sterge un link simbolic existent)
    symlink(path, symlink_name);  //creaza un link simbolic catre fisierul de log real(path)
}



void adauga_comoara(const char *hunt_id){
   mkdir(hunt_id,0755);     //aici se creaza un nou director numit hunt_id in directorul curent(permisiuni 0755)

   Treasure t;
   printf("ID comoară: ");   scanf("%d", &t.treasure_id);
   getchar(); //pt a consuma newline-ul ramas dupa scanf
  
   printf("Nume utilizator: ");   fgets(t.username,USERNAME_MAX,stdin);
   t.username[strcspn(t.username, "\n")] ='\0';   // elimină '\n'
  
   printf("Latitudine: "); scanf("%f", &t.latitude);
   printf("Longitudine: "); scanf("%f", &t.longitude); 
  
   printf("Indiciu: ");  getchar(); fgets(t.clue, CLUE_MAX, stdin);
   t.clue[strcspn(t.clue, "\n")] = '\0';
  
   printf("Valoare: "); scanf("%d", &t.value);


   char path[MAX_PATH];  //crearea caii pt fisierul de comori
   snprintf(path, sizeof(path), "%s/%s", hunt_id, TREASURE_FILE);

   int fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644); //fd-file descriptor(nr intreg) - reprezinta fisierul deschis
   /*O_WRONLY: deschide fișierul pentru scriere (nu citire).
     O_APPEND: orice scriere se face la finalul fișierului (append mode).
     O_CREAT: dacă fișierul nu există, va fi creat.*/
   if (fd < 0) { perror("Eroare la deschidere"); return; }

   write(fd, &t, sizeof(Treasure));   //scrie in fis fd continutul lui t
   close(fd);

   char log[256];
   snprintf(log, sizeof(log), "Adăugată comoara %d de %s", t.treasure_id, t.username);
   log_operation(hunt_id, log);   //logarea operatiunii in fisierul log - pt a pastra o urma a actiunilor
}



void list_treasures(const char *hunt_id) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", hunt_id, TREASURE_FILE);  //construim calea catre fisierul cu comori

    int fd = open(path, O_RDONLY);   //se deschide fisierul path
    if (fd < 0) { perror("open"); return; }

    struct stat st;
    stat(path, &st);    /*functia stat() se folosește pentru a obține informații despre fișierul specificat prin path
		        și le stochează în structura st.*/

    printf("Hunt: %s\n", hunt_id);
    printf("File size: %ld bytes\n", st.st_size); //dimensiunea fisierului in octeti
    printf("Last modified: %s", ctime(&st.st_mtime)); /*ctime() converteste timpul ultimei modificari a fisierului
							(st.st_mtime) intr-un sir de caractere in format lizibil*/

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %d | User: %s | (%f, %f) | Value: %d\n", t.treasure_id, t.username, t.latitude, t.longitude, t.value);
    }
    close(fd);

    log_operation(hunt_id, "Listed treasures");
}



void view_treasure(const char *hunt_id, int id) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", hunt_id, TREASURE_FILE);

    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror("open"); return; }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.treasure_id == id) {
            printf("Treasure ID: %d\nUser: %s\nCoordinates: (%f, %f)\nClue: %s\nValue: %d\n", 
                t.treasure_id, t.username, t.latitude, t.longitude, t.clue, t.value);
            close(fd);
            char log[256];
            snprintf(log, sizeof(log), "Viewed treasure %d", id);
            log_operation(hunt_id, log);
            return;
        }
    }
    printf("Treasure ID %d not found.\n", id);
    close(fd);
}



void remove_treasure(const char *hunt_id, int id) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", hunt_id, TREASURE_FILE);
    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror("open"); return; }

    int temp_fd = open("temp.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);  //O_TRUNC goleste fisierul daca deja exista
    if (temp_fd < 0) { perror("temp open"); close(fd); return; }
    //acest fisier temporar va conține toate comorile, cu excepția celei pe care vrem să o ștergem.
    
    Treasure t;
    int found = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.treasure_id != id) {
            write(temp_fd, &t, sizeof(Treasure));
        } else {
            found = 1;
        }
    }
    close(fd);
    close(temp_fd);

    if (found!=0) {
        rename("temp.dat", path);
        char log[256];
        snprintf(log, sizeof(log), "Removed treasure %d", id);
        log_operation(hunt_id, log);
    } else {
        printf("Treasure not found.\n");
        remove("temp.dat");
    }
}



void remove_hunt(const char *hunt_id) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", hunt_id, TREASURE_FILE);  //se formeaza calea catre fisierul cu comori
    unlink(path);   //sterge fisierul treasures.dat
    snprintf(path, sizeof(path), "%s/%s", hunt_id, LOG_FILE);  //se formeaza calea catre fisierul de log
    unlink(path);  //sterge fisierul de log
    rmdir(hunt_id); /*Această funcție șterge directorul doar dacă este gol. Asta e motivul pentru care fișierele din el
		      au fost eliminate anterior.*/

    char link_path[MAX_PATH];
    snprintf(link_path, sizeof(link_path), "logged_hunt-%s", hunt_id);
    unlink(link_path);  //se șterge linkul simbolic asociat logului

    printf("Removed hunt %s\n", hunt_id);
}


int main(int argc,char *argv[]){

  if (argc < 3) {
        printf("Usage:\n  --add <hunt>\n  --list <hunt>\n  --view <hunt> <id>\n  --remove_treasure <hunt> <id>\n  --remove_hunt <hunt>\n");
        return 1;
    }

    if (strcmp(argv[1], "--add") == 0) {
        adauga_comoara(argv[2]);
    } else if (strcmp(argv[1], "--list") == 0) {
        list_treasures(argv[2]);
    } else if (strcmp(argv[1], "--view") == 0 && argc == 4) {
        view_treasure(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "--remove_treasure") == 0 && argc == 4) {
        remove_treasure(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "--remove_hunt") == 0) {
        remove_hunt(argv[2]);
    } else {
        fprintf(stderr, "Invalid command.\n");
    }

  
  return 0;
}
