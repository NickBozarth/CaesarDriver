# sys is stored on my mounted usb drive and thats ok
SYSDIR=/mnt/usb1/sys

KMOD= caesar
SRCS= caesar.c

.include <bsd.kmod.mk>
