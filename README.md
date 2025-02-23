It send a command to make device reboot to booloader mode and then start an upgrade. There is a wait of 500ms after reboot command.
To a clean chip, it also no problem because the first reboot command will be ignored by bootloader. To speed up the progress on new chips, the first command and wait ccan be removed.

Example:
D:\>USBHID_Upgrade.exe EFM8UB1_HIDADC.bin
Read file size 1637(0x665) bytes
0 = \\?\hid#vid_10c4&pid_eac9#8&2f401957&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}
.................
D:\>
