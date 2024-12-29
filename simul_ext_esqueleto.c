#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "headers.h"

#define LONGITUD_COMANDO 100

void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps);
int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2);
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup);
int BuscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombre);
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos);
int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo);
int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre);
int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre, FILE *fich);
int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino, FILE *fich);
void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich);
void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich);
void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich);
void GrabarDatos(EXT_DATOS *memdatos, FILE *fich);

int main() {
    char comando[LONGITUD_COMANDO];
    char orden[LONGITUD_COMANDO];
    char argumento1[LONGITUD_COMANDO];
    char argumento2[LONGITUD_COMANDO];
    int grabardatos = 0;

    EXT_SIMPLE_SUPERBLOCK ext_superblock;
    EXT_BYTE_MAPS ext_bytemaps;
    EXT_BLQ_INODOS ext_blq_inodos;
    EXT_ENTRADA_DIR directorio[MAX_FICHEROS];
    EXT_DATOS memdatos[MAX_BLOQUES_DATOS];
    EXT_DATOS datosfich[MAX_BLOQUES_PARTICION];

    FILE *fent;

    // Open and read the partition file
    fent = fopen("particion1.bin", "r+b");
    if (fent == NULL) {
        perror("Error opening the partition file");
        return 1;
    }

    fread(&datosfich, SIZE_BLOQUE, MAX_BLOQUES_PARTICION, fent);

    memcpy(&ext_superblock,(EXT_SIMPLE_SUPERBLOCK *)&datosfich[0], SIZE_BLOQUE);
     memcpy(directorio, (EXT_ENTRADA_DIR *)&datosfich[3], sizeof(directorio));
     memcpy(&ext_bytemaps, (EXT_BLQ_INODOS *)&datosfich[1], SIZE_BLOQUE);
     memcpy(&ext_blq_inodos, (EXT_BLQ_INODOS *)&datosfich[2], SIZE_BLOQUE);
     memcpy(&memdatos,(EXT_DATOS *)&datosfich[4],MAX_BLOQUES_DATOS*SIZE_BLOQUE);

    // Command processing loop
    for (;;) {
        printf(">> ");
        fflush(stdin);
        fgets(comando, LONGITUD_COMANDO, stdin);

        // Parse the command
        if (ComprobarComando(comando, orden, argumento1, argumento2) != 0) {
            printf("Error parsing command\n");
            continue;
        }

        if (strcmp(orden, "info") == 0) {
            LeeSuperBloque(&ext_superblock);
            continue;
        }

        if (strcmp(orden, "bytemaps") == 0) {
            Printbytemaps(&ext_bytemaps);
            continue;
        }

        if (strcmp(orden, "dir") == 0) {
            Directorio(directorio, &ext_blq_inodos);
            continue;
        }

        if (strcmp(orden, "rename") == 0) {
            if (Renombrar(directorio, &ext_blq_inodos, argumento1, argumento2) == 0) {
                grabardatos = 1;
            }
            continue;
        }

        if (strcmp(orden, "remove") == 0) {
            if (Borrar(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, argumento1, fent) == 0) {
                grabardatos = 1;
            }
            continue;
        }

        if (strcmp(orden, "copy") == 0) {
            if (Copiar(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, memdatos, argumento1, argumento2, fent) == 0) {
                grabardatos = 1;
            }
            continue;
        }

        if (strcmp(orden, "print") == 0) {
            Imprimir(directorio, &ext_blq_inodos, memdatos, argumento1);
            continue;
        }

        if (strcmp(orden, "exit") == 0) {
            if (grabardatos) {
                Grabarinodosydirectorio(directorio, &ext_blq_inodos, fent);
                GrabarByteMaps(&ext_bytemaps, fent);
                GrabarSuperBloque(&ext_superblock, fent);
                GrabarDatos(memdatos, fent);
            }
            fclose(fent);
            printf("Exiting...\n");
            return 0;
        }

        printf("Error: Unknown command '%s'\n", orden);
    }

    return 0;
}


void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup) {
    printf("Superblock Information:\n");
    printf("Total inodes: %d\n", psup->s_inodes_count);
    printf("Total blocks: %d\n", psup->s_blocks_count);
    printf("Free blocks: %d\n", psup->s_free_blocks_count);
    printf("Free inodes: %d\n", psup->s_free_inodes_count);
    printf("First data block: %d\n", psup->s_first_data_block);
    printf("Block size: %d bytes\n", psup->s_block_size);
}


void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps) {
    printf("Inode Bytemap:\n");
    for (int i = 0; i < MAX_INODOS; i++) {
        printf("%d ", ext_bytemaps->bmap_inodos[i]);
    }
    printf("\n\nBlock Bytemap (first 25 elements):\n");
    for (int i = 0; i < 25; i++) {
        printf("%d ", ext_bytemaps->bmap_bloques[i]);
    }
    printf("\n");
}


int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2) {
    char *token = strtok(strcomando, " \n");
    if (token == NULL) return -1;

    strcpy(orden, token);
    token = strtok(NULL, " \n");
    if (token) strcpy(argumento1, token);
    else argumento1[0] = '\0';

    token = strtok(NULL, " \n");
    if (token) strcpy(argumento2, token);
    else argumento2[0] = '\0';

    return 0;
}


void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos) {
    printf("Directory Contents:\n");
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo != NULL_INODO) {
            EXT_SIMPLE_INODE inode = inodos->blq_inodos[directorio[i].dir_inodo];
            printf("File: %s, Size: %d bytes, Inode: %d, Blocks: ", 
                directorio[i].dir_nfich, inode.size_fichero, directorio[i].dir_inodo);
            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
                if (inode.i_nbloque[j] != NULL_BLOQUE) {
                    printf("%d ", inode.i_nbloque[j]);
                }
            }
            printf("\n");
        }
    }
}


