#include <stdio.h>
#include <sys/io.h>
#include <unistd.h>
#include "pci.h"

const unsigned int PCI_ENABLE_BIT = 0x80000000;
const unsigned int PCI_CONFIG_ADDRESS = 0xCF8;
const unsigned int PCI_CONFIG_DATA = 0xCFC;

FILE* out;

unsigned int getRegister(unsigned char bus, unsigned char device, unsigned char func, unsigned char pcireg)
{

    unsigned int tested_address = PCI_ENABLE_BIT | (bus << 16) | (device << 11) | (func << 8) | (pcireg << 2);

    outl(tested_address, PCI_CONFIG_ADDRESS);
    unsigned int ret = inl(PCI_CONFIG_DATA);

    return ret;
}

void printDev(unsigned int reg)
{
    unsigned int venId = reg & 0xFFFF;
    unsigned int devId = reg >> 16;
    printf("\nVenID = 0x%08x\nDevID = 0x%08x\n", venId, devId);
    fprintf(out, "\nVenID = 0x%08x\nDevID = 0x%08x\n", venId, devId);
    for(int i = 0; i < PCI_VENTABLE_LEN; i++)
    {

        if(venId == PciVenTable[i].VendorId)
        {
            printf("vendor: %s\n", PciVenTable[i].VendorName);
            fprintf(out, "vendor: %s\n", PciVenTable[i].VendorName);
        }
    }

    for(int i = 0; i < PCI_DEVTABLE_LEN; i++)
    {
        if(devId == PciDevTable[i].DeviceId && venId == PciDevTable[i].VendorId)
        {
            printf("device: %s\n", PciDevTable[i].DeviceName);
            fprintf(out, "device: %s\n", PciDevTable[i].DeviceName);
        }
    }
}

char * interruptPinType(unsigned char pin)
{
    switch(pin)
    {
    case 0:
        return "not used";
        break;
    case 1:
        return "INTA#";
        break;
    case 2:
        return "INTB#";
        break;
    case 3:
        return "INTC#";
        break;
    case 4:
        return "INTD#";
        break;
    case 5:
        return "reserve";
        break;
    }
}

void checkDevice(unsigned char bus, unsigned char device, unsigned char func)
{
    unsigned int reg = getRegister(bus, device, func, 0x08);
    unsigned char baseClass = reg >> 24;
    unsigned char subClass = (reg >> 16) & 0xFF;
    unsigned char interface = (reg >> 8) & 0xFF;

    if(baseClass != 0x06)
    {
        char isFound = 0;
        for(int i = 0; i < PCI_CLASSCODETABLE_LEN; i++)
        {
            if(baseClass == PciClassCodeTable[i].BaseClass
                    && subClass == PciClassCodeTable[i].SubClass
                    && interface == PciClassCodeTable[i].ProgIf)
            {
                printf("class code: \n\tbase class: %s \n\tsubclass: %s \n\tinterface: %s\n", PciClassCodeTable[i].BaseDesc, PciClassCodeTable[i].SubDesc, PciClassCodeTable[i].ProgDesc);
                fprintf(out, "class code: \n\tbase class: %s \n\tsubclass: %s \n\tinterface: %s\n", PciClassCodeTable[i].BaseDesc, PciClassCodeTable[i].SubDesc, PciClassCodeTable[i].ProgDesc);
                isFound = 1;
            }
        }
        if(!isFound)
        {
            printf("class code: not found\n");
            fprintf(out, "class code: not found\n");
        }

        reg = getRegister(bus, device, func, 0x30);
        unsigned char baseIO = reg & 0xFFFF;
        printf("base IO: 0x%08x\n", baseIO);
        fprintf(out, "base IO: 0x%08x\n", baseIO);


        reg = getRegister(bus, device, func, 0x3C);
        unsigned char interruptPin = (reg >> 8) & 0xFF;
        printf("interrupt pin: %s (0x%08x)\n", interruptPinType(interruptPin), interruptPin);
        fprintf(out, "interrupt pin: %s (0x%08x)\n", interruptPinType(interruptPin), interruptPin);

    }
}

int find()
{
    unsigned char bus, device, func;
    unsigned int reg;
    unsigned int count = 0;

    for (bus = 0; bus < 255; bus++)
    {
        for (device = 0; device < 32; device++)
        {
            for (func = 0; func < 8; func++)
            {
                reg = getRegister(bus, device, func, 0);
                if (reg != 0xffffffff)
                {
                    count++;
                    printf("Device #%d\n", count);
                    printf("bus %d, device %d, func %d: vendor=0x%08x\n", bus, device, func, reg);

                    fprintf(out, "Device #%d\n", count);
                    fprintf(out, "bus %d, device %d, func %d: vendor=0x%08x\n", bus, device, func, reg);

                    printDev(reg);
                    checkDevice(bus, device, func);

                    printf("\n\n\n");
                    fprintf(out, "\n\n\n");
                }
            }
        }
    }
    return 0;
}

int main()
{

    if (geteuid() != 0)
    {
        printf("please run under root\n");
    }
    else
    {
        out = fopen("output.txt", "w");
        iopl(3);
        find();
    }

    return 0;
}
