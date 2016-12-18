#include <ethernet.h>
#include <pci.h>

pci_dev_t pci_ethernet_device;
ethernet_dev_t ethernet_device;





void ethernet_read_command(uint16_t addr) {
    if(ethernet_device.bar_type == 0) {
        return in_meml(ethernet_device.mem_base + addr);
    }
    else {
        outportl(ethernet_device.io_base, addr);
        inportl(ethernet_device.io_base + addr + 4);
    }
}

void ethernet_write_command(uint16_t addr, uint16_t value) {
    if(ethernet_device.bar_type == 0) {
        out_meml(ethernet_device.mem_base + addr, value);
    }
    else {
        outportl(ethernet_device.io_base, addr);
        outportl(ethernet_device.io_base + addr + 4, value);
    }
}

void detect_eerprom() {
    uint32_t t;
    ethernet_write_command(REG_EEPROM, 0x1);
    for(int i = 0; i < 1000 && ethernet_device.eeprom_exist == 0; i++) {
        t = ethernet_read_command(REG_EEPROM);
        if(t & 0x10)
            ethernet_device.eeprom_exist = 1;
    }
}

uint32_t eeprom_read(uint8_t addr) {
    uint16_t data = 0;
    uint32_t tmp = 0;
    if (ethernet_device.eerprom_exists) {
        ethernet_write_command(REG_EEPROM, (1) | ((uint32_t)(addr) << 8) );
        while(!((tmp = ethernet_read_command(REG_EEPROM)) & (1 << 4)) );
    }
    else {
        ethernet_write_command( REG_EEPROM, (1) | ((uint32_t)(addr) << 2) );
        while(!((tmp = ethernet_read_command(REG_EEPROM)) & (1 << 1)) );
    }
    data = (uint16_t)((tmp >> 16) & 0xFFFF);
    return data;
}

int read_mac_addr() {
    uint32_t t;
    if(ethernet_device.eeprom_exist) {
        t = eeprom_read(0);
        ethernet_devic.emac_addr[0] = t & 0xff;
        ethernet_devic.mac_addr[1] = t >> 8;

        t = eeprom_read(0);
        ethernet_devic.mac_addr[2] = t & 0xff;
        ethernet_devic.mac_addr[3] = t >> 8;

        t = eeprom_read(0);
        ethernet_devic.mac_addr[4] = t & 0xff;
        ethernet_devic.mac_addr[5] = t >> 8;

    }
    else {
        uint8_t * mem_base_mac_8 = (uint8_t *) (mem_base+0x5400);
        uint32_t * mem_base_mac_32 = (uint32_t *) (mem_base+0x5400);
        if (mem_base_mac_32[0] != 0 ) {
            for(int i = 0; i < 6; i++) {
                ethernet_device.mac_addr[i] = mem_base_mac_8[i];
            }
        }
    }
}

void enable_interrupt() {
    ethernet_write_command(REG_IMASK ,0x1F6DC);
    ethernet_write_command(REG_IMASK ,0xff & ~4);
    ethernet_read_command(0xc0);
}

void ethernet_handler(register_t * reg) {
    uint32_t status = ethernet_read_command(0xc0);
    // Receive
    if(status & 0x80) {
        while(ethernet_device.rx_descs[ethernet_device.rx_cur].status & 0x1) {
            uint8_t * buf = (uint8_t *)ethernet_device.rx_descs[rx_cur]->addr;
            uint16_t len = rx_descs[rx_cur]->length;

            // Your packet is in buf now!

            rx_descs[rx_cur]->status = 0;
            ethernet_write_command(REG_RXDESCTAIL, rx_cur);
            rx_cur = (rx_cur + 1) % E1000_NUM_RX_DESC;
        }
    }
}

void sendPacket(void * p_data, uint16_t p_len) {
    ethernet_device.tx_descs[ethernet_device.tx_cur]->addr = (uint64_t)p_data;
    ethernet_device.tx_descs[ethernet_device.tx_cur]->length = p_len;
    ethernet_device.tx_descs[ethernet_device.tx_cur]->cmd = CMD_EOP | CMD_IFCS | CMD_RS | CMD_RPS;
    ethernet_device.tx_descs[ethernet_device.tx_cur]->status = 0;
    uint8_t old_cur = tx_cur;
    tx_cur = (tx_cur + 1) % E1000_NUM_TX_DESC;
    ethernet_write_command(REG_TXDESCTAIL, tx_cur);
    while(!(tx_descs[old_cur]->status & 0xff));
}