int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre) {
    int index = BuscaFich(directorio, inodos, nombre);
    if (index == -1) {
        printf("Error: File '%s' not found.\n", nombre);
        return -1;
    }
    EXT_SIMPLE_INODE *inode = &inodos->blq_inodos[directorio[index].dir_inodo];
    for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++) {
        if (inode->i_nbloque[i] != NULL_BLOQUE) {
            printf("%s", memdatos[inode->i_nbloque[i] - PRIM_BLOQUE_DATOS].dato);
        }
    }
    printf("\n");
    return 0;
}


int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps,
           EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre, FILE *fich) {
    int index = BuscaFich(directorio, inodos, nombre);
    if (index == -1) {
        printf("Error: File '%s' not found.\n", nombre);
        return -1;
    }
    EXT_SIMPLE_INODE *inode = &inodos->blq_inodos[directorio[index].dir_inodo];
    for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++) {
        if (inode->i_nbloque[i] != NULL_BLOQUE) {
            ext_bytemaps->bmap_bloques[inode->i_nbloque[i]] = 0;
            ext_superblock->s_free_blocks_count++;
            inode->i_nbloque[i] = NULL_BLOQUE;
        }
    }
    ext_bytemaps->bmap_inodos[directorio[index].dir_inodo] = 0;
    ext_superblock->s_free_inodes_count++;
    directorio[index].dir_nfich[0] = '\0';
    directorio[index].dir_inodo = NULL_INODO;
    return 0;
}


int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo) {
    int index = BuscaFich(directorio, inodos, nombreantiguo);
    if (index == -1) {
        printf("Error: File '%s' not found.\n", nombreantiguo);
        return -1;
    }
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (strcmp(directorio[i].dir_nfich, nombrenuevo) == 0) {
            printf("Error: File '%s' already exists.\n", nombrenuevo);
            return -1;
        }
    }
    strncpy(directorio[index].dir_nfich, nombrenuevo, LEN_NFICH - 1);
    directorio[index].dir_nfich[LEN_NFICH - 1] = '\0';
    return 0;
}


int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps,
           EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino, FILE *fich) {
    int srcIndex = BuscaFich(directorio, inodos, nombreorigen);
    if (srcIndex == -1) {
        printf("Error: Source file '%s' not found.\n", nombreorigen);
        return -1;
    }
    if (BuscaFich(directorio, inodos, nombredestino) != -1) {
        printf("Error: Destination file '%s' already exists.\n", nombredestino);
        return -1;
    }
    int newInodeIndex = -1;
    for (int i = 0; i < MAX_INODOS; i++) {
        if (ext_bytemaps->bmap_inodos[i] == 0) {
            newInodeIndex = i;
            break;
        }
    }
    if (newInodeIndex == -1) {
        printf("Error: No free inodes available.\n");
        return -1;
    }

   
    EXT_SIMPLE_INODE *srcInode = &inodos->blq_inodos[directorio[srcIndex].dir_inodo];
    EXT_SIMPLE_INODE *newInode = &inodos->blq_inodos[newInodeIndex];
    newInode->size_fichero = srcInode->size_fichero;
    for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++) {
        newInode->i_nbloque[i] = NULL_BLOQUE;
    }
    for (int i = 0; i < MAX_NUMS_BLOQUE_INODO && srcInode->i_nbloque[i] != NULL_BLOQUE; i++) {
        for (int j = PRIM_BLOQUE_DATOS; j < MAX_BLOQUES_PARTICION; j++) {
            if (ext_bytemaps->bmap_bloques[j] == 0) {
                ext_bytemaps->bmap_bloques[j] = 1;
                memcpy(&memdatos[j - PRIM_BLOQUE_DATOS], &memdatos[srcInode->i_nbloque[i] - PRIM_BLOQUE_DATOS], SIZE_BLOQUE);
                newInode->i_nbloque[i] = j;
                break;
            }
        }
    }
    ext_bytemaps->bmap_inodos[newInodeIndex] = 1;
    int destDirIndex = -1;
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo == NULL_INODO) {
            destDirIndex = i;
            break;
        }
    }
    if (destDirIndex == -1) {
        printf("Error: No space in directory.\n");
        return -1;
    }
    strcpy(directorio[destDirIndex].dir_nfich, nombredestino);
    directorio[destDirIndex].dir_inodo = newInodeIndex;
    return 0;
}


void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich) {
    fseek(fich, SIZE_BLOQUE * 3, SEEK_SET);
    fwrite(directorio, sizeof(EXT_ENTRADA_DIR) * MAX_FICHEROS, 1, fich);
    fseek(fich, SIZE_BLOQUE * 2, SEEK_SET);
    fwrite(inodos, sizeof(EXT_BLQ_INODOS), 1, fich);
}


void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich) {
    fseek(fich, SIZE_BLOQUE * 1, SEEK_SET);
    fwrite(ext_bytemaps, sizeof(EXT_BYTE_MAPS), 1, fich);
}


void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich) {
    fseek(fich, 0, SEEK_SET);
    fwrite(ext_superblock, sizeof(EXT_SIMPLE_SUPERBLOCK), 1, fich);
}


void GrabarDatos(EXT_DATOS *memdatos, FILE *fich) {
    fseek(fich, SIZE_BLOQUE * PRIM_BLOQUE_DATOS, SEEK_SET);
    fwrite(memdatos, sizeof(EXT_DATOS), MAX_BLOQUES_DATOS, fich);
}


int BuscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombre) {
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo != NULL_INODO && strcmp(directorio[i].dir_nfich, nombre) == 0) {
            return i;
        }
    }
    return -1; // File not found
}

