#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <curses.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <math.h>

#include "ext4.h"
#include "hexVisor.c"

/* Variables globales */
int fd;  /* Archivo a leer */
long fs; /* Tamaño de archivo */

/* Funciones */
char *mapFile(char *filePath);
void showMenu();
void particiones(char *ptr);
void superbloque(char *ptr);
void showGDT(char *ptr);
void gdt(char *ptr);
int lee_inode(struct ext4_inode *ptrInode, char *ptr, char *nombre);

/* Funciones del lector en hexadecimal */
char *hazLinea(char *base, int dir, long lnHex);
void show(char *map, long lnHex);

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
    mvprintw(1, 1, "Opciones");
    mvprintw(3, 1, "1: Particiones\n");
    mvprintw(4, 1, "2: Superblock particion Linux\n");
    mvprintw(5, 1, "3: Explorar particion Linux\n");

    move(getmaxy(stdscr) - 4, 1);
    addstr("Controles:\n  ^ v   : navegar.\n  ENTER : elegir seleccion.\n  'q'   : salir");
}

void particiones(char *ptr)
{
    unsigned char hi, si, hf, sf, type;
    unsigned short ci, cf;
    __le32 sector, size;
    char *checkpoint;

    clear();
    move(1, 1);
    addstr("Particion  Ci   Hi   Si   Sector inicio   Cf   Hf   Sf   Tamaño   Tipo\n");

    for (int row = 2; row < 6; row++)
    {

        checkpoint = ptr;
        /**
         * Status
         * 0x00
         */

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
        type = *ptr;

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
        sector = *(__le32 *)ptr;

        ptr += 4;
        /**
         * "Number of sectors in partition"
         * 4bytes (little endian)
         * 0x0C
         * 0x0D
         * 0x0E
         * 0x0F
         */
        size = *(__le32 *)ptr;

        //  Particion  Ci   Hi   Si   Sector inicio   Cf   Hf   Sf   Tamaño   Tipo\n
        // 0_________0_2____7__0_2____7__0_________0__3____8_0__3____8_0______7
        mvprintw(row, 12, "%hu", ci);

        mvprintw(row, 17, "%u", hi);

        mvprintw(row, 22, "%u", si);

        if (sector)
        {
            mvprintw(row, 27, "%u", sector);
        }
        else
        {
            mvprintw(row, 27, "-");
        }

        mvprintw(row, 43, "%hu", cf);

        mvprintw(row, 48, "%hu", hf);

        mvprintw(row, 53, "%hu", sf);

        mvprintw(row, 58, "%u", size);

        mvprintw(row, 67, "%X%c", type, 'h');

        ptr = checkpoint + 0x10;
    }

    move(getmaxy(stdscr) - 2, 1);
    addstr("Controles:\n  BACKSPACE : atras");

    int c = getch();
    while (c != KEY_BACKSPACE)
    {
        c = getch();
        /* Exit on BACKSPACE */
    }
}

void superbloque(char *ptr)
{
    int row = 1;

    clear();
    move(row++, 1);

    addstr("Superblock\n");
    struct ext4_super_block superbloque;
    memcpy(&superbloque, &ptr[0 + 0x400], sizeof(struct ext4_super_block));

    mvprintw(++row, 1, "Recuendo Inodes: %u", superbloque.s_inodes_count);

    mvprintw(++row, 1, "Recuento Blocks: %u", superbloque.s_blocks_count_lo);

    mvprintw(++row, 1, "Etiqueta: %s", superbloque.s_volume_name);

    mvprintw(++row, 1, "Montado por ultima vez en: %s", superbloque.s_last_mounted);

    mvprintw(++row, 1, "Primer Inode: %u", superbloque.s_first_ino);

    mvprintw(++row, 1, "Block size: %u", (unsigned short)pow((double)2, (double)(10 + superbloque.s_log_block_size)));

    mvprintw(++row, 1, "Inode size: %u", superbloque.s_inode_size);

    mvprintw(++row, 1, "Blocks per group: %u", superbloque.s_blocks_per_group);

    mvprintw(++row, 1, "Inodes per group: %u", superbloque.s_inodes_per_group);

    mvprintw(++row, 1, "Magic number: %X", superbloque.s_magic);

    /* END */
    move(getmaxy(stdscr) - 2, 1);
    addstr("Controles:\n  BACKSPACE : atras");

    int c = getch();
    while (c != KEY_BACKSPACE)
    {
        c = getch();
        /* Exit on BACKSPACE */
    }
}

