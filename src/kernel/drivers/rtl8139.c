#include <rtl8139.h>
#include <pci.h>
#include <serial.h>
#include <string.h>
#include <xxd.h>

pci_dev_t pci_rtl8139_device;
rtl8139_dev_t rtl8139_device;

uint32_t current_packet_ptr;

// Four TXAD register, you must use a different one to send packet each time(for example, use the first one, second... fourth and back to the first)
uint8_t TSAD_array[4] = {0x20, 0x24, 0x28, 0x2C};
uint8_t TSD_array[4] = {0x10, 0x14, 0x18, 0x1C};

void receive_packet() {
    uint16_t * t = (uint16_t*)(rtl8139_device.rx_buffer + current_packet_ptr);
    // Skip packet header, get packet length
    uint16_t packet_length = *(t + 1);

    // Skip, packet header and packet length, now t points to the packet data
    t = t + 2;
    //qemu_printf("Printing packet at addr 0x%x\n", (uint32_t)t);
    //xxd(t, packet_length);

    // Now, ethernet layer starts to handle the packet(be sure to make a copy of the packet, insteading of using the buffer)
    // and probabbly this should be done in a separate thread...
    void * packet = kmalloc(packet_length);
    memcpy(packet, t, packet_length);
    ethernet_handle_packet(packet, packet_length);

    current_packet_ptr = (current_packet_ptr + packet_length + 4 + 3) & RX_READ_POINTER_MASK;

    if(current_packet_ptr > RX_BUF_SIZE)
        current_packet_ptr -= RX_BUF_SIZE;

    outports(rtl8139_device.io_base + CAPR, current_packet_ptr - 0x10);
}

void rtl8139_handler(register_t * reg) {
    //qemu_printf("RTL8139 interript was fired !!!! \n");
    uint16_t status = inports(rtl8139_device.io_base + 0x3e);

    if(status & TOK) {
        qemu_printf("Packet sent\n");
    }
    if (status & ROK) {
        //qemu_printf("Received packet\n");
        receive_packet();
    }


    outports(rtl8139_device.io_base + 0x3E, 0x5);
}

void read_mac_addr() {
    uint32_t mac_part1 = inportl(rtl8139_device.io_base + 0x00);
    uint16_t mac_part2 = inports(rtl8139_device.io_base + 0x04);
    rtl8139_device.mac_addr[0] = mac_part1 >> 0;
    rtl8139_device.mac_addr[1] = mac_part1 >> 8;
    rtl8139_device.mac_addr[2] = mac_part1 >> 16;
    rtl8139_device.mac_addr[3] = mac_part1 >> 24;

    rtl8139_device.mac_addr[4] = mac_part2 >> 0;
    rtl8139_device.mac_addr[5] = mac_part2 >> 8;
    qemu_printf("MAC Address: %01x:%01x:%01x:%01x:%01x:%01x\n", rtl8139_device.mac_addr[0], rtl8139_device.mac_addr[1], rtl8139_device.mac_addr[2], rtl8139_device.mac_addr[3], rtl8139_device.mac_addr[4], rtl8139_device.mac_addr[5]);
}

void get_mac_addr(uint8_t * src_mac_addr) {
    memcpy(src_mac_addr, rtl8139_device.mac_addr, 6);
}

void rtl8139_send_packet(void * data, uint32_t len) {
    // First, copy the data to a physically contiguous chunk of memory
    void * transfer_data = kmalloc_a(len);
    void * phys_addr = virtual2phys(kpage_dir, transfer_data);
    memcpy(transfer_data, data, len);

    // Second, fill in physical address of data, and length
    outportl(rtl8139_device.io_base + TSAD_array[rtl8139_device.tx_cur], (uint32_t)phys_addr);
    outportl(rtl8139_device.io_base + TSD_array[rtl8139_device.tx_cur++], len);
    if(rtl8139_device.tx_cur > 3)
        rtl8139_device.tx_cur = 0;
}

/*
 * Initialize the rtl8139 card driver
 * */
void rtl8139_init() {
    // First get the network device using PCI
    pci_rtl8139_device = pci_get_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, -1);
    uint32_t ret = pci_read(pci_rtl8139_device, PCI_BAR0);
    rtl8139_device.bar_type = ret & 0x1;
    // Get io base or mem base by extracting the high 28/30 bits
    rtl8139_device.io_base = ret & (~0x3);
    rtl8139_device.mem_base = ret & (~0xf);
    qemu_printf("rtl8139 use %s access (base: %x)\n", (rtl8139_device.bar_type == 0)? "mem based":"port based", (rtl8139_device.bar_type != 0)?rtl8139_device.io_base:rtl8139_device.mem_base);

    // Set current TSAD
    rtl8139_device.tx_cur = 0;

    // Enable PCI Bus Mastering
    uint32_t pci_command_reg = pci_read(pci_rtl8139_device, PCI_COMMAND);
    if(!(pci_command_reg & (1 << 2))) {
        pci_command_reg |= (1 << 2);
        pci_write(pci_rtl8139_device, PCI_COMMAND, pci_command_reg);
    }

    // Send 0x00 to the CONFIG_1 register (0x52) to set the LWAKE + LWPTN to active high. this should essentially *power on* the device.
    outportb(rtl8139_device.io_base + 0x52, 0x0);

    // Soft reset
    outportb(rtl8139_device.io_base + 0x37, 0x10);
    while((inportb(rtl8139_device.io_base + 0x37) & 0x10) != 0) {
        // Do nothibg here...
    }

    // Allocate receive buffer
    rtl8139_device.rx_buffer = kmalloc(8192 + 16 + 1500);
    memset(rtl8139_device.rx_buffer, 0x0, 8192 + 16 + 1500);
    outportl(rtl8139_device.io_base + 0x30, (uint32_t)virtual2phys(kpage_dir,rtl8139_device.rx_buffer));

    // Sets the TOK and ROK bits high
    outports(rtl8139_device.io_base + 0x3C, 0x0005);

    // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP
    outportl(rtl8139_device.io_base + 0x44, 0xf | (1 << 7));

    // Sets the RE and TE bits high
    outportb(rtl8139_device.io_base + 0x37, 0x0C);

    // Register and enable network interrupts
    uint32_t irq_num = pci_read(pci_rtl8139_device, PCI_INTERRUPT_LINE);
    register_interrupt_handler(32 + irq_num, rtl8139_handler);
    qemu_printf("Registered irq interrupt for rtl8139, irq num = %d\n", irq_num);

    read_mac_addr();
}
