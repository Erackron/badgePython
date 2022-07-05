import mch22

SPI_CMD_NOP1             = 0x00
SPI_CMD_WISHBONE         = 0xf0
SPI_CMD_LOOPBACK         = 0xf1
SPI_CMD_LCD_PASSTHROUGH  = 0xf2
SPI_CMD_BUTTON_REPORT    = 0xf4
SPI_CMD_FREAD_GET        = 0xf8
SPI_CMD_FREAD_PUT        = 0xf9
SPI_CMD_IRQ_ACK          = 0xfd
SPI_CMD_RESP_ACK         = 0xfe
SPI_CMD_NOP2             = 0xff

class FPGAWB:
    def __init__(self):
        self.buf = bytearray()
        self.buf.append(SPI_CMD_WISHBONE)
        self.read_cnt = 0

    def queue_write(self, dev, addr, val):
        # Dev sel & Mode (Write, Re-Address)
        self.buf.append(0xc0 | (dev & 0xf))

        # Address
        self.buf.append((addr >> 18) & 0xff);
        self.buf.append((addr >> 10) & 0xff);
        self.buf.append((addr >> 2) & 0xff);

        # Data
        self.buf.append((val >> 24) & 0xff);
        self.buf.append((val >> 16) & 0xff);
        self.buf.append((val >> 8) & 0xff);
        self.buf.append((val) &0xff);

    def queue_read(self, dev, addr):
        # Dev sel & Mode (Write, Re-Address)
        self.buf.append(0x40 | (dev & 0xf))

        # Address
        self.buf.append((addr >> 18) & 0xff);
        self.buf.append((addr >> 10) & 0xff);
        self.buf.append((addr >> 2) & 0xff);

        # Data
        self.buf.extend(b'\x00\x00\x00\x00');
        self.read_cnt = self.read_cnt+1

    def exec(self):
        mch22.fpga_send(bytes(self.buf))
        readcmd = bytearray(2+(self.read_cnt*4))
        readcmd[0] = SPI_CMD_RESP_ACK
        res = mch22.fpga_transaction(bytes(readcmd))
        return [res[2+i*4:6+i*4] for i in range(self.read_cnt)]

