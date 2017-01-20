#include<xxd.h>
#include<serial.h>



void xxd(void * data, unsigned int len)
{
    unsigned int i, j;

    for(i = 0; i < len + ((len % DUMP_COLS) ? (DUMP_COLS - len % DUMP_COLS) : 0); i++) {
        /* print offset */
        if(i % DUMP_COLS == 0) {
            qemu_printf("0x%06x: ", i);
        }

        /* print hex data */
        if(i < len) {
            qemu_printf("%02x ", 0xFF & ((char*)data)[i]);
        }
        else {
            /* end of block, just aligning for ASCII dump */
            qemu_printf("   ");
        }

        /* print ASCII dump */
        if(i % DUMP_COLS == (DUMP_COLS - 1)) {
            for(j = i - (DUMP_COLS - 1); j <= i; j++) {
                if(j >= len) {
                    write_serial(' ');
                }
                else if(isprint(((char*)data)[j])) /* printable char */ {
                    write_serial(0xFF & ((char*)data)[j]);
                }
                else {
                    write_serial('.');
                }
            }
            write_serial('\n');
        }
    }
}