void showGDT(char *ptr)
{
    int row = 1;
    clear();
    move(row++, 1);
    addstr("Primer Block Group Descriptor\n");

    struct ext4_group_desc gdt;
    memcpy(&gdt, &ptr[0 + 0x800], sizeof(struct ext4_group_desc));

    mvprintw(++row, 1, "Recuendo Inodes: %hu", gdt.bg_used_dirs_count_lo);

    mvprintw(++row, 1, "Bitmap block: %u", gdt.bg_block_bitmap_lo);

    mvprintw(++row, 1, "Inode table block: %u", gdt.bg_inode_table_lo);

    /* END */
    move(getmaxy(stdscr) - 3, 1);
    addstr("Controles:\n  ENTER     : siguiente\n  BACKSPACE : atras");
}

void gdt(char *ptr)
{
    struct ext4_group_desc gdt;
    memcpy(&gdt, &ptr[0 + 0x800], sizeof(struct ext4_group_desc));

    struct ext4_inode inode;
    memcpy(&inode, (ptr + (0x400 * gdt.bg_inode_table_lo) + 0x100), sizeof(struct ext4_inode));

    showGDT(ptr);

    int c = getch();
    while (1)
    {
        switch (c)
        {
        case KEY_BACKSPACE:
            return;
            break;

        case 10:
            lee_inode(&inode, ptr, "/");
            break;

        default:
            break;
        }

        showGDT(ptr);

        c = getch();
    }
}

