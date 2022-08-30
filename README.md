# DEMO_MODULE_MANAGER_THREADX

![Alt text](/Documentations/setup.jpg?raw=true "Demo HW setup")

Demonstration of the Azure-RTOS ThreadX Module Manager functionality and the associated run time loadable Module.

In order to isolate the Module memory the SoC Memory Protection Unit (aka MPU) functionality is also used.

In order to download the Module via FTP and save it as a file the NetX and LevelX/FileX middlewares are also used.

The demo can be configured in order to demonstrate:

-- Module executed as external downloadable file (aka Module execution from file)

-- Module executed as internally flashed (aka Module execution in place)

-- Module debugging executing the module in place from internal Flash memory

For more informations about the B-U585I-IOT HW setup please visit https://www.st.com/en/evaluation-tools/b-u585i-iot02a.html

To update the FW of WiFi MXCHIP mounted on B-U585I-IOT please visit https://www.st.com/content/st_com/en/products/development-tools/software-development-tools/stm32-software-development-tools/stm32-utilities/x-wifi-emw3080b.html

![Alt text](/Documentations/B-U585I-IOT.jpg?raw=true "B-U585I-IOT")