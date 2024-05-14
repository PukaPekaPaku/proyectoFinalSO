#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <curses.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "ext4.h"

/* Variables globales */
int fd;  /* Archivo a leer */
long fs; /* Tamaño de archivo */
int term_x, term_y;

/* Funciones */
char *mapFile(char *filePath)
{
    /* Abre archivo */
    fd = open(filePath, O_RDONLY);
    if (fd == -1)
    {
        perror("Error abriendo el archivo");
        return (NULL);
    }

    /* Mapea archivo */
    struct stat st;
    fstat(fd, &st);
    fs = st.st_size;

    char *map = mmap(0, fs, PROT_READ, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED)
    {
        close(fd);
        perror("Error mapeando el archivo");
        return (NULL);
    }

    return map;
}

void showMenu()
{
    clear();
    move(1, 1);
    addstr("Opciones");
    move(3, 1);
    addstr("1: Particiones\n");
    move(4, 1);
    addstr("2: Superblock\n");
    move(5, 1);
    addstr("3: Explorar particion");

    move(getmaxy(stdscr) - 4, 1);
    addstr("Controles:\n  ^ v   : navegar.\n  ENTER : elegir seleccion.\n  'q'   : salir");
}

void particiones(char *ptr)
{
    unsigned char hi, si, hf, sf;
    unsigned short ci, cf;
    unsigned int sector, size;
    char buffer[20], *checkpoint;

    clear();
    move(1, 1);
    addstr("Particion  Ci   Hi   Si   Sector inicio   Cf   Hf   Sf   Tamaño\n");

    for (int row = 2; row < 6; row++)
    {

        /**
         * Status
         * 0x00
         */
        checkpoint = ptr;

        ptr++;
        /**
         * Cylinder-Head-Sector i
         * Head
         * 0x01
         */
        hi = *ptr;

        ptr++;
        /**
         * Sector
         * 0x02 - 0b__543210
         */
        si = *ptr & 0x3F;

        /**
         * Cylinder
         * 0x02 - 0b98______
         * 0x03 - 0b76543210
         */
        ci = *(ptr + 1) | ((*ptr & 0xC0) << 2);

        ptr += 2;
        /**
         * Partition type
         * 0x04
         */

        ptr++;
        /**
         * Cylinder-Head-Sector f
         * Head
         * 0x05
         */
        hf = *ptr;

        ptr++;
        /**
         * Sector
         * 0x06 - 0b__543210
         */
        sf = *ptr & 0x3F;

        /**
         * Cylinder
         * 0x06 - 0b98______
         * 0x07 - 0b76543210
         */
        cf = *(ptr + 1) | ((*ptr & 0xC0) << 2);

        ptr += 2;
        /**
         * "LBA of first absolute sector in the partition"
         * 4bytes (little endian)
         * 0x08
         * 0x09
         * 0x0A
         * 0x0B
         */
        sector = (unsigned char)*ptr | ((unsigned char)*(ptr + 1) << 8) | ((unsigned char)*(ptr + 2) << 16) | ((unsigned char)*(ptr + 3) << 24);

        ptr += 4;
        /**
         * "Number of sectors in partition"
         * 4bytes (little endian)
         * 0x0C
         * 0x0D
         * 0x0E
         * 0x0F
         */
        size = (unsigned char)*ptr | ((unsigned char)*(ptr + 1) << 8) | ((unsigned char)*(ptr + 2) << 16) | ((unsigned char)*(ptr + 3) << 24);

        //  Particion  Ci   Hi   Si   Sector inicio   Cf   Hf   Sf   Tamaño\n
        // 0_________0_2____7__0_2____7__0_________0__3____8_0__3____8
        move(row, 12);
        sprintf(buffer, "%hu", ci);
        addstr(buffer);
        memset(buffer, 0, sizeof(buffer));

        move(row, 17);
        sprintf(buffer, "%u", hi);
        addstr(buffer);
        memset(buffer, 0, sizeof(buffer));

        move(row, 22);
        sprintf(buffer, "%u", si);
        addstr(buffer);
        memset(buffer, 0, sizeof(buffer));

        move(row, 27);
        if (sector)
        {
            sprintf(buffer, "%u", sector);
            addstr(buffer);
            memset(buffer, 0, sizeof(buffer));
        }
        else
        {
            addstr("Sin particion");
        }

        move(row, 43);
        sprintf(buffer, "%hu", cf);
        addstr(buffer);
        memset(buffer, 0, sizeof(buffer));

        move(row, 48);
        sprintf(buffer, "%u", hf);
        addstr(buffer);
        memset(buffer, 0, sizeof(buffer));

        move(row, 53);
        sprintf(buffer, "%u", sf);
        addstr(buffer);
        memset(buffer, 0, sizeof(buffer));

        move(row, 58);
        sprintf(buffer, "%u", size);
        addstr(buffer);
        memset(buffer, 0, sizeof(buffer));

        ptr = checkpoint + 0x10;
    }

    move(getmaxy(stdscr) - 2, 1);
    addstr("Controles:\n  'q'   : salir");

    int c = getch();
    while (c != 'q')
    {
        c = getch();
        /* Exit on 'q' */
    }
}

