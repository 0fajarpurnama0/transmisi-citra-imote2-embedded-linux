#
# Makefile for Linux TOS_MAC driver based on CC2420
#
#
#cc2420-y += CC2420.o HPLCC2420.o
#obj-$(CONFIG_TOS_MAC) += cc2420.o
#obj-$(CONFIG_TOS_MAC) += HPLCC2420.o
#obj-$(CONFIG_TOS_MAC) += CC2420.o
#obj-$(CONFIG_TOS_MAC) += TOSMac.o
#tos_mac-y += TOSMac.o CC2420.o HPLCC2420.o
tos_mac-y += byteorder.o ostimer.o SG2_CC2420.o CC2420.o TOSMac.o
obj-$(CONFIG_TOS_MAC) += tos_mac.o

ARMCC=iwmmxt_le-gcc

clean:
	rm -f *.o 
	rm -f *.mod.c
	rm -f *.mod.o
	rm -f *.ko
	rm -f CntToLedsAndFrm RfmToLeds

utils:
	$(ARMCC) CntToLedsAndRfm.c -o CntToLedsAndRfm
	$(ARMCC) RfmToLeds.c -o RfmToLeds
