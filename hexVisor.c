#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <curses.h>
#include <sys/stat.h>
#include <sys/mman.h>

char *hazLinea(char *base, int dir, long lnHex)
{
    char linea[100]; // La linea es mas pequeña
    int o = 0;
    // Muestra 16 caracteres por cada linea
    o += sprintf(linea, "%08x ", lnHex * 16); // offset en hexadecimal
    for (int i = 0; i < 4; i++)
    {
        unsigned char a, b, c, d;
        a = base[dir + 4 * i + 0];
        b = base[dir + 4 * i + 1];
        c = base[dir + 4 * i + 2];
        d = base[dir + 4 * i + 3];
        o += sprintf(&linea[o], "%02x %02x %02x %02x ", a, b, c, d);
    }
    for (int i = 0; i < 16; i++)
    {
        if (isprint(base[dir + i]))
        {
            o += sprintf(&linea[o], "%c", base[dir + i]);
        }
        else
        {
            o += sprintf(&linea[o], ".");
        }
    }
    sprintf(&linea[o], "\n");

    return (strdup(linea));
}

void show(char *map, long lnHex)
{
    for (int i = 0; i < 25; i++)
    {
        // Haz linea, base y offset
        char *l = hazLinea(map, i * 16, lnHex++);
        addstr(l);
    }

    /* END */
    move(getmaxy(stdscr) - 4, 1);
    addstr("Controles:\n  ^ v < > : navegar.\t\t\t 'g' : Saltar a direccion.\n 'a' : Ir al inicio de la lectura.\t 'z' : Ir al final de la lectura\n 's' : guardar archivo\t\t\t BACKSPACE : atras");
}

int lee(char *map, long tam, char *ptr, char *nombre, struct ext4_extent *ex)
{
    int row, col;
    long lnHex = 0L;
    short offset = 0;
    char offsetStr[20];
    clear();

    char *inicio = map;
    char buffer[80], temp[20];
    long jump, lastLine = (tam / 16) - 1;

    show(map, lnHex);

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
                map -= 16;
                move(0, 0);
                show(map, --lnHex);
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
                map += 16;
                move(0, 0);
                show(map, ++lnHex);
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
                    map = inicio + (lastLine - 24) * 16;
                    lnHex = lastLine - 24;
                }
                else
                {
                    map = inicio + jump;
                    lnHex = (jump / 0x10);
                }
                move(0, 0);
                show(map, lnHex);

                move(26, 9);
                sprintf(offsetStr, "%s %x", "Offset:", offset);
                addstr(offsetStr);

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
            memset(temp, 0, sizeof(temp));

            break;
        case 'a':
            map = inicio;
            lnHex = 0;
            clear();
            show(map, lnHex);
            row = 0;
            col = 9;
            break;
        case 'z':
            map = inicio + (lastLine - 24) * 16;
            lnHex = lastLine - 24;
            clear();
            show(map, lnHex);
            row = 24;
            col = 9;
            break;
        /* Modificacion inciso 3 */
        case 's':
                sprintf(buffer, "%s", nombre);
                FILE *file = fopen(buffer, "w");
                memset(buffer, 0, sizeof(buffer));

                if (file == NULL)
                {
                    perror("Error al crear el archivo");
                    return -2;
                }

                int temp = 0;
                while(ex[temp].ee_start_lo != 0) {
                    fwrite((ptr + (ex[temp].ee_start_lo * 0x400)), 1, (ex[temp].ee_len * 0x400), file);
                    temp++;
                }

                fclose(file);

                move(27, 9);
                addstr("Archivo guardado");

                break;
        /* Fin modificacion inciso 3 */
        default:
            break;
        }
        move(row, col);
        c = getch();
    }

    return 0;
}
