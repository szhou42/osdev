#include <e1000.h>
#include <pci.h>
#include <printf.h>
#include <string.h>

pci_dev_t pci_e1000_device;
e1000_dev_t e1000_device;



void e1000_linkup() {
    uint32_t val;
    val = e1000_read_command(REG_CTRL);
    e1000_write_command(REG_CTRL, val | ECTRL_SLU);
    val = e1000_read_command(REG_CTRL);
}

uint32_t e1000_read_command(uint16_t addr) {
    if(e1000_device.bar_type == 0) {
        return in_meml(e1000_device.mem_base + addr);
    }
    else {
        outportl(e1000_device.io_base, addr);
        return inportl(e1000_device.io_base + addr + 4);
    }
}

void e1000_write_command(uint16_t addr, uint32_t value) {
    if(e1000_device.bar_type == 0) {
        out_meml(e1000_device.mem_base + addr, value);
    }
    else {
        outportl(e1000_device.io_base, addr);
        outportl(e1000_device.io_base + addr + 4, value);
    }
}

void detect_eeprom() {
    uint32_t t;
    e1000_write_command(REG_EEPROM, 0x1);
    for(int i = 0; i < 1000 && e1000_device.eeprom_exist == 0; i++) {
        t = e1000_read_command(REG_EEPROM);
        if(t & 0x10)
            e1000_device.eeprom_exist = 1;
    }
}

uint32_t eeprom_read(uint8_t addr) {
    uint16_t data = 0;
    uint32_t tmp = 0;
    if (e1000_device.eeprom_exist) {
        e1000_write_command(REG_EEPROM, (1) | ((uint32_t)(addr) << 8) );
        while(!((tmp = e1000_read_command(REG_EEPROM)) & (1 << 4)) );
    }
    else {
        e1000_write_command( REG_EEPROM, (1) | ((uint32_t)(addr) << 2) );
        while(!((tmp = e1000_read_command(REG_EEPROM)) & (1 << 1)) );
    }
    data = (uint16_t)((tmp >> 16) & 0xFFFF);
    return data;
}

void read_mac_addr() {
    uint32_t t;
    if(e1000_device.eeprom_exist) {
        t = eeprom_read(0);
        e1000_device.mac_addr[0] = t & 0xff;
        e1000_device.mac_addr[1] = t >> 8;

        t = eeprom_read(1);
        e1000_device.mac_addr[2] = t & 0xff;
        e1000_device.mac_addr[3] = t >> 8;

        t = eeprom_read(2);
        e1000_device.mac_addr[4] = t & 0xff;
        e1000_device.mac_addr[5] = t >> 8;

    }
    else {
        uint8_t * mem_base_mac_8 = (uint8_t *) (e1000_device.mem_base+0x5400);
        uint32_t * mem_base_mac_32 = (uint32_t *) (e1000_device.mem_base+0x5400);
        if (mem_base_mac_32[0] != 0 ) {
            for(int i = 0; i < 6; i++) {
                e1000_device.mac_addr[i] = mem_base_mac_8[i];
            }
        }
    }
    printf("Your mac addr is: ");
    for(int i = 0; i < 6; i++)
        printf("%x ", e1000_device.mac_addr[i]);
    printf("\n");
}

void get_mac_addr(uint8_t * src_mac_addr) {
    memcpy(src_mac_addr, e1000_device.mac_addr, 6);
}

void enable_interrupt() {
    e1000_write_command(REG_IMASK ,0x1F6DC);
    e1000_write_command(REG_IMASK ,0xff & ~4);
    e1000_read_command(0xc0);
}

void e1000_handler(register_t * reg) {
    uint32_t status = e1000_read_command(0xc0);
    printf("e1000 handler was fired, status = %d\n", status);
    if(status & 0x04) {
        printf("Start link\n");
        e1000_linkup();
    }

    if(status & 0x10) {
        printf("Threshold\n");
    }
    // Receive
    if(status & 0x80) {
        while(e1000_device.rx_descs[e1000_device.rx_cur]->status & 0x1) {
            uint8_t * buf = (uint8_t *)e1000_device.rx_descs[e1000_device.rx_cur]->addr;
            uint16_t len = e1000_device.rx_descs[e1000_device.rx_cur]->length;

            printf("Received: %s\n", buf);
            // Your packet is in buf now!

            e1000_device.rx_descs[e1000_device.rx_cur]->status = 0;
            e1000_write_command(REG_RXDESCTAIL, e1000_device.rx_cur);
            e1000_device.rx_cur = (e1000_device.rx_cur + 1) % E1000_NUM_RX_DESC;
        }
    }
}

