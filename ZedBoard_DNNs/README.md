This document intends to be a guide to be able use the Xilinx Edge AI tools with a ZedBoard SoC device. Most of the documentation, tutorials and examples provided by Xilinx in order to use their AI libraries and tools have been created for boards with Zynq MPSoCs Ultrascale+ chips and others. Never the less, the hardware IP block created by Xilinx to run DNN inference on their boards, the Deep Learning Procesing Unit (DPU), supports its implementation on Zynq-7000 family chips. ZedBoard mouns a Z-7020 chip, which is compatible with this hardware description block.

In this guide it is preteded to explain the whole process to implement DNN inference on Zedboard. Software tools that have to be installed, creation of the hardware description project, configuration of an Operating System project to use this harware description with the ZedBoard, installation of Edge AI compilation tools and inference of several DNN models such as mobilenetv1, mobilenetv2 and inceptionv1.

### Table of Contents

- [Prerequisites](#prereqisites)
- [Hardware description project](#hardware-description-project)
  - [Create a Vivado Design Suite Project](#create-a-vivado-design-suite-project)
  - [Import DPU IP to the project](#import-dpu-ip-to-the-project)
  - [Import and Interconnect all necessary IP blocks](#import-and-interconnect-all-necessary-ip-blocks)




Prerequisites
-------------
Which Ubuntu I'm using, all tools of Xilinx I'm using ... .
In the Vivado installation, import the board files.





Hardware description project
----------------------------
A project with a hardware description for ZedBoard has to be created in order to perform inference of any DNN. The essential hardware block to be able to implement DNN onto the ZedBoard is the DPU (Deep Learning Procesing Unit), which is an IP block created by Xilinx. In order to be able to import the DPU block to your own project, you have to download the DPU target reference design (TRD) [here](https://www.xilinx.com/products/intellectual-property/dpu.html#overview).

This TRD has been created by Xilinx to use the DPU with the ZCU102 board, which has a Zynq MPSoC UltraScale+ chip. Therefore, the only part that is needed from this `.zip` file we download is the DPU IP block, which is going to be impornted into the project created for ZedBoard.



### Create a Vivado Design Suite Project
Open the Vivado tool. One easy way to do this in Ubuntu 18.04 LTS would be to open up a terminal window and type in `Vivado`. Once the software has booted, follow the isntructions:

- Press the `Create Project` option.

![alt text](https://raw.githubusercontent.com/UviDTE-FPSoC/vitis-dnn/master/ZedBoard_DNNs/GuideImages/CreateProject.png)

- Give a name to your project, for example *ZedBoard_DPU_2019_2*. Select a directory to save the project at. In this, case the directory doesn't have to be saved to any special location.
- Select an RTL Project

![alt text](https://raw.githubusercontent.com/UviDTE-FPSoC/vitis-dnn/master/ZedBoard_DNNs/GuideImages/RTLProject.png)

- Click next in the `Add Sources` and `Add Constraints` windows.
- In the `Default Part` window, select the boards option and look for *ZedBoard* in the 'Search' menu, as shown in the image. Once you have selected ZedBoard, click next.

![alt text](https://raw.githubusercontent.com/UviDTE-FPSoC/vitis-dnn/master/ZedBoard_DNNs/GuideImages/BoardFile.png)

- Once you are in the project summary, click `Finish`.


### Import the DPU IP to the project
Enter the directory where the DPU TRD `.zip` file was downloaded to and extract it at any location. Now, within the folder that was extracted, enter this directory.

```
cd zcu102-dpu-trd-2019-1-timer/pl/srcs/
```

In this directory there is a folder named *dpu_ip*. This is the folder which contains the DPU IP block and which is necesary to import into the previously created project. It is not necessary, but it's recommended to copy this folder into the *ZedBoard_DPU_2019_2* folder project. Open up a terminal in the directory which is shown above and enter the following commands.

```
cp dpu_ip /media/arroas/HDD/MinhasCousas/EEI/Mestrado/2_Curso/TFM/vitis-dnn/ZedBoard_DNNs/ZedBoard_DPU_2019_2/ZedBoard_DPU_2019_2.ip_user_files/
```

It is possible to now go into the Vivado project and press the `Tools` scroll-down window. Press on the `Settings` option, which will open a new window.

![alt text](https://raw.githubusercontent.com/UviDTE-FPSoC/vitis-dnn/master/ZedBoard_DNNs/GuideImages/AddIPBlock.png)

In this window, press the `IP >>> Repository` option. You can now press the add `+` option and search for the directory the *dpu_ip* folder was copied to. After this, press the `Select` button.

This action will import the DPU IP block into you IP cathalog, allowing you to introduce it into an IP Design.



### Import and Interconnect all necessary IP blocks
The implementation of DNN inference into the ZedBoard requires a hardware description that contains not just the DPU IP block. In this section, an IP Block Design will be created, including in it all the necessary IP blocks and interconecting all of them in order to create a hardware description file which will be used to run the DNN inference on ZedBoard.

With the Vivado project *ZedBoard_DPU_Config* opened, go to the `Project Manager`, at the left side of the Vivado software window and press `Create Block Design`, under the `IP INTEGRATOR` option. A new window pops up, which allows giving the block a name, *ZedBoard_DPU_HW*, and specifying a directory and source set for the design. This options shall be left as default.

![alt text](https://raw.githubusercontent.com/UviDTE-FPSoC/vitis-dnn/master/ZedBoard_DNNs/GuideImages/IPBlockDesign_Default.png)

The IP Block Design is now created. In order to import a new block, right click in the diagram window and press the `Add IP` option. This action enables a window where it is possible to type in the name of each of the IP blocks which have to be imported. Include the following ones:

- **ZYNQ7 Processing System**. This is the IP block which describes the ZedBoard Z-7020 chip.

The configuration of the Processing System (PS) is now described. First of all, double click the Zynq block that was just imported into the project. Once the `Re-costumize` window pops up, click on the `Presets` option and select the ZedBoard preset. The preset will therefore be applied, which will enable the most common periferics of the ZedBoard such as the UART port.

![alt text](https://raw.githubusercontent.com/UviDTE-FPSoC/vitis-dnn/master/ZedBoard_DNNs/GuideImages/ZedBoard_Preset.png)

The preset sets up several signals that will be needed later on in the DPU configuration, but we will indicate the process of activating them all.

- `FCLK_CLK0`. This signal creates a 100 MHz clock signal needed for the DPU register configuration. This signal is a low frequency signal, as the registers are only configured at the beginning of any DPU task.

![alt text](https://raw.githubusercontent.com/UviDTE-FPSoC/vitis-dnn/master/ZedBoard_DNNs/GuideImages/100MHz_PS_Signal.png)

- `FCLK_RESET0_N`. Reset signal of the generated FCLK_CLK0 clock signal. It is generated automatically when enabling the previous signal.

- `M_AXI_GP0`. The PS communicates with the Programmable Logic (PL) through a 32-bit instruction to the DPU in order to control the configuration registers of the IP block. With this port, the PS works as the master and the PL, where the DPU is implemented works as slave.

![alt text](https://raw.githubusercontent.com/UviDTE-FPSoC/vitis-dnn/master/ZedBoard_DNNs/GuideImages/M_AXI_GP0.png)

- `M_AXI_GP0_ACLK`. When selecting the M_AXI_GP0 port, the aclk signal is added as default. This signal has to be connected to a clock signal that generates the frequency at which the DPU registers have to be configured. This rate would be the one created by `FCLK_CLK0`.

- `IRQ_F2P[0:0]`. Interrupt input port of the PS which enables the PS to receive the interruptions generated by any core of the DPU.

![alt text](https://raw.githubusercontent.com/UviDTE-FPSoC/vitis-dnn/master/ZedBoard_DNNs/GuideImages/Interrupt_PS.png)

- `S_AXI_HPx`. It is necessary to add three AXI slave interfaces in order to communicate data and instructions with the DPU. The picture shows how to set up three of the four available ports. The data communicating ports have to be 64 bits, and the instruction port can be 32 bits. Boards with 128-bit support for the AXI interface should use this option as the DPU data ports can work with 64 or 128 bits of width. Once these three ports have been added, two signals for each of the ports are created as well.

![alt text](https://raw.githubusercontent.com/UviDTE-FPSoC/vitis-dnn/master/ZedBoard_DNNs/GuideImages/S_AXI_HPx_32_PS.png)

- `S_AXI_HPx_FIFO_CTRL`. Signal that is not going to be used.

- `S_AXI_HPx_ACLK`. Frequency signal at which the DPU is going to exchange instructions and data with the PS.




- **Deep Learning Procesing Unit (DPU)**.
*É posible, en caso de que sexa necesario aumentar a dispoñibilidade de recursos do PL reducir o número de bloques DSP que implementa a DPU. Isto pódese levar a cabo seleccionando low dpu usage.*

- **Concat**. This block enables concatenation of different width signals into one bus.
- **AXI Interconnect**.