int lee_inode(struct ext4_inode *ptrInode, char *ptr, char *nombre)
{
    if (((ptrInode->i_mode) & 0x4000) == 0x4000)
    {
        /* Estamos leyendo un directorio */
        struct ext4_extent_header eh;
        memcpy(&eh, &(ptrInode->i_block[0]), sizeof(struct ext4_extent_header));

        struct ext4_extent ex;
        memcpy(&ex, &(ptrInode->i_block[3]), sizeof(struct ext4_extent));

        unsigned short numEntries = 0;
        struct ext4_dir_entry_2 auxDirEntry;

        char *ptrDirs = (ptr + (ex.ee_start_lo * 0x400));

        memcpy(&auxDirEntry, ptrDirs, sizeof(struct ext4_dir_entry_2));

        while (auxDirEntry.inode != 0)
        {
            numEntries++;
            ptrDirs += auxDirEntry.rec_len;
            memcpy(&auxDirEntry, ptrDirs, sizeof(struct ext4_dir_entry_2));
        }

        ptrDirs = (ptr + (ex.ee_start_lo * 0x400));
        struct ext4_dir_entry_2 dirEntries[numEntries];
        char *status[numEntries];

        for (int temp = 0; temp < numEntries; temp++)
        {
            status[temp] = "";
        }

        int i = 0;
        while (i < numEntries)
        {
            memcpy(&dirEntries[i], ptrDirs, sizeof(struct ext4_dir_entry_2));
            ptrDirs += dirEntries[i].rec_len;
            i++;
        }

        /* ----------------------------------------------------------------------------------------------- */
        int row;
    printScreen:
        row = 0;
        clear();
        move(++row, 1);
        addstr("Directorio");
        row++;

        int j = 0;
        while (j < numEntries)
        {
            mvprintw(++row, 1, "%s -> %u %s", dirEntries[j].name, dirEntries[j].inode, status[j]);
            j++;
        }

        /* END */
        move(getmaxy(stdscr) - 4, 1);
        addstr("Controles:\n  ^ v       : navegar.\n  ENTER     : elegir seleccion.\n  BACKSPACE : atras");
        row = 3;
        move(row, 1);
        /* ----------------------------------------------------------------------------------------------- */

        int c = getch();
        while (1)
        {
            switch (c)
            {
            case KEY_BACKSPACE: /* Exit on BACKSPACE */
                return 0;
                break;

            case KEY_UP:
                if (row > 3)
                {
                    row--;
                }
                break;

            case KEY_DOWN:
                if ((row - 2) < numEntries)
                {
                    row++;
                }
                break;

            case 10:
                /* Elemento seleccionado */
                unsigned int gdOfSelected = (dirEntries[row - 3].inode - 1) / 0x7F8;
                unsigned int offsetOfSelected = (dirEntries[row - 3].inode - 1) % 0x7F8;

                struct ext4_group_desc gd;
                memcpy(&gd, (ptr + 0x800 + (gdOfSelected * sizeof(struct ext4_group_desc))), sizeof(struct ext4_group_desc));

                struct ext4_inode inodeN;
                memcpy(&inodeN, (ptr + (0x400 * gd.bg_inode_table_lo) + (offsetOfSelected * 0x100)), sizeof(struct ext4_inode));

                /**
                 * Return   Status
                 * -1       Lectura a archivo vacio
                 * 0        Lectura a directorio
                 * 1        Lectura a archivo con salida estandar
                 */

                int out = lee_inode(&inodeN, ptr, dirEntries[row - 3].name);
                switch (out)
                {
                case -1:
                    status[row - 3] = " - Archivo vacio";
                    goto printScreen;
                    break;
                case 1:
                    goto printScreen;
                    break;

                default:
                    return 0;
                    break;
                }
                break;

            default:
                break;
            }
            move(row, 1);
            c = getch();
        }
    }
    else if (((ptrInode->i_mode) & 0x8000) == 0x8000)
    {
        /* Estamos leyendo un archivo */
        struct ext4_extent_header eh;
        memcpy(&eh, &(ptrInode->i_block[0]), sizeof(struct ext4_extent_header));

        if (eh.eh_entries == 0)
        {
            return -1;
        }

        struct ext4_extent ex[eh.eh_entries];

        int i = 0;
        for (i = 0; i < eh.eh_entries; i++)
        {
            memcpy(&(ex[i]), &(ptrInode->i_block[(i + 1) * 3]), sizeof(struct ext4_extent));
        }

        // Visualizamos la primera parte del archivo
        char *ptrFile = (ptr + (ex[0].ee_start_lo * 0x400));
        long tam = (long)(ex[0].ee_len * 0x400);

        // lee(ptrFile, tam);

        /* Funcion lee */
        int row, col;
        long lnHex = 0L;
        short offset = 0;
        clear();

        char *inicio = ptrFile;
        char buffer[256];
        long jump, lastLine = (tam / 16) - 1;

        show(ptrFile, lnHex);

        mvprintw(26, 9, "Offset:%x", offset);

        row = 0;
        col = 9;
        move(row, col);
        refresh();

        int c = getch();

        while (c != KEY_BACKSPACE)
        {
            switch (c)
            {
            case KEY_LEFT:
                if (col > 9)
                {
                    col -= 3;
                    offset--;
                    mvprintw(26, 9, "Offset:%x", offset);

                    move(27, 0);
                    clrtoeol();
                }
                break;
            case KEY_RIGHT:
                if (col < 54)
                {
                    col += 3;
                    offset++;
                    mvprintw(26, 9, "Offset:%x", offset);

                    move(27, 0);
                    clrtoeol();
                }
                break;
            case KEY_UP:
                if (row > 0)
                {
                    row--;
                }
                else if (lnHex > 0)
                {
                    ptrFile -= 16;
                    move(0, 0);
                    show(ptrFile, --lnHex);
                }
                move(27, 0);
                clrtoeol();
                break;
            case KEY_DOWN:
                if (row < 24)
                {
                    row++;
                }
                else if (lnHex < (lastLine - 24))
                {
                    ptrFile += 16;
                    move(0, 0);
                    show(ptrFile, ++lnHex);
                }
                else
                {
                    move(27, 9);
                    addstr("Fin de lectura. Guarde el archivo para visualizarlo en su totalidad.");
                }
                move(27, 0);
                clrtoeol();
                break;
            case 'g':

                move(27, 9);
                addstr("Saltar a direccion: 0x");

                move(27, 31);
                clrtoeol();
                echo();
                getstr(buffer);
                noecho();
                jump = strtol(buffer, NULL, 16);
                offset = jump % 0x10;
                jump -= offset;

                if ((jump / 0x10) <= lastLine)
                {
                    if ((jump / 0x10) >= lastLine - 24)
                    {
                        ptrFile = inicio + (lastLine - 24) * 16;
                        lnHex = lastLine - 24;
                    }
                    else
                    {
                        ptrFile = inicio + jump;
                        lnHex = (jump / 0x10);
                    }
                    move(0, 0);
                    show(ptrFile, lnHex);

                    mvprintw(26, 9, "Offset: %x", offset);

                    row = 0;
                    col = 9 + (offset * 3);
                }
                else
                {
                    move(27, 9);
                    clrtoeol();
                    addstr("Salto fuera del archivo");
                }

                memset(buffer, 0, sizeof(buffer));
                break;
            case 'a':
                ptrFile = inicio;
                lnHex = 0;
                clear();
                show(ptrFile, lnHex);
                row = 0;
                col = 9;
                break;
            case 'z':
                ptrFile = inicio + (lastLine - 24) * 16;
                lnHex = lastLine - 24;
                clear();
                show(ptrFile, lnHex);
                row = 24;
                col = 9;
                break;
            case 's':
                sprintf(buffer, "%s", nombre);
                FILE *file = fopen(buffer, "w");
                memset(buffer, 0, sizeof(buffer));

                if (file == NULL)
                {
                    perror("Error al crear el archivo");
                    return -2;
                }

                for (int temp = 0; temp < eh.eh_entries; temp++)
                {
                    fwrite((ptr + (ex[temp].ee_start_lo * 0x400)), 1, (ex[temp].ee_len * 0x400), file);
                }

                fclose(file);

                move(27, 9);
                addstr("Archivo guardado");

                break;
            default:
                break;
            }
            move(row, col);
            c = getch();
        }

        return 1;
    }
    else
    {
        return -2;
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
                superbloque(ptr + 0x100000);
                break;
            case 3:
                gdt(ptr + 0x100000);
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