/*
 * Receive buffer descriptor init
 */
void rx_init() {
    rx_desc_t * descs;
    void * virtual_addr = kmalloc(sizeof(rx_desc_t) * E1000_NUM_RX_DESC + 16);
    void * phys_addr = virtual2phys(kpagedir, virtual_addr);

    descs = phys_addr;

    for(int i = 0; i < E1000_NUM_RX_DESC; i++) {
        ethernet_device.rx_descs[i] = (rx_desc_t *)((uint8_t *)descs + i*16);
        ethernet_device.rx_descs[i]->addr = (uint64_t)(uint8_t *)(kmalloc_ptr->khmalloc(8192 + 16));
        ethernet_device.rx_descs[i]->status = 0;
    }
    ethernet_write_command(REG_TXDESCLO, (uint32_t)((uint64_t)phys_addr>> 32) );
    ethernet_write_command(REG_TXDESCHI, (uint32_t)((uint64_t)phys_addr & 0xFFFFFFFF));
    ethernet_write_command(REG_RXDESCLO, (uint64_t)phys_addr);
    ethernet_write_command(REG_RXDESCHI, 0);
    ethernet_write_command(REG_RXDESCLEN, E1000_NUM_RX_DESC * 16);
    ethernet_write_command(REG_RXDESCHEAD, 0);
    ethernet_write_command(REG_RXDESCTAIL, E1000_NUM_RX_DESC - 1);
    ethernet_write_command(REG_RCTRL, RCTL_EN| RCTL_SBP| RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC  | RCTL_BSIZE_2048);
    ethernet_device.rx_cur = 0;
}

/*
 * Transfer buffer descriptor init
 */
void tx_init() {
    tx_desc_t * descs;
    void * virtual_addr = kmalloc(sizeof(tx_desc_t) * E1000_NUM_TX_DESC + 16);
    void * phys_addr = virtual2phys(kpagedir, virtual_addr);
    descs = (tx_desc_t *)phys_addr;
    for(int i = 0; i < E1000_NUM_TX_DESC; i++) {
        tx_descs[i] = (tx_desc_t *)((uint8_t*)descs + i * 16);
        tx_descs[i]->addr = 0;
        tx_descs[i]->cmd = 0;
        tx_descs[i]->status = TSTA_DD;
    }
    ethernet_write_command(REG_TXDESCHI, (uint32_t)((uint64_t)phys_addr >> 32) );
    ethernet_write_command(REG_TXDESCLO, (uint32_t)((uint64_t)phys_addr & 0xFFFFFFFF));
    ethernet_write_command(REG_TXDESCLEN, E1000_NUM_TX_DESC * 16);
    ethernet_write_command(REG_TXDESCHEAD, 0);
    ethernet_write_command(REG_TXDESCTAIL, 0);
    ethernet_write_command(REG_TCTRL,  TCTL_EN | TCTL_PSP | (15 << TCTL_CT_SHIFT) | (64 << TCTL_COLD_SHIFT) | TCTL_RTLC);
    ethernet_write_command(REG_TIPG,  0x0060200A);
    tx_cur = 0;
}

/*
 * Initialize the ethernet card driver
 * */
void ethernet_init() {
    // First get the network device using PCI
    pci_ethernet_device = pci_get_device(ETHERNET_VENDOR_ID, E1000_DEVICE_ID);
    // Get bar type
    ethernet_device.bar_type = pci_read(pci_ethernet_device, PCI_BAR0);
    // Get io base or mem base by extracting the high 28/30 bits
    ethernet_device.io_base = ethernet_device.bar_type & (~0x3);
    ethernet_device.mem_base = ethernet_device.bar_type & (~0xf);

    // Enable PCI Bus Mastering
    uint32_t pci_command_reg = pci_read(ata_device, PCI_COMMAND);
    if(!(pci_command_reg & (1 << 2))) {
        pci_command_reg |= (1 << 2);
        pci_write(ata_device, PCI_COMMAND, pci_command_reg);
    }

    // Detect EEPROM and read mac address
    detect_eeprom();
    read_mac_addr();

    for(int i = 0; i < 0x80; i++)
        ethernet_write_command(0x5200 + i*4, 0);

    // Register and enable network interrupts
    uint32_t irq_num = pci_read(pci_ethernet_device, PCI_INTERRUPT_LINE);
    register_interrupt_handler(32 + irq_num, ethernet_handler);
    enableInterrupt();
    rxinit();
    txinit();
}
