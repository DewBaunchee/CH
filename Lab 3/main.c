#include <stdio.h>
#include <sys/io.h>
#include <unistd.h>
#include "pci.h"

const unsigned int PCI_ENABLE_BIT = 0x80000000;
const unsigned int PCI_CONFIG_ADDRESS = 0xCF8;
const unsigned int PCI_CONFIG_DATA = 0xCFC;

FILE * output;

unsigned int try_device(unsigned char bus, unsigned char device, unsigned char func, unsigned char pcireg)
{

    unsigned int tested_address = PCI_ENABLE_BIT | (bus << 16) | (device << 11) | (func << 8) | (pcireg << 2);

    outl(tested_address, PCI_CONFIG_ADDRESS);
    unsigned int ret = inl(PCI_CONFIG_DATA);

    return ret;
}

void printDev(unsigned int data)
{
    unsigned int venId = data & 0xFFFF;
    unsigned int devId = data >> 16;

    printf("\nVendorID = 0x%08x\nDeviceID = 0x%08x\n\n", venId, devId);
    fprintf(output, "\nVendorID = 0x%08x\nDeviceID = 0x%08x\n\n", venId, devId);
    for(int i = 0; i < PCI_VENTABLE_LEN; i++)
    {

        if(venId == PciVenTable[i].VendorId)
        {
            printf("Vendor: %s\n", PciVenTable[i].VendorName);
            fprintf(output, "Vendor: %s\n", PciVenTable[i].VendorName);
        }
    }

    for(int i = 0; i < PCI_DEVTABLE_LEN; i++)
    {
        if(devId == PciDevTable[i].DeviceId && venId == PciDevTable[i].VendorId)
        {
            printf("Device: %s\n", PciDevTable[i].DeviceName);
            fprintf(output, "Device: %s\n", PciDevTable[i].DeviceName);
        }
    }
}

char * interruptPinType(unsigned char pin)
{
    switch(pin)
    {
    case 0:
        return "Not used";
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
        return "Reserved";
        break;
    }
}

void checkDevice(unsigned char bus, unsigned char device, unsigned char func)
{
    unsigned int data = try_device(bus, device, func, 0x08);
    unsigned char baseClass = data >> 24;
    unsigned char subClass = (data >> 16) & 0xFF;
    unsigned char interface = (data >> 8) & 0xFF;

    if(baseClass != 0x06)
    {
        printf("+----------------------------------------------+\n");
        fprintf(output, "+----------------------------------------------+\n");
        char isFound = 0;
        for(int i = 0; i < PCI_CLASSCODETABLE_LEN; i++)
        {
            if(baseClass == PciClassCodeTable[i].BaseClass
                    && subClass == PciClassCodeTable[i].SubClass
                    && interface == PciClassCodeTable[i].ProgIf)
            {
                printf("Class code: \n\tBase class: %s \n\tSubclass: %s \n\tInterface: %s\n", PciClassCodeTable[i].BaseDesc, PciClassCodeTable[i].SubDesc, PciClassCodeTable[i].ProgDesc);
                fprintf(output, "Class code: \n\tBase class: %s \n\tSubclass: %s \n\tInterface: %s\n", PciClassCodeTable[i].BaseDesc, PciClassCodeTable[i].SubDesc, PciClassCodeTable[i].ProgDesc);
                isFound = 1;
            }
        }

        if(!isFound)
        {
            printf("Class code: [NOT FOUND]\n");
            fprintf(output, "Class code: [NOT FOUND]\n");
        }
        data = try_device(bus, device, func, 0x3C);
        unsigned char interruptPin = (data >> 8) & 0xFF;

        printf("+----------------------------------------------+\n");
        printf("Interrupt pin: %s (0x%08x)\n", interruptPinType(interruptPin), interruptPin);
        fprintf(output, "+----------------------------------------------+\n");
        fprintf(output, "Interrupt pin: %s (0x%08x)\n", interruptPinType(interruptPin), interruptPin);
    }
    else
    {
        unsigned int data = try_device(bus, device, func, 0x18);
        unsigned char subordinateBus = (data >> 16) & 0xFF;
        unsigned char secondaryBus = (data >> 8) & 0xFF;
        unsigned char primaryBus = data & 0xFF;

        printf("+----------------------------------------------+\n");
        printf("Bus: \n\tSubordinate bus: 0x%08x \n\tSecondary bus: 0x%08x \n\tPrimary bus: 0x%08x\n", subordinateBus, secondaryBus, primaryBus);
        fprintf(output, "+----------------------------------------------+\n");
        fprintf(output, "Bus: \n\tSubordinate bus: 0x%08x \n\tSecondary bus: 0x%08x \n\tPrimary bus: 0x%08x\n", subordinateBus, secondaryBus, primaryBus);
    }
}

int find()
{
    unsigned char bus, device, func;
    unsigned int data;

    for (bus = 0; bus < 255; bus++)
    {
        for (device = 0; device < 32; device++)
        {
            for (func = 0; func < 8; func++)
            {
                data = try_device(bus, device, func, 0);
                if (data != 0xffffffff)
                {
                    printf("bus %d, device %d, func %d: vendor=0x%08x\n", bus, device, func, data);
                    fprintf(output, "bus %d, device %d, func %d: vendor=0x%08x\n", bus, device, func, data);

                    printDev(data);
                    checkDevice(bus, device, func);

                    printf("===================================================================\n");
                    fprintf(output, "===================================================================\n");
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
        output = fopen("out.txt", "w");
        iopl(3);
        find();
    }

    return 0;
}