void superbloque(char *ptr)
{
    int row = 1;
    char buffer[80];

    clear();
    move(row++, 1);

    addstr("Superblock\n");
    struct ext4_super_block *superbloque = (struct ext4_super_block *)ptr;

    move(++row, 1);
    sprintf(buffer, "%s%u\n", "Recuento Inodes: ", superbloque->s_inodes_count);
    addstr(buffer);
    memset(buffer, 0, sizeof(buffer));

    move(++row, 1);
    sprintf(buffer, "%s%u\n", "Recuento Blocks: ",superbloque->s_blocks_count_lo);
    addstr(buffer);
    memset(buffer, 0, sizeof(buffer));

    move(++row, 1);
    sprintf(buffer, "%s%s\n", "Etiqueta: ",superbloque->s_volume_name);
    addstr(buffer);
    memset(buffer, 0, sizeof(buffer));

    move(++row, 1);
    sprintf(buffer, "%s%s\n", "Montado por ultima vez en: ",superbloque->s_last_mounted);
    addstr(buffer);
    memset(buffer, 0, sizeof(buffer));

    move(++row, 1);
    sprintf(buffer, "%s%u\n", "Primer Inode: ",superbloque->s_first_ino);
    addstr(buffer);
    memset(buffer, 0, sizeof(buffer));

    move(++row, 1);
    sprintf(buffer, "%s%u\n", "Block size: ",sizeof(struct ext4_super_block));
    addstr(buffer);
    memset(buffer, 0, sizeof(buffer));

    move(++row, 1);
    sprintf(buffer, "%s%u\n", "Inode size: ",superbloque->s_inode_size);
    addstr(buffer);
    memset(buffer, 0, sizeof(buffer));

    move(++row, 1);
    sprintf(buffer, "%s%u\n", "Blocks per group: ",superbloque->s_blocks_per_group);
    addstr(buffer);
    memset(buffer, 0, sizeof(buffer));

    move(++row, 1);
    sprintf(buffer, "%s%u\n", "Inodes per group: ",superbloque->s_inodes_per_group);
    addstr(buffer);
    memset(buffer, 0, sizeof(buffer));

    move(++row, 1);
    sprintf(buffer, "%s%X\n", "Magic number: ",superbloque->s_magic);
    addstr(buffer);
    memset(buffer, 0, sizeof(buffer));

    /* END */

    move(getmaxy(stdscr) - 2, 1);
    addstr("Controles:\n  'q'   : salir");

    int c = getch();
    while (c != 'q')
    {
        c = getch();
        /* Exit on 'q' */
    }
}

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        printf("Uso: %s <archivo> \n", argv[0]);
        return (-1);
    }

    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    /* Mapeo de archivo */
    char *ptr = mapFile(argv[1]);
    if (ptr == NULL)
    {
        exit(EXIT_FAILURE);
    }

    /* "Checkpoint" para desmapear */
    char *ptr_ini = ptr;

    /* Menu */
    showMenu();
    int row = 3, col = 1;
    move(row, col);
    refresh();

    char aux[20];

    int c = getch();
    while (c != 'q')
    {
        switch (c)
        {
        case KEY_UP:
            if (row > 3)
            {
                row--;
            }
            break;
        case KEY_DOWN:
            if (row < 5)
            {
                row++;
            }
            break;
        case 10:
            switch (row - 2)
            {
            case 1:
                particiones(ptr + 0x01BE);
                break;
            case 2:
                // Hard-coded :/
                superbloque(ptr + 0x100400);
                break;

            default:
                break;
            }
            clear();
            showMenu();
            break;

        default:
            
            break;
        }
        move(row, col);
        c = getch();
    }

    /* Desmapeo */
    if (munmap(ptr_ini, fd) == -1)
    {
        perror("Error al desmapear");
    }
    close(fd);

    endwin();

    return 0;
}
