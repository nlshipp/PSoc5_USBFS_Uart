# PSoc5_USBFS_Uart
USBFS Uart test.  Shows two issues, USBFS_GetChar() puts USB into bad state if more than 1 char is in USB packet - subsequent USBFS_GetChar() calls return random data.

USBFS_PutString/_PutData outputs garbage if passed string constant when using DMA mode.
