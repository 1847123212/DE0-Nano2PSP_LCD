DE0-Nano2PSP_LCD
================

This repo contains all the code necessary for setting up a Nios II softcore
able to drive a PSP 1000 LCD. The pin mapping in this project are made for 
a daugther board I have made for the DE0-Nano board. Schematics for the board
can be found at: ???


Build instructions
------------------
These instructions are testet with quartus 12.1

1. Open project in quartus
2. Open Qsys
3. In Tools select `generate´
4. Close Qsys
5. Compile in Quartus (Time to make a cup of coffee)
6. In Tools select `Nios II Software Build Tool for Eclipse`
7. Set project root as workspace
8. File->New->Nios II Application and BSP from Template
9. SOPC Information File name is the ending on .sopcinfo in project root
10. Name the project display_test and click Finish
11. Right click on the folder ending with display_test_bsp find Niso II and pick BSP Editor
12. In main tab under hal select enable_reduced_device_drivers and enable_small_c_library
13. Under the Linker Script tap set all regions to onchip_memory
14. Click Generate and Exit
15. Delete hello_world.c from display_test folder
16. Go to terminal and make symlink from src folder in git root to software/display_test/
17. Select Project->Build All
18. Program FPGA
19. Right click display_test folder and select Run As -> Nios II Hardware

If all is OK the LCD display should now show a test screen.