void e1000_send_packet(void * p_data, uint16_t p_len) {
    e1000_device.tx_descs[e1000_device.tx_cur]->addr = (uint64_t)virtual2phys(kpage_dir, p_data);
    e1000_device.tx_descs[e1000_device.tx_cur]->length = p_len;
    e1000_device.tx_descs[e1000_device.tx_cur]->cmd = CMD_EOP | CMD_IFCS | CMD_RS;
    e1000_device.tx_descs[e1000_device.tx_cur]->status = 0x0;
    uint8_t old_cur = e1000_device.tx_cur;
    e1000_device.tx_cur = (e1000_device.tx_cur + 1) % E1000_NUM_TX_DESC;

    uint32_t head = e1000_read_command(REG_TXDESCHEAD);
    uint32_t tail = e1000_read_command(REG_TXDESCTAIL);
    e1000_write_command(REG_TXDESCTAIL, e1000_device.tx_cur);

    head = e1000_read_command(REG_TXDESCHEAD);
    tail = e1000_read_command(REG_TXDESCTAIL);
    while(!(e1000_device.tx_descs[old_cur]->status & 0xff)) {
        //printf("Packet not send yet... status: %d\n", e1000_device.tx_descs[old_cur]->status);
        head = e1000_read_command(REG_TXDESCHEAD);
        tail = e1000_read_command(REG_TXDESCTAIL);
        printf("head = %d, tail = %d\n", head, tail);
    }
    printf("PACKET SEND SUCCESS!\n");
}

/*
 * Receive buffer descriptor init
 */
void rx_init() {
    void * virtual_addr = kmalloc(sizeof(rx_desc_t) * E1000_NUM_RX_DESC + 16);
    void * phys_addr = virtual2phys(kpage_dir, virtual_addr);

    for(int i = 0; i < E1000_NUM_RX_DESC; i++) {
        e1000_device.rx_descs[i] = (rx_desc_t *)((uint8_t *)virtual_addr + i*16);
        e1000_device.rx_descs[i]->addr = (uint64_t)virtual2phys(kpage_dir, kmalloc(8192 + 16));
        e1000_device.rx_descs[i]->status = 0;
    }
    e1000_write_command(REG_RXDESCLO, (uint32_t)phys_addr);
    e1000_write_command(REG_RXDESCHI, 0);
    e1000_write_command(REG_RXDESCLEN, E1000_NUM_RX_DESC * 16);
    e1000_write_command(REG_RXDESCHEAD, 0);
    e1000_write_command(REG_RXDESCTAIL, E1000_NUM_RX_DESC);
    e1000_write_command(REG_RCTRL, RCTL_EN| RCTL_SBP| RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC  | RCTL_BSIZE_2048);
    e1000_device.rx_cur = 0;
}

/*
 * Transfer buffer descriptor init
 */
void tx_init() {
    void * virtual_addr = kmalloc(sizeof(tx_desc_t) * E1000_NUM_TX_DESC + 16);
    void * phys_addr = virtual2phys(kpage_dir, virtual_addr);
    for(int i = 0; i < E1000_NUM_TX_DESC; i++) {
        e1000_device.tx_descs[i] = (tx_desc_t *)((void *)virtual_addr + i * 16);
        e1000_device.tx_descs[i]->addr = 0;
        e1000_device.tx_descs[i]->cmd = 0;
        e1000_device.tx_descs[i]->status = TSTA_DD;
    }
    e1000_write_command(REG_TXDESCLO, (uint32_t)phys_addr);
    e1000_write_command(REG_TXDESCHI, 0);
    e1000_write_command(REG_TXDESCLEN, E1000_NUM_TX_DESC * 16);
    e1000_write_command(REG_TXDESCHEAD, 0);
    e1000_write_command(REG_TXDESCTAIL, 0);
    e1000_write_command(REG_TCTRL,  TCTL_EN | TCTL_PSP | (15 << TCTL_CT_SHIFT) | (64 << TCTL_COLD_SHIFT) | TCTL_RTLC);
    //e1000_write_command(REG_TCTRL, 0b0110000000000111111000011111010);
    e1000_write_command(REG_TIPG,  0x0060200A);
    e1000_device.tx_cur = 0;
}

/*
 * Initialize the e1000 card driver
 * */
void e1000_init() {
    // First get the network device using PCI
    pci_e1000_device = pci_get_device(e1000_VENDOR_ID, E1000_DEVICE_ID, -1);
    uint32_t ret = pci_read(pci_e1000_device, PCI_BAR0);
    e1000_device.bar_type = ret & 0x1;
    printf("E1000 use %s access\n", (e1000_device.bar_type == 0)? "mem based":"port based");
    // Get io base or mem base by extracting the high 28/30 bits
    e1000_device.io_base = ret & (~0x3);
    e1000_device.mem_base = ret & (~0xf);

    // Identity map the page around mem_base``
    allocate_region(kpage_dir, e1000_device.mem_base, e1000_device.mem_base + 12 * PAGE_SIZE, 1, 1, 1);

    // Enable PCI Bus Mastering
    uint32_t pci_command_reg = pci_read(pci_e1000_device, PCI_COMMAND);
    if(!(pci_command_reg & (1 << 2))) {
        pci_command_reg |= (1 << 2);
        pci_write(pci_e1000_device, PCI_COMMAND, pci_command_reg);
    }

    // Detect EEPROM and read mac address
    detect_eeprom();
    read_mac_addr();

    // Register and enable network interrupts
    uint32_t irq_num = pci_read(pci_e1000_device, PCI_INTERRUPT_LINE);
    register_interrupt_handler(32 + irq_num, e1000_handler);
    enable_interrupt();

    e1000_linkup();

    for(int i = 0; i < 0x80; i++)
        e1000_write_command(0x5200 + i*4, 0);
    rx_init();
    tx_init();
}
